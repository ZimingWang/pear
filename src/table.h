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

#define  MAX_ATTRIBUTE_SIZE  8
#define  ATTRIBUTE_NAME_LEN  32
#define	 MAX_STR_LEN				 4096

typedef struct
{
	uint8_t 	name[ATTRIBUTE_NAME_LEN];
	uint16_t 	len;
}Attribute;

typedef struct
{
	Attribute *attribute;
	uint8_t		 size;
}Table;

Table* newTable();
void 	 free_table(Table *table);
status make_table(Table *table, void **name, const uint16_t *len, const uint8_t count);
bool	 verify_attributes(const Table *table, const uint16_t *len, const uint8_t count);
status write_table(const Table *table, int *fd);
void 	 get_table_info(const Table *table, uint8_t *type, uint8_t *size, uint16_t *total);

#endif /* _TABLE_H_ */