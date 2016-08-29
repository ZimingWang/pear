/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-21 19:36:05
**/

#ifndef _BTREE_H_
#define _BTREE_H_

#include <pthread.h>

#include "error.h"
#include "page.h"
typedef struct BNode BNode;

#define  NODE  0
#define  LEAF  1

#define  MAX_DEPTH  8

#define  PTR_SIZE  (sizeof(BNode *))

struct BNode
{
	unsigned     tag:1;			// LEAF  NODE
	unsigned   split:1;			// 是否刚刚分裂
	unsigned    lock:2;			// WRITE READ
	uint8_t 		kcount;			// 关键值数量
	uint8_t 		ncount;			// 孩子结点数量 NODE
	uint32_t		index;			// 页面号      LEAF
	union {
		void 				*key;			// 关键值    NODE
		Page 				*page;		// 数据页面  LEAF
	};
	BNode 		 **child;			// NODE  孩子结点
													// LEAF  下一结点
};

typedef struct
{
	BNode  	 *node;
	uint8_t   index;
}Pair;

typedef struct
{
	uint8_t 	   n;								// B+ 树的度
	uint8_t		   key_len;					// 关键值的长度
	uint8_t 	   min_key;					// 叶子结点允许的最少关键值数
	uint8_t		   max_key;					// 结点中允许的最大关键值数
	uint8_t 	   min_node;				// 非叶子结点中最少孩子数
	uint8_t		   max_node;				// 分裂时非叶子结点中存在的孩子结点数

	uint16_t     data_len;				// 数据长度(键+值)

	BNode       *root;
	Pager        pager;

	pthread_mutex_t lock;

	int8_t     (*compare)(const void *, const void *, const uint32_t);
}BTree;

BTree* newBTree();
status free_btree(BTree *btree);
status init_btree(BTree *btree, uint16_t key_len, uint16_t data_len,
									int8_t (*compare)(const void *, const void *, const uint32_t));
status insert_data(BTree *btree, const void *val);
status delete_data(BTree *btree, const void *key);

#endif /* _BTREE_H_ */