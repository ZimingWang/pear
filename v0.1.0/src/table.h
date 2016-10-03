/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 16:04:25
**/

#ifndef _TABLE_H_
#define _TABLE_H_

#include "util.h"
#include "error.h"

#define  MAX_ATTRIBUTE  8

typedef struct
{
	char 			name[32];
	uint16_t	len;
}Attribute;

typedef struct
{
	Attribute  *attribute;
	uint8_t		  attri_num;
}Table;

Table* newTable();
void 	 free_table(Table *table);
status make_table(Table *table, const void **name, const uint16_t *len, const uint8_t count);
bool	 verify_attributes(	const Table *table, const void **val, const uint16_t *len,
													const uint8_t count, void *buf);
bool 	 verify_key(const Table *table, const uint16_t len);
status write_table(const Table *table, int *fd);
void 	 get_table_info(const Table *table, uint16_t *type, uint8_t *len, uint16_t *total);

#endif /* _TABLE_H_ */