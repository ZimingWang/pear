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

#include "error.h"

typedef struct BNode BNode;

#define  NODE  0
#define  LEAF  1

struct BNode
{
	unsigned 			 tag:8;				// LEAF NODE
	unsigned		kcount:8;				// 关键值数量
	union {
		unsigned  ncount:8;				// 孩子结点数量
		unsigned  index:24;				// 页面位置
	};
	void 					*key;					// 关键值
	BNode 			**child;				// 若为叶子结点, 结点为下一叶结点, 若为内部结点, 指向孩子结点

};

typedef struct
{
	uint8_t 	 n;								// B+ 树的度
	uint8_t		 key_len;					// 关键值的长度
	uint8_t		 leaf_key;				// 结点分裂时左叶子结点的最小关键值数
	uint8_t		 node_key;				// 结点分裂时左非叶子结点的最小孩子数, 右非叶子结点关键值的起始序号
	uint8_t		 max_node;				// 分裂时非叶子结点中存在的孩子结点数
	uint8_t 	 min_key;					// 叶子结点允许的最少关键值数
	uint8_t		 max_key;					// 结点中允许的最大关键值数
	uint8_t 	 min_node;				// 非叶子结点中允许的最少孩子数
	uint16_t 	 leaf_size;				// 叶子结点的字节数
	uint16_t 	 node_size;				// 非叶子结点的字节数
	uint16_t 	 bnode_size;			// B+树结点的字节数
	uint16_t	 page_size;				// 页面大小
}BTinfo;

typedef struct
{
	BNode    *root;
	BTinfo 	 	info;
	int8_t		(*compare)(const void *, const void *, const uint32_t);
}BTree;

BTree* newBTree();
void 	 free_btree(BTree *btree);
status init_btree(BTree *btree, uint16_t key_len, uint16_t total,
									int8_t (*compare)(const void *, const void *, const uint32_t));

#endif /* _BTREE_H_ */