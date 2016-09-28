/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-09-06 17:38:19
**/

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "lock.h"

LockHashTable hashtable;

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
	if (pthread_rwlock_destroy(&lock->lock))
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

	if (pthread_rwlock_init(&lock->lock, NULL))
		warning("锁互斥量初始化失败 :(");

	lock->id = id;
	lock->shared = 0;
	lock->next = NULL;

	return lock;
}

static status _lock(HashBucket *bucket, uint64_t id, lock_status mode)
{
	pthread_mutex_lock(&bucket->lock);
	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id) break;
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
			lock = bucket->head;
			while (lock) {
				if (!lock->shared) break;
				lock = lock->next;
			}
		}
	}
	assert(lock);
	pthread_mutex_unlock(&bucket->lock);
	if (mode == WRITE) {
		pthread_rwlock_wrlock(&lock->lock);
		lock->shared = WRITE;
	} else {
		pthread_rwlock_rdlock(&lock->lock);
		lock->shared = READ;
	}
	if (lock->id != id)
		lock->id = id;

	return Ok;
}

status lock(const void *ptr, lock_status mode)
{
	uint64_t id = (uint64_t)ptr;
	uint8_t bucket_index = _hash(id);
	return _lock(&hashtable.entry[bucket_index], id, mode);
}

static status _unlock(HashBucket *bucket, uint64_t id, lock_status mode)
{
	pthread_mutex_lock(&bucket->lock);
	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id)
			break;
		lock = lock->next;
	}
	if (!lock) {
		alert("解锁 %ld 模式: %s 失败 :(", id, mode == READ ? "read" : "write");
		print_hash_lock_table_status(NULL);
		exit(-1);
	}
	pthread_mutex_unlock(&bucket->lock);
	lock->shared = 0;
	pthread_rwlock_unlock(&lock->lock);
	return Ok;
}

status unlock(const void *ptr, lock_status mode)
{
	uint64_t id = (uint64_t)ptr;
	uint8_t bucket_index = _hash(id);
	return _unlock(&hashtable.entry[bucket_index], id, mode);
}

static status _upgrade(HashBucket *bucket, uint64_t id)
{
	pthread_mutex_lock(&bucket->lock);
	Lock *lock = bucket->head;
	while (lock) {
		if (lock->id == id)
			break;
		lock = lock->next;
	}
	if (!lock) {
		alert("升级锁 %ld 失败 :(", id);
		print_hash_lock_table_status(NULL);
		exit(-1);
	}
	pthread_mutex_unlock(&bucket->lock);
	pthread_rwlock_unlock(&lock->lock);
	pthread_rwlock_wrlock(&lock->lock);
	lock->shared = WRITE;
	return Ok;
}

status upgrade(const void *ptr)
{
	uint64_t id = (uint64_t)ptr;
	uint8_t bucket_index = _hash(id);
	return _upgrade(&hashtable.entry[bucket_index], id);
}

static void _print_hash_bucket_status(const HashBucket *bucket)
{
	Lock *lock = bucket->head;
	printf("len %d\n", bucket->len);
	while (lock) {
		printf("id = %ld  shared %d\n", lock->id, lock->shared);
		lock = lock->next;
	}
}

void* print_hash_lock_table_status(void *arg)
{
	for (uint8_t i = 0; i != TABLE_SIZE; ++i) {
		if (hashtable.entry[i].len) {
			printf("idx %d ", i);
			_print_hash_bucket_status(&hashtable.entry[i]);
		}
	}
	return NULL;
}
