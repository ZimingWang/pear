/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 16:25:27
**/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

typedef uint8_t bool;

#define  true   1
#define  false  0

#define  STR  0
#define  INT 	1

int8_t compare_str(const void *s1, const void *s2, const uint32_t len);
int8_t compare_int(const void *i1, const void *i2, const uint32_t len);

void get_comparator(const uint8_t type,
										int8_t (**fun)(const void *, const void *, const uint32_t));

#endif /* _UTIL_H_ */