/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Link:     https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 16:14:43
**/


#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdio.h>
#include <stdint.h>

typedef int8_t status;

#define  Ok    			1
#define  Warning 		0
#define  Fatal     -1

#define warning(arg, ...)	{													\
	fprintf(stderr, "warning:" arg "\n%s 	%s : %d\n", 					\
	##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); \
	return Warning;																		\
}

#define fatal(arg, ...) { 													\
	fprintf(stderr, "fatal:" arg "\n%s 	%s : %d\n",						\
	##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); \
	return Fatal;																			\
}

#endif /* _ERROR_H_ */