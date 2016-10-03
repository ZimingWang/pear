/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Link:     https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-09-03 16:36:51
**/

#include <stdlib.h>
#include <unistd.h>

#include "thread_pool.h"

static JobQueue queue;

static void* _run_job(void *);

status init_job_queue()
{
	queue.base  = (Job *)calloc(JOB_QUEUE_SIZE, sizeof(Job));
	if (!queue.base)
		warning("初始化工作队列失败 :(");

	queue.front = 0;
	queue.avail_back = 0;
	queue.work_back  = 0;

	queue.avail = (int8_t *)malloc(JOB_QUEUE_SIZE * sizeof(int8_t));
	queue.work  = (int8_t *)malloc(JOB_QUEUE_SIZE * sizeof(int8_t));
	if (!queue.avail || !queue.work)
		warning("工作队列参数初始化失败 :(");

	for (uint8_t i = 0; i < JOB_QUEUE_SIZE; ++i) {
		queue.avail[i] = i;
		queue.work[i]  = -1;
	}

	if (pthread_mutex_init(&queue.lock, NULL))
		warning("工作队列锁初始化失败 :(");
	if (pthread_cond_init(&queue.ready, NULL))
		warning("工作队列条件变量初始化失败 :(");
	if (pthread_cond_init(&queue.empty, NULL))
		warning("工作队列条件变量初始化失败 :(");

	queue.threads = (pthread_t *)malloc(THREAD_NUM * sizeof(pthread_t));
	if (!queue.threads)
		warning("工作线程ID初始化失败 :(");

	for (uint8_t i = 0; i < THREAD_NUM; ++i)
		if (pthread_create(&queue.threads[i], NULL, _run_job, (void *)NULL))
			warning("工作线程初始化失败 :(");

	queue.shutdown = false;

	return Ok;
}

status free_job_queue()
{
	clear_job_queue();

	queue.shutdown = true;
	pthread_cond_broadcast(&queue.ready);

	if (queue.base)  free(queue.base);
	if (queue.avail) free(queue.avail);
	if (queue.work)  free(queue.work);

	for (uint8_t i = 0; i < THREAD_NUM; ++i)
		if (pthread_join(queue.threads[i], NULL))
			warning("回收线程失败 :(");

	if (pthread_mutex_destroy(&queue.lock))
		warning("工作队列锁销毁失败 :(");
	if (pthread_cond_destroy(&queue.ready))
		warning("工作队列条件变量销毁失败 :(");
	if (pthread_cond_destroy(&queue.empty))
		warning("工作队列条件变量销毁失败 :(");

	return Ok;
}

void clear_job_queue()
{
	pthread_mutex_lock(&queue.lock);
	while (queue.front != queue.avail_back || queue.front != queue.work_back)
		pthread_cond_wait(&queue.empty, &queue.lock);
	pthread_mutex_unlock(&queue.lock);
}

static void* _run_job(void *arg)
{
	int8_t seq;
	Job *job;
	for (;;) {
		pthread_mutex_lock(&queue.lock);
		if (queue.work[queue.work_back] < 0)
			pthread_cond_wait(&queue.ready, &queue.lock);

		if (queue.shutdown) {
			pthread_mutex_unlock(&queue.lock);
			break;
		}

		if (queue.work[queue.work_back] < 0) {
			pthread_mutex_unlock(&queue.lock);
			continue;
		}
		seq = queue.work[queue.work_back];
		job = &queue.base[seq];
		queue.work[queue.work_back++] = -1;
		if (queue.work_back == JOB_QUEUE_SIZE) queue.work_back = 0;
		pthread_mutex_unlock(&queue.lock);

		job->fun(job->arg1, job->arg2);
		free(job->arg2);

		pthread_mutex_lock(&queue.lock);
		queue.avail[queue.avail_back++] = seq;
		if (queue.avail_back == JOB_QUEUE_SIZE) queue.avail_back = 0;
		pthread_mutex_unlock(&queue.lock);
		if (queue.avail_back == queue.work_back)
			pthread_cond_signal(&queue.empty);
	}
	return (void *)NULL;
}

void put_job(status (*fun)(void *, const void *), void *arg1, void *arg2)
{
	pthread_mutex_lock(&queue.lock);
	while (queue.avail[queue.front] < 0)
		pthread_cond_wait(&queue.empty, &queue.lock);
	int8_t seq = queue.avail[queue.front];
	Job *job = &queue.base[seq];
	job->fun  = fun;
	job->arg1 = arg1;
	job->arg2 = arg2;
	queue.avail[queue.front] 	= -1;
	queue.work[queue.front++] = seq;
	if (queue.front == JOB_QUEUE_SIZE) queue.front = 0;
	pthread_mutex_unlock(&queue.lock);
	pthread_cond_signal(&queue.ready);
}
