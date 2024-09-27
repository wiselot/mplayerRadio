#ifndef _RADIO_H
#define _RADIO_H

// 日志标志
#define MPLAYER_RADIO_TIP					"MplayerRadio"
// 为了用户看清oled显示的消息的延迟(s)
#define MPLAYER_RADIO_WAIT_SEC		        3
// 等待mplayer启动的延迟(s)
#define MPLAYER_RADIO_RUN_WAIT              0.5
// mplayer消息缓冲区大小
#define MPLAYER_RADIO_MESSAGE_SIZE          512
// 定义时mplayer执行stop(停止),否则quit(返回tty)
#define MPLAYER_RADIO_QUIT
// 消息队列大小
#define MPLAYER_RADIO_CMD_QUEUE_SIZE        8

// 本地资源库路径
#define MPLAYER_RADIO_RES_UPDATE_PATH	"/home/wiselot/Radio/mplayer-radio-res"
// 本地资源描述文件路径
#define MPLAYER_RADIO_RES_LIST_PATH     "/home/wiselot/Radio/mplayer-radio-res/radio_list.txt"
// 远程资源仓库(git)
#define MPLAYER_RADIO_RES_UPDATE_SITE	"https://gitee.com/wiselot/mplayer-radio-res.git"

// wiring 编号的按键gpio

#define BUTTON_0_PIN     21
#define BUTTON_1_PIN     22
#define BUTTON_2_PIN     23
#define BUTTON_3_PIN     24
#define BUTTON_4_PIN     25
#define BUTTON_5_PIN     26
#define BUTTON_6_PIN     28

// oled 一行装下的12号字体个数
#define PANEL_WIDTH_FONT_NUM	21
// oled 一列装下的12号字体个数
#define PANEL_HEIGHT_FONT_NUM	5

// 退出键
#define RADIO_EXIT_BUTTON  	BUTTON_0_PIN
// 下一首键
#define RADIO_NEXT_BUTTON  	BUTTON_1_PIN
// 上一首键
#define RADIO_PRE_BUTTON	BUTTON_2_PIN
// 暂停/播放键
#define RADIO_ENTER_BUTTON  BUTTON_3_PIN
// 音量+键
#define RADIO_VOLUP_BUTTON  BUTTON_4_PIN
// 音量-键
#define RADIO_VOLDN_BUTTON  BUTTON_5_PIN
// 菜单(选台)键
#define RADIO_MENU_BUTTON   BUTTON_6_PIN

/* 收集一些mplayer返回的数据
    type = 
    0: ICY info : 播放音乐电台时点歌名  自动返回
    1: AUDIO bitrate: kpbs              指令返回
    2: AUDIO codec: codec               指令返回
    3: AUDIO samples: Hz ch             指令返回
    4: META album: 专辑                 指令返回
    5: META artist: 艺术家              指令返回
    6: META comment: 评论               指令返回
    7: META genre: 流派                 指令返回
    8: META title: 歌名                 指令返回
    9: META track: 声道数               指令返回
    10:META year: 发行年份              指令返回
    11:TIME length: 歌曲总长            指令返回
    12:TIME pos: 歌曲位置               指令返回
*/
struct mplayerRetData
{
	char *streamTitle;
    struct{
        char *bitrate,*codec,*samples;
    }audio;
    struct{
        char *album,*artist,*comment,*genre,*title,*track,*year;
    }meta;
    struct{
        int length,pos;
    }time;
};

#endif
