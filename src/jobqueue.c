/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-29 20:41:59
**/

#include <stdlib.h>

#include "jobqueue.h"

static JobQueue queue;

static pthread_t tid[MAX_THREAD_NUM];

void put_job(void* (*fun)(void *), void *arg)
{
	int8_t seq;
	Job *job;
	pthread_mutex_lock(&queue.lock);
	if (queue.avail[queue.front] < 0)
		pthread_cond_wait(&queue.empty, &queue.lock);
	seq = queue.avail[queue.front];
	job = &queue.base[seq];
	job->fun = fun;
	job->arg = arg;
	queue.avail[queue.front] 	= -1;
	queue.work[queue.front++] = seq;
	if (queue.front == JOB_QUEUE_SIZE) queue.front = 0;
	pthread_mutex_unlock(&queue.lock);
	pthread_cond_signal(&queue.ready);
}

static void* _run_job(void *arg)
{
	int8_t seq;
	Job *job;
	for (;;) {
		pthread_mutex_lock(&queue.lock);
		if (queue.work[queue.work_back] < 0)
			pthread_cond_wait(&queue.ready, &queue.lock);
		if (queue.work[queue.work_back] < 0) {
			pthread_mutex_unlock(&queue.lock);
			continue;
		}
		seq = queue.work[queue.work_back];
		job = &queue.base[seq];
		queue.work[queue.work_back++] = -1;
		if (queue.work_back == JOB_QUEUE_SIZE) queue.work_back = 0;
		pthread_mutex_unlock(&queue.lock);
		job->fun(job->arg);
		pthread_mutex_lock(&queue.lock);
		queue.avail[queue.avail_back++] = seq;
		if (queue.avail_back == JOB_QUEUE_SIZE) queue.avail_back = 0;
		pthread_mutex_unlock(&queue.lock);
		pthread_cond_signal(&queue.empty);
	}
	return (void *)NULL;
}

void clear_job()
{
	pthread_mutex_lock(&queue.lock);
	while (queue.front != queue.avail_back || queue.front != queue.work_back)
		pthread_cond_wait(&queue.empty, &queue.lock);
	pthread_mutex_unlock(&queue.lock);
}

status init_job_queue()
{
	queue.base  = (Job *)calloc(JOB_QUEUE_SIZE, sizeof(Job));
	if (!queue.base)
		warning("初始化工作队列失败 :(");

	queue.front = 0;
	queue.avail_back = 0;
	queue.work_back  = 0;

	for (size_t i = 0; i < JOB_QUEUE_SIZE; ++i) {
		queue.avail[i] = i;
		queue.work[i]  = -1;
	}
	if (pthread_mutex_init(&queue.lock, NULL)) {
		free(queue.base);
		queue.base = NULL;
		warning("工作队列锁初始化失败 :(");
	}
	if (pthread_cond_init(&queue.ready, NULL)) {
		free(queue.base);
		queue.base = NULL;
		warning("工作队列锁初始化失败 :(");
	}
	if (pthread_cond_init(&queue.empty, NULL)) {
		free(queue.base);
		queue.base = NULL;
		warning("工作队列锁初始化失败 :(");
	}

	for (size_t i = 0; i < MAX_THREAD_NUM; ++i) {
		if (pthread_create(&tid[i], NULL, _run_job, (void *)NULL))
			warning("工作线程初始化失败 :(\n");
		if (pthread_detach(tid[i]))
			warning("工作线程脱离失败 :(\n");
	}
	return Ok;
}

void free_job_queue()
{
	if (queue.base)
		free(queue.base);

	// for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
		// pthread_cancel(tid[i]);
	if (pthread_mutex_destroy(&queue.lock))
		// printf("工作队列锁销毁失败 :(\n");
		;
	if (pthread_cond_destroy(&queue.ready))
		// printf("工作队列条件变量销毁失败 :(\n");
		;
	if (pthread_cond_destroy(&queue.empty))
		// printf("工作队列条件变量销毁失败 :(\n");
		;
}
