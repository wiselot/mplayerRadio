#ifndef _STORAGE_H
#define _STORAGE_H

#include "utils.h"

// 写入acionHistory类型
typedef enum{
	STARTUP=0,ENDUP=1,
}actionHistoryType;

/**
* @brief            写入actionHistory记录
* @param type       写入类型
* @param details    写入细节
* @param errMsg     返回错误信息,需要手动释放
* @return 返回说明
*       同sqlite3_exec的值
*/
int writeForm_actionHistory(actionHistoryType type,const char *details,char **errMsg);

/** 
 * @brief 写入播放历史
 * @param url       播放url
 * @param details   细节
 * @param errMsg    返回错误信息，需要手动释放
 * 
*/
int writeForm_steamHistory(const char *url,const char *details,char **errMsg);

/** 
 * @brief           storage主线程
 */
void *storage_thread_main(void *arg);

#endif