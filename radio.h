#ifndef _RADIO_H
#define _RADIO_H

#define MPLAYER_RADIO_TIP					"MplayerRadio"
#define MPLAYER_RADIO_WAIT_SEC		3

#define MPLAYER_RADIO_RES_UPDATE_PATH	"/home/wiselot/Radio/mplayer-radio-res"
#define MPLAYER_RADIO_RES_LIST_PATH     "/home/wiselot/Radio/mplayer-radio-res/radio_list.txt"
#define MPLAYER_RADIO_RES_UPDATE_SITE	"https://gitee.com/wiselot/mplayer-radio-res.git"

#define BUTTON_0_PIN     21
#define BUTTON_1_PIN     22
#define BUTTON_2_PIN     23
#define BUTTON_3_PIN     24
#define BUTTON_4_PIN     25
#define BUTTON_5_PIN     26

// 退出
#define RADIO_EXIT_BUTTON  	BUTTON_0_PIN
// 下一首
#define RADIO_NEXT_BUTTON  	BUTTON_1_PIN
// 上一首
#define RADIO_PRE_BUTTON	BUTTON_2_PIN
// 暂停/播放
#define RADIO_ENTER_BUTTON  BUTTON_3_PIN
// 音量+
#define RADIO_VOLUP_BUTTON  BUTTON_4_PIN
// 音量-
#define RADIO_VOLDN_BUTTON  BUTTON_5_PIN

#endif