/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-23 16:18:42
**/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

#include "error.h"
#include "file.h"

static void _make_file_name(char *buf, uint32_t seq, const char *type)
{
	char num[10] = {0};
	sprintf(num, "%d", seq);
	strcpy(buf, num);
	strcat(buf, type);
}

int Open(uint32_t seq, const char *type)
{
	char buf[16];
	_make_file_name(buf, seq, type);
	if (access(buf, F_OK)) {
		alert("文件 %s 不存在 :(", buf);
		return -1;
	}
	return open(buf, O_RDWR);
}

int Remove(uint32_t seq, const char *type)
{
	char buf[16];
	_make_file_name(buf, seq, type);
	if (access(buf, F_OK)) {
		alert("文件 %s 不存在 :(", buf);
		return -1;
	}
	return remove(buf);
}

int new_file(uint32_t seq, const char *type)
{
	char buf[16];
	_make_file_name(buf, seq, type);
	if (!access(buf, F_OK)) {
		if (!truncate64(buf, 0)) {
			alert("文件 %s 截断失败 :(", buf);
			return -1;
		}
	} else if (creat(buf, O_RDWR) < 0) {
		alert("文件 %s 不存在 :(", buf);
		return -1;
	}
	return open(buf, O_RDWR);
}
