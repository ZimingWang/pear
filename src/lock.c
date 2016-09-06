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

static inline uint8_t _hash(const uint64_t id)
{
	return ((id >> 4) & 0xFF) % TABLE_SIZE;
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
	return Ok;
}

status _free_hash_bucket(HashBucket *bucket)
{
	if (pthread_mutex_destroy(&bucket->lock))
		warning("哈希桶互斥量销毁失败 :(");
	return Ok;
}

status free_lock_hash_table()
{
	if (hashtable.entry) {
		for (uint8_t i = 0; i != TABLE_SIZE; ++i)
			if (_free_hash_bucket(&hashtable.entry[i]) != Ok)
				return Bad;
		free(hashtable.entry);
	}
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
	}
	if (!lock) {
		if (!bucket->len) {
			bucket->head = newLock(id);
			if (bucket->head) return Bad;
			bucket->tail = bucket->head;
			lock = bucket->head;
		} else if (bucket->len < MAX_LOCK_PER_QUEUE) {
			bucket->tail->next = newLock(id);
			if (!bucket->tail->next) return Bad;
			bucket->tail = bucket->tail->next;
			lock = bucket->tail;
		} else {
			warning("not implemented :(");
			exit(-1);
		}
	}

	if (lock->id != id) lock->id = id;
	pthread_mutex_lock(&lock->lock);
	lock->status = LOCK;

	pthread_mutex_unlock(&bucket->lock);
	return Ok;
}

status lock(const void *ptr)
{
	uint64_t id = *(uint64_t *)ptr;
	uint8_t bucket_index = _hash(id);
	return _lock(id, &hashtable.entry[bucket_index]);
}

static status _unlock(uint64_t id, HashBucket *bucket)
{
	pthread_mutex_lock(&bucket->lock);

	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id && lock->status == LOCK)
			break;
	}

	if (!lock) warning("解锁失败");

	pthread_mutex_unlock(&lock->lock);
	lock->status = FREE;

	pthread_mutex_unlock(&bucket->lock);
	return Ok;
}

status unlock(const void *ptr)
{
	uint64_t id = *(uint64_t *)ptr;
	uint8_t bucket_index = _hash(id);
	return _unlock(id, &hashtable.entry[bucket_index]);
}
