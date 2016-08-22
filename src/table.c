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

	table->attribute = calloc(MAX_ATTRIBUTE_SIZE, sizeof(Attribute));
	if (!table->attribute) {
		free_table(table);
		warning("表初始化失败 :(");
	}

	table->size = 0;
	return table;
}

static status _append_attribute(Table *table, const char *name, const uint16_t len)
{
	if (table->size == MAX_ATTRIBUTE_SIZE)
		warning("表属性数量到达允许数量 %d", MAX_ATTRIBUTE_SIZE);

	if (strlen((char *)name) >= ATTRIBUTE_NAME_LEN)
		warning("表属性名称过长 :(");

	strcpy((char *)table->attribute[table->size].name, name);

	if (len) {
		if (len >= MAX_STR_LEN)
			warning("字符串属性超过允许长度 %d", MAX_STR_LEN);
		table->attribute[table->size].len = len;
	} else {
		table->attribute[table->size].len = 0;
	}

	++table->size;
	return Ok;
}

status make_table(Table *table, void **name, const uint16_t *len, const uint8_t count)
{
	for (uint8_t i = 0; i != count; ++i) {
		if (_append_attribute(table, (const char *)name[i], len[i]) != Ok)
			warning("表建立失败 :(");
	}
	return Ok;
}

status write_table(const Table *table, int *fd)
{
	if (creat("table.pear", S_IRUSR | S_IWUSR) < 0)
		warning("表文件创建失败 :(");
	if ((*fd = open("table.pear", O_WRONLY)) < 0)
		warning("表文件打开失败 :(");
	char buf[1024] = {0};
	for (uint8_t i = 0; i != table->size; ++i) {
		strcat(buf, (char *)table->attribute[i].name);
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

void get_table_info(const Table *table, uint8_t *type, uint8_t *size, uint16_t *total)
{
	assert(table->size > 0 && table->attribute[0].len > 0);
	*type = STR;
	*size = table->attribute[0].len;
	*total = 0;
	for (uint8_t i = 0; i != table->size; ++i)
		*total += (table->attribute[i].len > 0) ? table->attribute[i].len : 4;
}


bool verify_attributes(const Table *table, const uint16_t *len, const uint8_t count)
{
	if (table->size != count) return false;
	for (uint8_t i = 0; i != count; ++i)
		if (len[i] > table->attribute[i].len)
			return false;
	return true;
}
