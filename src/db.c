/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:	  https://www.github.com/UncP
 *    > Description:
 *
 *    > Created Time: 2016-08-21 19:33:03
**/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "db.h"

DB* newDB(const char *name)
{
	if (mkdir(name, S_IRUSR | S_IWUSR) < 0)
		warning("数据库已存在于当前目录 :(");
	if (chdir(name) < 0)
		warning("数据库初始化失败 :(");

	DB *db = (DB *)calloc(1, sizeof(DB));
	if (!db) warning("数据库初始化失败 :(");
	strcpy(db->name, name);

	if (!getcwd(db->dir, 64)) {
		free(db);
		warning("无法获取数据库目录 :(");
	}
	// puts(db->name);
	// puts(db->dir);
	return db;
}

status create_table(DB *db, void **name, const uint16_t *len, const uint8_t count)
{
	if (db->table) warning("数据库表已存在 :(");
	db->table = newTable();

	if (!db->table)
		return Bad;

	if (make_table(db->table, name, len, count) != Ok) {
		free_table(db->table);
		db->table = NULL;
		return Bad;
	}

	if (write_table(db->table, &db->table_fd) != Ok) {
		free_table(db->table);
		db->table = NULL;
		return Bad;
	}

	db->btree = newBTree();
	if (!db->btree) return Bad;

	uint8_t type, size;
	uint16_t total;
	get_table_info(db->table, &type, &size, &total);

	int8_t (*compare)(const void *, const void *, const uint32_t);

	get_comparator(type, &compare);

	if (init_btree(db->btree, size, total, compare) != Ok) {
		free_btree(db->btree);
		db->btree = NULL;
		free_table(db->table);
		db->table = NULL;
		return Bad;
	}

	return Ok;
}

status put(DB *db, void **val, const uint16_t *len, const uint8_t count)
{
	if (!db->table) warning("数据库表不存在 :(");
	if (!verify_attributes(db->table, len, count)) {
		alert("插入数据与表属性不匹配 :(");
		return Bad;
	}
	// insert_data(db->btree, val);
	return Ok;
}
