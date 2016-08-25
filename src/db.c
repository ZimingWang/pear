/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:	  https://www.github.com/UncP/pear
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
	return db;
}

status create_table(DB *db, const void **name, const uint16_t *len, const uint8_t count)
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

	uint8_t type, key_len;
	uint16_t total;
	get_table_info(db->table, &type, &key_len, &total);

	int8_t (*compare)(const void *, const void *, const uint32_t);

	get_comparator(type, &compare);

	if (init_btree(db->btree, key_len, total, compare) != Ok) {
		free_btree(db->btree);
		db->btree = NULL;
		free_table(db->table);
		db->table = NULL;
		return Bad;
	}
	return Ok;
}

status put(DB *db, const void **val, const uint16_t *len, const uint8_t count)
{
	if (!db->table) warning("数据库表不存在 :(");
	char buf[db->btree->data_len];
	if (!verify_attributes(db->table, val, len, count, buf)) {
		alert("插入数据与表属性不匹配 :(");
		return Bad;
	}
	if (insert_data(db->btree, buf))
		++db->tuple;
	return Ok;
}

status drop(DB *db, const void *key, const uint16_t len)
{
	if (!db->table) warning("数据库表不存在 :(");
	if (!verify_key(db->table, len)) {
		alert("关键值属性错误 :(");
		return Bad;
	}
	if (delete_data(db->btree, key))
		--db->tuple;
	else {
		printf("%d\n", db->tuple);
		getchar();
	}
	return Ok;
}

status get(const DB *db, const void *key, const uint16_t len)
{
	if (!db->table) warning("数据库表不存在 :(");
	if (!verify_key(db->table, len)) {
		alert("关键值属性错误 :(");
		return Bad;
	}
	char buf[len];
	memcpy(buf, key, len);
	// lookup_data(db->btree, key);
	return Ok;
}

status shut(DB *db)
{
	if (db->table)
		free_table(db->table);
	if (db->btree)
		if (free_btree(db->btree) != Ok)
			return Bad;
	free(db);
	return Ok;
}
