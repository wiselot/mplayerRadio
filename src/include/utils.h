#ifndef _UTILS_H
#define _UTILS_H

// 默认数据存储位置
#define UTILS_DEFAULT_MENU_DB   "/home/wiselot/mplayerRadioData.db"

/* 数据库初始化语句 */
// sql语句大小
#define MAX_SQL_CMD_STRLEN			1024
// 活动记录表
#define DB_INIT_TABLE_ACTION_HISTORY "CREATE TABLE \"actionHistory\" ( \
	                                \"id\"	INTEGER NOT NULL, \
	                                \"type\"	INTEGER NOT NULL, \
	                                \"details\"	TEXT, \
	                                \"time\"	TEXT NOT NULL, \
	                                PRIMARY KEY(\"id\" AUTOINCREMENT) \
                                    );"
// 歌曲历史表
#define DB_INIT_TABLE_STREAM_HISTORY	"CREATE TABLE \"streamHistory\" (	\
										\"id\"	INTEGER NOT NULL,	\
										\"url\"	TEXT NOT NULL,	\
										\"details\"	TEXT,	\
										\"time\"	TEXT NOT NULL,	\
										PRIMARY KEY(\"id\" AUTOINCREMENT)	\
									);"
// 表actionHistory已经存在
#define DB_INIT_TABLE_EXISTS_MSG    "table \"actionHistory\" already exists"
// 表初始化数据
#define DB_INIT_TBALE_MSG           "INSERT INTO actionHistory (type,details,time)\
                                    VALUES(%d,'%s','%s')"
#define DB_INIT_TABLE_SH_MSG		"INSERT INTO actionHistory (url,details,time)\
									VALUES('%s','%s','%s')"

// mplayer 启动命令
#define MPLAYER_LANUCH_CMD			"mplayer -idle -quiet -slave"


#endif