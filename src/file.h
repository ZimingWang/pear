/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-23 16:17:35
**/

#ifndef _FILE_H_
#define _FILE_H_

#include "util.h"

#define DATA	".pearD"
#define TABLE ".pearT"

int  new_file(uint32_t, const char *);
int  Open(uint32_t , const char *);
int  Remove(uint32_t , const char *);

#endif /* _FILE_H_ */