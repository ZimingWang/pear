/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 16:08:01
**/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "table.h"
#include "error.h"

void free_table(Table *table)
{
	if (table->attribute)
		free(table->attribute);
	free(table);
}

Table* newTable()
{
	Table *table = (Table *)calloc(1, sizeof(Table));
	if (!table)
		warning("表初始化失败 :(");

	table->attribute = (Attribute *)calloc(MAX_ATTRIBUTE, sizeof(Attribute));
	if (!table->attribute) {
		free_table(table);
		warning("表初始化失败 :(");
	}

	return table;
}

status make_table(Table *table, const void **name, const uint16_t *len, const uint8_t count)
{
	for (uint8_t i = 0; i != count; ++i) {
		strcpy(table->attribute[i].name, (char *)name[i]);
		table->attribute[i].len = len[i];
		++table->attri_num;
	}
	return Ok;
}

status write_table(const Table *table, int *fd)
{
	if (creat("table.pear", S_IRUSR | S_IWUSR) < 0)
		warning("表文件创建失败 :(");
	if ((*fd = open("table.pear", O_WRONLY)) < 0)
		warning("表文件打开失败 :(");
	char buf[512] = {0};
	for (uint8_t i = 0; i != table->attri_num; ++i) {
		strcat(buf, table->attribute[i].name);
		if (table->attribute[i].len > 0) {
			strcat(buf, " STR ");
			char c[8];
			sprintf(c, "%d", table->attribute[i].len);
			strcat(c, "\n\0");
			strcat(buf, c);
		} else {
			strcat(buf, "\n");
		}
	}
	if (write(*fd, buf, strlen(buf)) < 0)
		warning("表文件写入失败 :(");
	return Ok;
}

void get_table_info(const Table *table, uint16_t *type, uint8_t *len, uint16_t *total)
{
	assert(table->attri_num);
	*type = STR;
	*len = table->attribute[0].len;
	*total = 0;
	for (uint8_t i = 0; i != table->attri_num; ++i)
		*total += table->attribute[i].len;
}

bool verify_attributes(const Table *table, const void **val, const uint16_t *len,
	const uint8_t count, void *buf)
{
	if (table->attri_num != count) return false;
	uint16_t offset = 0;
	for (uint8_t i = 0; i != count; ++i) {
		if (len[i] > table->attribute[i].len)
			return false;
		memcpy(buf + offset, val[i], len[i]);
		offset += table->attribute[i].len;
	}
	return true;
}

bool verify_key(const Table *table, const uint16_t len)
{
	assert(table->attri_num);
	return table->attribute[0].len > len;
}
