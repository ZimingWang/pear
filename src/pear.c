/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-21 01:48:27
**/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "parser.h"
#include "thread_pool.h"

status init_test(const char *dir)
{
	char file[64];
	strcpy(file, dir);
	strcat(file, "0.pear");
	return parse(file);
}

void test(const char *dir, int beg, int end)
{
	char file[64], num[8];
	for (int i = beg; i < end; ++i) {
		memset(file, 0, 64);
		strcpy(file, dir);
		sprintf(num, "%d", i);
		strcat(file, num);
		strcat(file, ".pear");
		parse(file);
	}
}

int main(int argc, char **argv)
{
	if (argc < 5)
		warning("缺少参数 :(");

	int beg1 = atoi(argv[1]);
	int end1 = atoi(argv[2]);
	int beg2 = atoi(argv[3]);
	int end2 = atoi(argv[4]);

	if (chdir(".."))
		warning("目录转换失败 :(");

	char dir[64];
	if (!getcwd(dir, 64))
		warning("获取当前目录失败 :(");

	strcat(dir, "/pear_syn/");

	init_job_queue();
	init_parser();

	if (init_test(dir) != Fatal) {
		test(dir, beg1, end1);
		test(dir, beg2, end2);
	}

	free_job_queue();
	free_parser();

	return 0;
}
