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

#include <pthread.h>

#include "error.h"

#define TABLE_SIZE          31
#define MAX_LOCK_PER_QUEUE  8

typedef enum { FREE, LOCK } lock_status;

typedef struct Lock Lock;

struct Lock
{
	uint64_t         id;
	pthread_mutex_t  lock;
	lock_status      status;
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
void   print_hash_lock_table_status();
status lock(const void *ptr);
status unlock(const void *ptr);

#endif /* _LOCK_H_ */