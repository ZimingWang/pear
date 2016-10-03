/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-21 21:24:33
**/

#include <string.h>
#include <assert.h>

#include "util.h"

inline int8_t compare_str(const void *s1, const void *s2, const uint32_t len)
{
	return (int8_t)strncmp((const char *)s1, (const char *)s2, len);
}

void get_comparator(const uint16_t type,
										int8_t (**fun)(const void *, const void *, const uint32_t))
{
	assert(type == STR);
	*fun = compare_str;
}
