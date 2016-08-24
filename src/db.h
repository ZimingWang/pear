/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 15:58:21
**/

#ifndef _DB_H_
#define _DB_H_

#include "error.h"
#include "table.h"
#include "btree.h"

typedef struct
{
	char 			 name[32];
	Table    	*table;
	BTree	  	*btree;
	uint32_t   tuple;
	int 		   table_fd;
	char 	 	 	 dir[64];
}DB;

DB* newDB(const char *name);
status create_table(DB *db, const void **name, const uint16_t *len, const uint8_t count);
status put(DB *db, const void **val, const uint16_t *len, const uint8_t count);
status drop(DB *db, const void *key, const uint16_t len);
status get(const DB *db, const void *key, const uint16_t len);

#endif /* _DB_H_ */