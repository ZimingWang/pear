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
#include "parser.h"

status init_test(const char *dir)
{
	char file[64];
	strcpy(file, dir);
	strcat(file, "0.pear");
	return parse(file);
}

int main(int argc, char **argv)
{
	if (chdir(".."))
		warning("目录转换失败 :(");

	char dir[64];
	if (!getcwd(dir, 64))
		warning("获取当前目录失败 :(");

	// printf("current directory %s\n", dir);
	strcat(dir, "/pear_syn/");

	init_parser();

	init_test(dir);

	free_parser();
	return 0;
}
