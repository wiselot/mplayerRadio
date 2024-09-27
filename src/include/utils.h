#ifndef _UTILS_H
#define _UTILS_H

// 默认数据存储位置
#define UTILS_DEFAULT_MENU_DB   "mplayerRadioData.db"

/* 数据库初始化语句 */
// 活动记录表
#define DB_INIT_TABLE_ACTION_HISTORY "CREATE TABLE \"actionHistory\" ( \
	                                \"id\"	INTEGER NOT NULL, \
	                                \"type\"	INTEGER NOT NULL, \
	                                \"details\"	TEXT, \
	                                \"time\"	TEXT NOT NULL, \
	                                PRIMARY KEY(\"id\") \
                                    );"
// 表已经存在
#define DB_INIT_TABLE_EXISTS_MSG    "table actionHistory already exists"
// 表初始化数据
#define DB_INIT_TBALE_MSG           "INSERT INTO actionHistory (id,type,details,time)\
                                    VALUES(0,0,'init','"



#endif