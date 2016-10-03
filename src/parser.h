/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 15:40:57
**/

#ifndef _PARSER_H_
#define _PARSER_H_

#include "error.h"
#include "db.h"

#define  BUFFER_SIZE  8192

typedef struct
{
	uint8_t 	   	op;
	const void  **ele;
	uint16_t   	 *len;
	uint8_t   	  count;
}Token;

typedef struct
{
	void   		*ptr;
	void 			*mem;
	uint16_t 	 len;
}Buffer;

typedef struct
{
	DB 			 *db;
	Buffer 		buffer;
	Token			token;
	int 			fd;
	uint32_t 	processed;
	uint32_t  line;
}Parser;

status init_parser();
void 	 free_parser();
status parse(const char *file);

#endif /* _PARSER_H_ */