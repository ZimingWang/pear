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
	int 		  *data_fd;
	uint32_t   data_file_num;
	int 		  *index_fd;
	uint32_t   index_file_num;
	char 	 	 	 dir[64];
}DB;

DB* newDB(const char *name);
status create_table(DB *db, void **name, const uint16_t *len, const uint8_t count);
status put(DB *db, void **val, const uint16_t *len, const uint8_t count);
status drop(DB *db, void *key);
status get(DB *db, void *key);

#endif /* _DB_H_ */