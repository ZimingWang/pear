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
#include "thread_pool.h"

DB* newDB(const char *name)
{
	if (mkdir(name, S_IRUSR | S_IWUSR | S_IROTH) < 0)
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

	uint8_t key_len;
	uint16_t type, total;
	get_table_info(db->table, &type, &key_len, &total);

	int8_t (*compare)(const void *, const void *, const uint32_t);

	get_comparator(type, &compare);

	if (init_btree(db->btree, key_len, total, compare) != Ok) {
		free(db->btree);
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

	char *buf = (char *)malloc(db->btree->data_len);
	// char buf[db->btree->data_len];
	if (!verify_attributes(db->table, val, len, count, buf))
		alert("插入数据与表属性不匹配 :(");

	put_job(insert_data, db->btree, buf);
	// insert_data(db->btree, buf);
	return Ok;
}

status drop(DB *db, const void *key, const uint16_t len)
{
	if (!db->table) warning("数据库表不存在 :(");
	if (!verify_key(db->table, len))
		alert("关键值属性错误 :(");

	// char *buf = (char *)malloc(db->btree->key_len);
	// memcpy(buf, key, len);
	// put_job(delete_data, db->btree, buf);
	delete_data(db->btree, key);

	return Ok;
}

status get(const DB *db, const void *key, const uint16_t len)
{
	if (!db->table) warning("数据库表不存在 :(");
	if (!verify_key(db->table, len)) {
		alert("关键值属性错误 :(");
		return Bad;
	}
	// lookup_data(db->btree, key);
	return Ok;
}

status shut(DB *db)
{
	if (db->table)
		free_table(db->table);
	if (db->btree) {
		if (free_btree(db->btree) != Ok)
			return Bad;
	}
	free(db);
	return Ok;
}

uint32_t tuple_number(const DB *db)
{
	return db->btree->tuple;
}
