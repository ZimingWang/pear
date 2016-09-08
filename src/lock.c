/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-09-06 17:38:19
**/

#include <stdlib.h>

#include "lock.h"

LockHashTable hashtable;

static int lock_count = 0;

static inline uint8_t _hash(const uint64_t id)
{
	return id % TABLE_SIZE;
}

static status _init_hash_bucket(HashBucket *bucket)
{
	if (pthread_mutex_init(&bucket->lock, NULL))
		warning("哈希桶互斥量初始化失败 :(");

	return Ok;
}

status init_lock_hash_table()
{
	hashtable.entry = (HashBucket *)calloc(TABLE_SIZE, sizeof(HashBucket));
	if (!hashtable.entry)
		warning("锁哈希表初始化失败 :(");

	for (uint8_t i = 0; i != TABLE_SIZE; ++i)
		if (_init_hash_bucket(&hashtable.entry[i]) != Ok) {
			free_lock_hash_table();
			return Bad;
		}
	if (pthread_mutex_init(&hashtable.lock, NULL))
		warning("哈希锁表互斥量初始化失败 :(");
	return Ok;
}

status _free_lock(Lock *lock)
{
	if (pthread_mutex_destroy(&lock->lock))
		warning("锁互斥量初始化失败 :(");
	free(lock);
	return Ok;
}

status _free_hash_bucket(HashBucket *bucket)
{
	Lock *lock = bucket->head;
	while (lock) {
		Lock *next = lock->next;
		_free_lock(lock);
		lock = next;
	}
	if (pthread_mutex_destroy(&bucket->lock))
		warning("哈希桶互斥量销毁失败 :(");
	return Ok;
}

status free_lock_hash_table()
{
	// print_hash_lock_table_status();
	if (hashtable.entry) {
		for (uint8_t i = 0; i != TABLE_SIZE; ++i)
			if (_free_hash_bucket(&hashtable.entry[i]) != Ok)
				return Bad;
		free(hashtable.entry);
	}
	if (pthread_mutex_destroy(&hashtable.lock))
		warning("哈希锁表互斥量销毁失败 :(");

	return Ok;
}

static Lock* newLock(uint64_t id)
{
	Lock *lock = (Lock *)malloc(sizeof(Lock));
	if (!lock) warning("锁初始化失败 :(");

	if (pthread_mutex_init(&lock->lock, NULL))
		warning("锁互斥量初始化失败 :(");

	lock->id = id;
	lock->status = FREE;
	lock->next = NULL;

	return lock;
}

static status _lock(uint64_t id, HashBucket *bucket)
{
	pthread_mutex_lock(&bucket->lock);

	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id)
			break;
		if (lock->status == FREE)
			break;
		lock = lock->next;
	}
	if (!lock) {
		if (!bucket->len) {
			bucket->head = newLock(id);
			if (!bucket->head) return Bad;
			bucket->tail = bucket->head;
			lock = bucket->head;
			++bucket->len;
		} else if (bucket->len < MAX_LOCK_PER_QUEUE) {
			lock = newLock(id);
			if (!lock) return Bad;
			bucket->tail->next = lock;
			bucket->tail = lock;
			++bucket->len;
		} else {
			warning("not implemented :(");
			exit(-1);
		}
	}

	pthread_mutex_unlock(&bucket->lock);
	pthread_mutex_lock(&lock->lock);
	if (lock->id != id) lock->id = id;
	lock->status = LOCK;

	return Ok;
}

status lock(const void *ptr)
{
	uint64_t id = (uint64_t)ptr;
	uint8_t bucket_index = _hash(id);
	pthread_mutex_lock(&hashtable.lock);
	++lock_count;
	pthread_mutex_unlock(&hashtable.lock);
	// printf("lock    id = %ld  index %d\n", id, bucket_index);
	return _lock(id, &hashtable.entry[bucket_index]);
}

static status _unlock(uint64_t id, HashBucket *bucket)
{
	pthread_mutex_lock(&bucket->lock);

	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id && lock->status == LOCK)
			break;
		lock = lock->next;
	}

	if (!lock) {
		alert("解锁 %ld 失败 :(", id);
		print_hash_lock_table_status();
		exit(-1);
	}

	pthread_mutex_unlock(&bucket->lock);
	lock->status = FREE;
	pthread_mutex_unlock(&lock->lock);
	return Ok;
}

status unlock(const void *ptr)
{
	pthread_mutex_lock(&hashtable.lock);
	--lock_count;
	pthread_mutex_unlock(&hashtable.lock);
	uint64_t id = (uint64_t)ptr;
	uint8_t bucket_index = _hash(id);
	// printf("unlock  id = %ld  index %d\n", id, bucket_index);
	return _unlock(id, &hashtable.entry[bucket_index]);
}

static void _print_hash_bucket_status(const HashBucket *bucket)
{
	Lock *lock = bucket->head;
	printf("len %d\n", bucket->len);
	while (lock) {
		printf("id = %ld  status %s\n", lock->id, lock->status == FREE ? "free" : "lock");
		lock = lock->next;
	}
}

void print_hash_lock_table_status()
{
	for (uint8_t i = 0; i != TABLE_SIZE; ++i) {
		if (hashtable.entry[i].len)
			_print_hash_bucket_status(&hashtable.entry[i]);
	}
}

/*
int main()
{
	init_lock_hash_table();
	uint64_t **id = (uint64_t **)malloc(10 * sizeof(uint64_t *));
	for (int i = 0; i != 10; ++i) {
		id[i] = malloc(sizeof(uint64_t));
		lock(id[i]);
	}
	puts("lock");
	lock(id[0]);
	puts("unlock");
	unlock(id[0]);
	print_hash_lock_table_status();
	// for (int i = 0; i != 10; ++i) {
	// 	unlock(id[i]);
	// 	free(id[i]);
	// }
	free(id);
	free_lock_hash_table();
	return 0;
}
*/