/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Link:     https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-09-03 16:36:51
**/

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>

#include "util.h"
#include "error.h"

#define  THREAD_NUM      4
#define  JOB_QUEUE_SIZE	 64

typedef struct Job
{
	status (*fun)(void *, const void *);
	void* arg1;
	void* arg2;
}Job;

typedef struct
{
	Job             *base;				// 队列
	int8_t 	        *avail; 			// 空闲位置
	int8_t 	        *work;				// 占用位置
	int8_t           front;				// 队列头部
	int8_t           avail_back;	// 队列尾部
	int8_t           work_back;		// 队列尾部
	pthread_t       *threads;			// 工作线程
	pthread_mutex_t  lock;				// 队列锁
	pthread_cond_t   ready;				// 条件变量
	pthread_cond_t   empty;				// 条件变量
	bool             shutdown;		// 是否撤销工作线程
}JobQueue;

status   init_job_queue();
void     put_job(status (*)(void *, const void *), void *, void *);
void     clear_job_queue();
status   free_job_queue();

#endif /* _THREADPOOL_H_ */