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

static int fd_fifo;
static int fd_pipe[2];
static int mplayer_stat = 0;
static char **radio_url_list = (char **)NULL;
static int radio_num = 0;
static int radio_now = 0;

static void main_report_info(char *tip,char *oled_msg,char *con_msg){
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

static void refresh_radio_panel(){
	OLED_Clear();
	OLED_ShowString(0,0,(const u8*)"Now playing:",16);
	if(radio_url_list[radio_now])
		OLED_ShowString(4,20,(const u8*)radio_url_list[radio_now],12);
	OLED_Refresh_Gram();
}

static int deleteFgetsN(char *p){
	char *find = strchr(p, '\n');
	if(find){
		*find = '\0';
		return 1;
	}
	return 0;
}

static void *get_pthread(void *arg)
{
	char buf[512];
	int doWrite = 0;
	while(mplayer_stat==0)
	{
		//  还应该分析mplayer的返回数据,这里先放着
		// 简化,按键检测应该结合中断

		if(!digitalRead(RADIO_EXIT_BUTTON)){
      sleep(0.1);
      if(!digitalRead(RADIO_EXIT_BUTTON)){
				// stop
				doWrite = 1;
				strcpy(buf,"stop\n");
			}
		}
		if(!digitalRead(RADIO_ENTER_BUTTON)){
      sleep(0.1);
      if(!digitalRead(RADIO_ENTER_BUTTON)){
				// pause
				doWrite = 1;
				strcpy(buf,"pause\n");
			}
		}
		if(!digitalRead(RADIO_NEXT_BUTTON)){
      sleep(0.1);
      if(!digitalRead(RADIO_NEXT_BUTTON)){
				// next
				doWrite = 1;
				if(++radio_now >= radio_num)	radio_now = 0;
				sprintf(buf,"load %s\n",radio_url_list[radio_now]);
			}
		}
		if(!digitalRead(RADIO_PRE_BUTTON)){
      sleep(0.1);
      if(!digitalRead(RADIO_PRE_BUTTON)){
				// front
				doWrite = 1;
				if(--radio_now < 0)	radio_now = radio_num - 1;
				sprintf(buf,"load %s\n",radio_url_list[radio_now]);
			}
		}
		if(doWrite){
			refresh_radio_panel();
			if(write(fd_fifo,buf,strlen(buf))!=strlen(buf)){
				printf("CMD Write failed!");
				perror("write");
			}
			doWrite = 0;
		}
	}
}

static void *print_pthread(void *arg)
{
	char buf[512];
	close(fd_pipe[1]);
	int size=0;
	while(mplayer_stat==0)
	{
		size=read(fd_pipe[0],buf,sizeof(buf));
		buf[size]='\0';
		printf("Mplayer return: %s\n",buf);
	}
}

static int do_git_update(){
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

static int load_radio_list(FILE *fp){
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
	int _i = 0;
	while(fgets(buf, sizeof(buf), radio_list)!=NULL && _i < radio_num){
		radio_url_list[_i] = (char *)malloc(strlen(buf)+1);
		deleteFgetsN(buf);
		strcpy(radio_url_list[_i],buf);
		_i++;
	}

	puts("Radio list:\n");
	for(_i=0;_i<radio_num;_i++){
		puts(radio_url_list[_i]);
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
		execlp("mplayer","mplayer","-slave","-quiet","-input",fifo_buf,radio_url_list[0],NULL);
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
		printf("Return mainThread:Radio");
		sleep(MPLAYER_RADIO_WAIT_SEC);
		fflush(stdout);
		return err;
}
