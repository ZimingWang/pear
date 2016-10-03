/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-09-05 17:10:06
**/

#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef __USE_UNIX98
#define __USE_UNIX98 1
#endif

#include <pthread.h>

#include "error.h"

#define TABLE_SIZE          31
#define MAX_LOCK_PER_QUEUE  8

typedef enum { FREE, WRITE, READ } lock_status;

typedef struct Lock Lock;

struct Lock
{
	uint64_t         id;
	pthread_rwlock_t lock;
	uint8_t         shared;
	Lock            *next;
};

typedef struct
{
	pthread_mutex_t  lock;
	Lock            *head;
	Lock            *tail;
	uint8_t          len;
}HashBucket;

typedef struct
{
	HashBucket     *entry;
	pthread_mutex_t lock;
}LockHashTable;

status init_lock_hash_table();
status free_lock_hash_table();
void*   print_hash_lock_table_status(void *arg);
status lock(const void *ptr, lock_status mode);
status unlock(const void *ptr, lock_status mode);
status upgrade(const void *ptr);

#endif /* _LOCK_H_ */