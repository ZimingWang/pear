/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-21 21:13:11
**/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "btree.h"

BTree* newBTree()
{
	BTree *btree = calloc(1, sizeof(BTree));
	if (!btree) warning("B+树创建失败 :(");
	return btree;
}

inline BNode* newBNode(BTinfo *info, uint8_t tag)
{
	void *mem;
	mem = calloc(1, (tag == LEAF) ? (info->bnode_size + info->leaf_size) :
																	(info->bnode_size + info->node_size));

	if (!mem)
		warning("B+树结点初始化失败 :(");

	BNode *bnode = (BNode *)mem;
	bnode->tag 	 = tag;
	bnode->key 	 = (char *)(mem + info->bnode_size);

	if (tag == NODE)
		bnode->child = (BNode **)(mem + info->bnode_size + info->leaf_size);

	return bnode;
}

#ifdef DEBUG
static void _print_btree_info(const BTinfo *info)
{
	char buf[1024];
	sprintf(buf, "N = %d\n", info->n);
	sprintf(buf + strlen(buf), "key_len    = %d\n", info->key_len);
	sprintf(buf + strlen(buf), "leaf_key   = %d\n", info->leaf_key);
	sprintf(buf + strlen(buf), "node_key   = %d\n", info->node_key);
	sprintf(buf + strlen(buf), "max_node   = %d\n", info->max_node);
	sprintf(buf + strlen(buf), "min_key    = %d\n", info->min_key);
	sprintf(buf + strlen(buf), "max_key    = %d\n", info->max_key);
	sprintf(buf + strlen(buf), "min_node   = %d\n", info->min_node);
	sprintf(buf + strlen(buf), "leaf_size  = %d\n", info->leaf_size);
	sprintf(buf + strlen(buf), "node_size  = %d\n", info->node_size);
	sprintf(buf + strlen(buf), "bnode_size = %d\n", info->bnode_size);
	printf("%s", buf);
}
#endif

static void _make_tree_info(BTinfo *info, uint16_t key_len, uint16_t total)
{
	assert(total < 81);
	info->page_size = 4096;
	info->n = ((info->page_size - 1) / (total + 1)) + 1;
	assert(key_len < 256);
	info->key_len  = key_len;
	info->leaf_key = ((info->n - 1) + ((info->n - 1) % 2)) >> 1;
	info->node_key = info->leaf_key + 1;
	info->max_node = info->n + 1;

	info->min_key  = info->leaf_key;
	info->max_key  = info->n - 1;

	info->min_node = (info->n + (info->n % 2)) >> 1;

	info->leaf_size  = (uint16_t)info->n * info->key_len;
	info->node_size  = info->leaf_size + info->max_node * sizeof(BNode *);
	info->bnode_size = sizeof(BNode);
	// _print_btree_info(info);
}

status init_btree(BTree 	*btree,
									uint16_t key_len,
									uint16_t total,
									int8_t (*compare)(const void *, const void *, const uint32_t))
{
	_make_tree_info(&btree->info, key_len, total);
	btree->root = newBNode(&btree->info, LEAF);
	if (!btree->root)
		return Bad;
	btree->compare = compare;
	assert(btree->compare("112345", "112346", 6) < 0);
	assert(btree->compare("123", "123", 3) == 0);

	return Ok;
}

void free_btree(BTree *btree)
{
	free(btree);
}
