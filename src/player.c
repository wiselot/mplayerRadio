#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <mpg123.h>
#include <ao/ao.h>
#include <alsa/asoundlib.h>

typedef struct _steam_url
{
    const char * url;
    struct _steam_url *next,*front;
}stream_url;

// mpg123句柄
static mpg123_handle *_mh;
// 播放队列
static stream_url *steam_list;


/*
* @brief 播放流进程
*/
static void *new_stream_player(void *arg){
    // arg空,等待

}

/*
* @brief 播放器主线程
*/
void *player_thread_main(void *arg)
{
    // mpg123 初始化
    if(mpg123_init()!=MPG123_OK){
        fprintf(stderr,"Player init:mpg123 init failed!\n");
        pthread_exit(NULL);
    }

    // mpg123句柄创建
    int mpg123_error;

    if(!(_mh=mpg123_new(NULL,&mpg123_error))){
        fprintf(stderr,"Player init:mpg123 handle init failed(Code:%d)!\n",mpg123_error);
        mpg123_exit();
        pthread_exit(NULL);
    }

    // 播放队列初始化
    steam_list = (stream_url *)malloc(sizeof(stream_url));


}