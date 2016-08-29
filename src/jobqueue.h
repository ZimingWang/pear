/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-29 20:41:36
**/

#ifndef _JOBQUEUE_H_
#define _JOBQUEUE_H_

#include <pthread.h>

#include "util.h"
#include "error.h"

#define  MAX_THREAD_NUM  0x3
#define  JOB_QUEUE_SIZE	 0x8

typedef struct Job
{
	void* (*fun)(void *);
	void* arg;
}Job;

typedef struct
{
	Job *base;										// 队列
	int8_t 	avail[JOB_QUEUE_SIZE];// 空闲位置
	int8_t 	work[JOB_QUEUE_SIZE];	// 占用位置
	int8_t  front;								// 队列头部
	int8_t  avail_back;						// 队列尾部
	int8_t  work_back;						// 队列尾部
	pthread_mutex_t lock;					// 队列锁
	pthread_cond_t  ready;				// 条件变量
	pthread_cond_t  empty;				// 条件变量
}JobQueue;

status init_job_queue();
void put_job(void* (*)(void *), void *);
void clear_job();
void free_job_queue();
void print_job_status();

#endif /* _JOBQUEUE_H_ */