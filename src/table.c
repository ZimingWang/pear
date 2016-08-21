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

#include "table.h"
#include "error.h"

void _reset_table(Table *table)
{
	table->size = 0;
}

status init_table(Table *table)
{
	table->attribute = calloc(MAX_ATTRIBUTE_SIZE, sizeof(Attribute));
	if (!table->attribute) fatal("表初始化失败 :(");
	table->size = 0;
	return Ok;
}

void free_table(Table *table)
{
	if (table->attribute)
		free(table->attribute);
	_reset_table(table);
}

static status _append_attribute(Table *table, const char *name, const uint16_t len)
{
	if (table->size == MAX_ATTRIBUTE_SIZE)
		warning("表属性数量到达允许数量 %d", MAX_ATTRIBUTE_SIZE);

	if (strlen(name) >= ATTRIBUTE_NAME_LEN)
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

status make_table(Table *table, const uint8_t **name, const uint16_t *len, const uint8_t count)
{
	for (uint8_t i = 0; i != count; ++i) {
		if (_append_attribute(table, (char *)name[i], len[i]) != Ok) {
			_reset_table(table);
			warning("表建立失败 :(");
		}
	}
	return Ok;
}

bool verify_attributes(const Table *table, const uint16_t *len, const uint8_t count)
{
	if (table->size != count) return false;
	for (uint8_t i = 0; i != count; ++i)
		if (len[i] > table->attribute[i].len)
			return false;
	return true;
}
