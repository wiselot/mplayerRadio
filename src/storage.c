#include "utils.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sqlite3.h> 

static sqlite3 *data_db;
static char sql_cmd[MAX_SQL_CMD_STRLEN];
static char sql_local_time[30];

static int result_callback(void *data, int argc, char **argv, char **azColName);
static int getFormatedTime(char *Time);

/** 
 * @brief               sqlite回调函数
 */
static int result_callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

/** 
 * @brief           获取当前时间格式化输出
 * @param Time      time
 *
 * @return 返回说明
 *     1 fail
 *     0 succeed
 */
static int getFormatedTime(char *Time)
{
    if(!Time)   return 1;
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);
    sprintf(Time,"%4d-%02d-%02d-%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    return 0;
}

/**
* @brief            写入actionHistory记录
* @param type       写入类型
* @param details    写入细节
* @param errMsg     返回错误信息,需要手动释放
* @return 返回说明
*       同sqlite3_exec的值
*/
int writeForm_actionHistory(actionHistoryType type,const char *details,char **errMsg)
{
    getFormatedTime(sql_local_time);
    sprintf(sql_cmd,DB_INIT_TBALE_MSG,type,details?details:"(null)",sql_local_time);
    return sqlite3_exec(data_db,sql_cmd,result_callback,NULL,errMsg);
}

/** 
 * @brief 写入播放历史
 * @param url       播放url
 * @param details   细节
 * @param errMsg    返回错误信息，需要手动释放
 * 
*/
int writeForm_steamHistory(const char *url,const char *details,char **errMsg)
{
    getFormatedTime(sql_local_time);
    sprintf(sql_cmd,DB_INIT_TABLE_SH_MSG,url,details?details:"(null)",sql_local_time);
    return sqlite3_exec(data_db,sql_cmd,result_callback,NULL,errMsg);
}

/** 
 * @brief           storage主线程
 */
void *storage_thread_main(void *arg)
{
    // 打开数据库
    if(sqlite3_open(UTILS_DEFAULT_MENU_DB,&data_db)!=SQLITE_OK){
        fprintf(stderr,"Storage open or created data db failed!\n");
        pthread_exit(NULL);
    }

    // 初始化
    char *errMsg = 0;
    if(sqlite3_exec(data_db,DB_INIT_TABLE_ACTION_HISTORY,result_callback,NULL,&errMsg)!=SQLITE_OK ||
        sqlite3_exec(data_db,DB_INIT_TABLE_STREAM_HISTORY,result_callback,NULL,&errMsg)!=SQLITE_OK){
        if(errMsg && !strcmp(errMsg,DB_INIT_TABLE_EXISTS_MSG))
            printf("Storage already inited.\n");
        else{
            fprintf(stderr,"Storage can't init the data db!(Get from sqlite:%s)\n",errMsg?errMsg:"null");
            pthread_exit(NULL);
        }
    }
    
    // 写入记录
    if(writeForm_actionHistory(STARTUP,"init",&errMsg)!=SQLITE_OK){
        fprintf(stderr,"Storage write startup info failed!(Code:%s)\n",errMsg?errMsg:"(null)");
        pthread_exit(NULL);
    }
    
}