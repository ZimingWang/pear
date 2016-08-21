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

typedef struct
{
	Table    	*table;
	BTree	  	*btree;
	uint32_t   tuple;
	int 		   table_fd;
	int 		  *data_fd;
	uint32_t   data_file_num;
	int 		  *index_fd;
	uint32_t   index_file_num;
	char 	 	 	*dir;
}DB;

#endif /* _DB_H_ */