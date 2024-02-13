#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <wiringPi.h>

#include "oled.h"
#include "radio.h"

// mplayer通信通道
static int fd_fifo;
static int fd_pipe[2];
/* mplayer 状态 
stat = 
-1: mplayer加载中(或空载)
0:  mplayer运行中
1:  mplayer退出
*/
static int mplayer_stat = -1;
// mplayer 启动触发按键检测
static int mplayer_start_sign = 0;
// 电台url表
static char **radio_url_list = (char **)NULL;
// 电台name表
static char **radio_name_list = (char **)NULL;
// 电台总数
static int radio_num = 0;
// 当前电台
static int radio_now = 0;
// 执行panel刷新标志
static int doWrite = 0;
// 获取StreamTitle 数据头
static const char * _dataHead_st = "\nICY Info: StreamTitle=";
static int _dataHeadLen_st = strlen("\nICY Info: StreamTitle=");

// 获取 总长(s) 指令
static const char * _cmdContent_al = "get_time_length";
// 获取 总长(s) 数据头
static const char * _dataHead_al = "ANS_LENGTH=";
static int _dataHeadLen_al = strlen("ANS_LENGTH=");

// 获取 当前位置 指令
static const char * _cmdContent_atp = "get_time_pos";
// 获取 当前位置 数据头
static const char *_dataHead_atp = "ANS_TIME_POSITION=";
static int _dataHeadLen_atp = strlen("ANS_TIME_POSITION=");

/* 从 mplayer 获得的数据
 * 好吧,确实应该用消息队列写
 */
static struct mplayerRetData radioGetData;

/* 电台展示名
 * 显示 'StreamTitle' 优先于 'radio_name'
 */
static char streamTitle_disp[64];

/* 打印日志
 * 打印oled_msg至oled,con_msg至stdout(不为NULL时)
*/
static void main_report_info(char *tip,char *oled_msg,char *con_msg);
/* 打印错误日志并延时
 * 打印oled_msg至oled,con_msg至stdout(不为NULL时) ,再延时
*/
static void main_report_error(char *tip,char *oled_msg,char *con_msg);
/*  刷新播放面板(处于菜单模式不刷新)
*/
static void refresh_radio_panel();
/* 刷新进度条(处于菜单模式不刷新)
 * 现在不会显示
*/
static void refresh_radio_time_pos();
// 去除fgets获得的讨厌的'\n'
static int deleteFgetsN(char *p);
// 比较两个int型的大小
static int min(int x,int y);
// 掌管按键响应,面板刷新,命令发送的神
static void *get_pthread(void *arg);
// 掌管mplayer返回数据处理的神
static void *print_pthread(void *arg);
// 从gitee更新资源仓库
static int do_git_update();
// 加载资源描述文档
static int load_radio_list(FILE *fp);

static void main_report_info(char *tip,char *oled_msg,char *con_msg)
{
	if(oled_msg){
		OLED_Clear();
		OLED_ShowString(0,0,(const u8*)oled_msg,12);
		OLED_Refresh_Gram();
	}
	if(con_msg){
		printf("[%s]:%s\n",tip?tip:"(null)",con_msg);
	}
}

static void main_report_error(char *tip,char *oled_msg,char *con_msg)
{
	main_report_info(tip,oled_msg,con_msg);
	sleep(MPLAYER_RADIO_WAIT_SEC);
}

static void refresh_radio_panel()
{
	OLED_Clear();
	OLED_ShowString(0,0,(const u8*)"     ~~ Radio ~~",12);
	if(radioGetData.streamTitle!=NULL && strcmp(radioGetData.streamTitle,"")){
		strncpy(streamTitle_disp,radioGetData.streamTitle,sizeof(streamTitle_disp));
	}
	else{
		strncpy(streamTitle_disp,radio_name_list[radio_now],sizeof(streamTitle_disp));
	}
	OLED_ShowString(0,20,(const u8*)streamTitle_disp,12);
	OLED_Refresh_Gram();
}

static void refresh_radio_time_pos()
{
	OLED_Fill(0,50,127,63,0);
	char _t[21];
	int t_len = radioGetData.time.length;
	int t_pos = radioGetData.time.pos;
	sprintf(_t,"  %02d:%02d:%02d / %02d:%02d:%02d",t_pos/3600,(t_pos%3600)/60,t_pos%60,
													t_len/3600,(t_len%3600)/60,t_len%60);
	OLED_ShowString(0,51,(const u8*)_t,12);
	OLED_Refresh_Gram();
}

static int deleteFgetsN(char *p)
{
	char *find = strchr(p, '\n');
	if(find){
		*find = '\0';
		return 1;
	}
	return 0;
}

static int min(int x,int y)
{
	return (x<y)?x:y;
}

static void *get_pthread(void *arg)
{
	char buf[MPLAYER_RADIO_MESSAGE_SIZE];
	int menu_index = 0;
	char menu_choose[64];
	int _doWrite = 0;

	int radio_stat = 0;

	int _doNextRadio = 0;
	int _doPreRadio = 0;
	int radio_volume = 4;

	memset(buf,0,sizeof(buf));
	// 应该可以做成任务队列,这样写太麻烦,而且可以顺便获取mplayer返回
	while(mplayer_stat<=0)
	{
		if(mplayer_stat<0 && !mplayer_start_sign){
			sleep(MPLAYER_RADIO_RUN_WAIT);
			continue;
		}
		//  还应该分析mplayer的返回数据,这里先放着
		// 简化,按键检测应该结合中断
		if(radio_stat==0){
			// 播放主页
			//refresh_radio_time_pos();
			if(!digitalRead(RADIO_MENU_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_MENU_BUTTON)){
					// 选台
					radio_stat = 1;
					_doWrite = 1;
				}
			}
			if(!digitalRead(RADIO_ENTER_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_ENTER_BUTTON)){
					// pause
					doWrite = 1;
					strcpy(buf,"pause\n");
				}
			}
			if(!digitalRead(RADIO_NEXT_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_NEXT_BUTTON)){
					// next
					_doNextRadio = 1;
				}
			}
			if(!digitalRead(RADIO_PRE_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_PRE_BUTTON)){
					// front
					_doPreRadio = 1;
				}
			}
			if(!digitalRead(RADIO_VOLUP_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_VOLUP_BUTTON)){
					// volume up
					if(radio_volume<4){
						sprintf(buf,"volume %d 1\n",++radio_volume*25);
						doWrite = 1;
					}
				}
			}
			if(!digitalRead(RADIO_VOLDN_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_VOLDN_BUTTON)){
					// volume down
					if(radio_volume>0){
						sprintf(buf,"volume %d 1\n",--radio_volume*25);
						doWrite = 1;
					}
				}
			}
			if(!digitalRead(RADIO_EXIT_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_EXIT_BUTTON)){
					// stop
					doWrite = 1;
					#ifdef MPLAYER_RADIO_QUIT
					strcpy(buf,"stop\n");
					#else
					strcpy(buf,"quit\n"); // 不退出
					#endif
				}
			}
		}
		else if(radio_stat==1){
			// 选台模式内部刷新
			if(_doWrite){
				_doWrite = 0;
				OLED_Clear();
				sprintf(menu_choose,"Choose %d",menu_index);
				OLED_ShowString(0,0,(const u8*)menu_choose,12);
				int i;
				for(i=menu_index;i< menu_index + min(PANEL_HEIGHT_FONT_NUM-1,radio_num-menu_index);i++){
					snprintf(menu_choose,PANEL_WIDTH_FONT_NUM,"[%d]%s",i,radio_name_list[i]);
					OLED_ShowString(0,(i-menu_index+1)*12,(const u8*)menu_choose,12);
				}
				OLED_Refresh_Gram();
			}
			if(!digitalRead(RADIO_NEXT_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_NEXT_BUTTON)){
					// 下一项
					_doWrite = 1;
					if(++menu_index >= radio_num)	menu_index = 0;
					_doNextRadio = 1;
				}
			}
			if(!digitalRead(RADIO_PRE_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_PRE_BUTTON)){
					// 上一项
					_doWrite = 1;
					if(--menu_index < 0)	menu_index = radio_num - 1;
					_doPreRadio = 1;
				}
			}
			if(!digitalRead(RADIO_MENU_BUTTON)){
				sleep(0.01);
				if(!digitalRead(RADIO_MENU_BUTTON)){
					// 退出菜单
					radio_stat = 0;
					doWrite = 1;
					memset(buf,0,sizeof(buf));
				}
			}

		}
		if(_doNextRadio){
			doWrite = 1;
			if(++radio_now >= radio_num)	radio_now = 0;
			sprintf(buf,"load %s\n",radio_url_list[radio_now]);
			_doNextRadio = 0;
		}
		if(_doPreRadio){
			doWrite = 1;
			if(--radio_now < 0)	radio_now = radio_num - 1;
			sprintf(buf,"load %s\n",radio_url_list[radio_now]);
			_doPreRadio = 0;
		}
		if(doWrite){
			// 播放器页面刷新
			doWrite = 0;
			radioGetData.streamTitle = NULL;
			if(radio_stat == 0) refresh_radio_panel();
			int _err = 0;
			 _err |= (write(fd_fifo,buf,strlen(buf))!=strlen(buf));
			/* 这样写会命令只能执行最后一个(或者一个执行不了?) */
			//_err |= (write(fd_fifo,_cmdContent_al,strlen(_cmdContent_al)!=strlen(_cmdContent_al)));
			//_err |= (write(fd_fifo,_cmdContent_atp,strlen(_cmdContent_atp))!=strlen(_cmdContent_atp));
			if(_err){
				printf("CMD Write failed!");
				perror("write");
			}
			
		}
	}
}

static void *print_pthread(void *arg)
{
	char buf[MPLAYER_RADIO_MESSAGE_SIZE];
	close(fd_pipe[1]);
	int size=0;
	while(mplayer_stat<=0)
	{
		size=read(fd_pipe[0],buf,sizeof(buf));
		buf[size]='\0';
		// mplayer 负载启动
		if(size)	mplayer_start_sign = 1;
		// ICY Info: StreamTitle='Eliza Rose & Calvin Harris - Body Moving';
		if(!strncmp(buf,_dataHead_st,_dataHeadLen_st)){
			if(!radioGetData.streamTitle)	free(radioGetData.streamTitle);
			radioGetData.streamTitle = (char *)malloc(size);
			sscanf(buf,"\nICY Info: StreamTitle='%[^']'",radioGetData.streamTitle);
			// 触发panel更新
			refresh_radio_panel();
		}
		// ANS_LENGTH=268.00
		else if(!strncmp(buf,_dataHead_al,_dataHeadLen_al)){
			radioGetData.time.length = (int)atof((char*)buf[_dataHeadLen_al]);
		}
		// ANS_TIME_POSITION=1728.6
		else if(!strncmp(buf,_dataHead_atp,_dataHeadLen_atp)){
			radioGetData.time.pos = (int)atof((char *)buf[_dataHeadLen_atp]);
		}
		printf("Mplayer return: %s\n",buf);
	}
}

static int do_git_update()
{
	int err;
	/*
	if(!(err = chdir(MPLAYER_RADIO_RES_UPDATE_PATH))){
		perror("chdir");
		return err;
	}
	*/
	char cmd[512];
	sprintf(cmd,"cd %s && git pull origin master && cd ",MPLAYER_RADIO_RES_UPDATE_PATH);
	if(!(err = system(cmd))){
		perror("git");
		return err;
	}
	return 0;
}

static int load_radio_list(FILE *fp)
{
	if(!fp)	return 1;
	// 第一行数量
	char buf[512];
	memset(buf,0,512);
	if(!fgets(buf, sizeof(buf), fp)){
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but the radio list file error...",
											"Radio list file lost list num");
		return 1;
	}
	deleteFgetsN(buf);
	if(!(radio_num += atoi(buf))){
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but no radio list",
											"Radio list file lost radios");
		return 1;
	}
	return 0;
}

int main(int argc,char **argv)
{

	/* 参数说明:
		--no-update:			不从git库更新资源
		--help:					打印帮助s
	*/
	int update = 1;
	int i;
	for(i=1;i<argc;i++){
		if(!strcmp(argv[i],"--no-update"))	update = 0;
		if(!strcmp(argv[i],"--help")){
			puts("Mplayer Radio Help:");
			puts("--no-update:			stop pull resource from git");
			puts("--help:				print help");
			return 0;
		}
	}
	// init
	if(oled_init())
	{
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but we can't init the oled",
												"Init oled failed!");
		return 1;
	}
	// 软件设置上拉
	pullUpDnControl(BUTTON_0_PIN,PUD_UP);
	pullUpDnControl(BUTTON_1_PIN,PUD_UP);
	pullUpDnControl(BUTTON_2_PIN,PUD_UP);
	pullUpDnControl(BUTTON_3_PIN,PUD_UP);
	pullUpDnControl(BUTTON_4_PIN,PUD_UP);
	pullUpDnControl(BUTTON_5_PIN,PUD_UP);
	pullUpDnControl(BUTTON_6_PIN,PUD_UP);

	// 歌名初始化
	memset(streamTitle_disp,0,sizeof(streamTitle_disp));
	streamTitle_disp[63] = '.';
	streamTitle_disp[62] = '.';
	streamTitle_disp[61] = '.';

	// 欢迎
	OLED_ShowString(0,16,(const u8 *)(" WECLOME!"),24);
	OLED_Refresh_Gram();

	if(update){
		main_report_info(MPLAYER_RADIO_TIP,"Start pulling resources...",
				"Pulling resources from gitee...");
		int _updated;
		if((_updated = do_git_update())){
			main_report_error(MPLAYER_RADIO_TIP,"Sorry,but we can't update the resources!",
												"Get Update from gitee failed!");
		}
		else{
			main_report_info(MPLAYER_RADIO_TIP,"Update done.","Pulling Done.");
		}
	}

	int err = 0;
	char buf[512];
	memset(buf,0,512);

	// load radio list
	FILE *radio_list = NULL;
	radio_list = fopen(MPLAYER_RADIO_RES_LIST_PATH,"r");
	if(!radio_list){
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but the radio list file lost...",
											"Radio list file lost");
		//return 1;
	}
	load_radio_list(radio_list);
	printf("Found %d Radio\n",radio_num);
	if(!radio_num){
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but no radio list",
											"Radio list file lost radios");
		return 1;
	}

	// load to list
	radio_url_list = (char **)malloc(sizeof(char *)*radio_num);
	radio_name_list = (char **)malloc(sizeof(char *)*radio_num);
	int _i = 0;
	while(fgets(buf, sizeof(buf), radio_list)!=NULL && _i < radio_num){
		radio_name_list[_i] = (char *)malloc(strlen(buf)+1);
		radio_url_list[_i] = (char *)malloc(strlen(buf)+1);
		deleteFgetsN(buf);
		printf("%s\n",buf);
		// strcpy(radio_url_list[_i],buf);		note: we have use new format.
		if(sscanf(buf,"[%[^]]]%[^\n]",radio_name_list[_i],radio_url_list[_i])==2)
			// 防止了部分(好吧,我承认是大部分)歌曲名有空格
			_i++;
		else{
			free(radio_url_list[_i]);
			free(radio_name_list[_i]);
		}
	}

	puts("Radio list:\n");
	for(_i=0;_i<radio_num;_i++){
		//puts(radio_url_list[_i]);
		printf("Name:%s\tUrl:%s\n",radio_name_list[_i],radio_url_list[_i]);
	}

	int fd;
	if(mkfifo("cmd", 0777))
	{
		if(!unlink("cmd"))
			mkfifo("cmd", 0777);
	}
	if(pipe(fd_pipe)<0)
	{
		perror("pipe error\n");
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but init the radio failed!",
											"Radio-Mplayer pipe error");
		return 1;
	}
 	pid_t pid;
	pid=fork();
	if(pid<0)
	{
		perror("fork");
		main_report_error(MPLAYER_RADIO_TIP,"Sorry,but init the radio failed!",
											"Child Thread Mplayer fork failed");
		return 1;
	}
	if(pid==0)
	{
		close(fd_pipe[0]);
		dup2(fd_pipe[1],1);
		fd_fifo=open("cmd",O_RDWR);
		char fifo_buf[64];
		sprintf(fifo_buf,"file=%s","cmd");
		// when we can use '+' operater,that's better!(C++)
		execlp("mplayer","mplayer","-slave","-quiet","-idle","-input",fifo_buf,\
			radio_url_list[0],
			NULL);
	}
	else{
		pthread_t tid1;
		pthread_t tid2;

		fd_fifo=open("cmd",O_RDWR);
		if(fd<0){
			perror("open");
			main_report_error(MPLAYER_RADIO_TIP,"Sorry,but init the radio failed!",
												"Open fifo failed!");
			return 1;
		}
		pthread_create(&tid1,NULL,get_pthread,NULL);
		pthread_create(&tid2,NULL,print_pthread,NULL);
		pthread_detach(tid1);
		pthread_detach(tid2);

		refresh_radio_panel();

		int status;
		pid_t  child_pid;
		child_pid =  wait(&status);
		if (child_pid != 0){
				if (WIFEXITED(status)){
					// 正常终止
					mplayer_stat = 1;
					printf("Mplayer return %d.\n",WEXITSTATUS(status));
				}
			if(WIFSIGNALED(status)){
					// 异常终止
					mplayer_stat = 2;
					printf("Mplayer return %d.\n",WEXITSTATUS(status));
				}
			}
		}
		printf("Return mainThread:Radio\n");
		OLED_Clear();
		OLED_ShowString(24,16,(const u8 *)("BYE!"),24);
		OLED_Refresh_Gram();
		sleep(MPLAYER_RADIO_WAIT_SEC);
		fflush(stdout);
		return err;
}
