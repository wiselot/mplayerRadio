#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h> 

static int start_storage(sqlite3 *db,char *msg);
static int close_storage(sqlite3 *db,char *msg);
static int result_callback(void *data, int argc, char **argv, char **azColName);
static int init_storage(sqlite3 *db,char *msg);
static int getFormatedTime(char *Time);


/** 
 * @brief           打开存储库
 * @param db        数据库句柄
 * @param msg       错误信息存储
 *
 * @return 返回说明
 *     1 fail
 *     0 succeed
 */
static int start_storage(sqlite3 *db,char *msg)
{
    if(!db) return 1;
    if(sqlite3_open(UTILS_DEFAULT_MENU_DB,&db)!=SQLITE_OK){
        if(msg) strcpy(msg,sqlite3_errmsg(db));
        return 1;
    }
    return 0;
}

/** 
 * @brief           关闭存储库
 * @param db        数据库句柄
 * @param msg       错误信息存储
 *
 * @return 返回说明
 *     1 fail
 *     0 succeed
 */
static int close_storage(sqlite3 *db,char *msg)
{
    if(!db) return 1;
    if(sqlite3_close(db)!=SQLITE_OK){
        if(msg) strcpy(msg,sqlite3_errmsg(db));
        return 1;
    }
    return 0;
}


/** 
 * @brief               sqlite回调函数
 */
static int result_callback(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s: ", (const char*)data);
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
 * @brief           初始化存储库
 * @param db        数据库句柄
 * @param msg       错误信息存储
 *
 * @return 返回说明
 *     1 fail
 *     0 succeed
 */
static int init_storage(sqlite3 *db,char *msg)
{
    if(!db) return 1;
    int ret = 0;
    // 活动表
    ret |= sqlite3_exec(db,DB_INIT_TABLE_ACTION_HISTORY,result_callback,0,&msg);
    if(ret && strcmp(msg,DB_INIT_TABLE_EXISTS_MSG))
    {
        // 表不存在而且创建失败
        return 1;
    }
    char cmd[128];
    char time[30];
    if(getFormatedTime(time)){
        strcpy(time,"2024-1-1-00:00:00");
    }
    sprintf(cmd,"%s%s')",DB_INIT_TBALE_MSG,time);
    ret |= sqlite3_exec(db,cmd,result_callback,0,&msg);
    if(ret) return 1;
    
    return 0;
}

