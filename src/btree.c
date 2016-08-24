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

#define PTR(ptr, index, len) (((void *)ptr) + (((uint32_t)index) * len))
#define LEN(beg, end, len) 	 ((((uint32_t)end) - beg) * len)
#define KEY(leaf)						 (*((uint8_t *)(leaf->page->data + 0)))

BTree* newBTree()
{
	BTree *btree = calloc(1, sizeof(BTree));
	if (!btree) warning("B+树创建失败 :(");
	return btree;
}

static BNode* newBNode(BTree *btree, uint8_t tag)
{
	BNode *bnode;
	if (tag == LEAF) {
		bnode = (BNode *)calloc(1, sizeof(BNode));
		if (!bnode) warning("B+树结点初始化失败 :(");
		bnode->page = fresh_page(&btree->pager);
		if (!bnode->page) {
			free(bnode);
			return NULL;
		}
		bnode->index = bnode->page->index;
	} else {
		uint32_t bnode_size = sizeof(BNode);
		uint32_t key_size   = (uint32_t)btree->n * btree->key_len;
		uint32_t child_size = (uint32_t)btree->max_node * sizeof(BNode *);
		void *mem = calloc(1, bnode_size + key_size + child_size);
		if (!mem) warning("B+树结点初始化失败 :(");
		bnode = (BNode *)mem;
		bnode->key 	 = (char *)(mem + bnode_size);
		bnode->child = (BNode **)(mem + bnode_size + key_size);
	}

	bnode->tag 	 = tag;

	return bnode;
}

#ifdef DEBUG
static void _print_btree_info(const Btree *btree)
{
	char buf[512];
	sprintf(buf, "N = %d\n", btree->n);
	sprintf(buf + strlen(buf), "key_len    = %d\n", btree->key_len);
	sprintf(buf + strlen(buf), "min_key    = %d\n", btree->min_key);
	sprintf(buf + strlen(buf), "max_key    = %d\n", btree->max_key);
	sprintf(buf + strlen(buf), "min_node   = %d\n", btree->min_node);
	sprintf(buf + strlen(buf), "max_node   = %d\n", btree->max_node);
	printf("%s", buf);
}
#endif

status init_btree(BTree 	*btree, uint16_t key_len, uint16_t total,
									int8_t (*compare)(const void *, const void *, const uint32_t))
{
	if (init_pager(&btree->pager, 4096) != Ok)
		return Bad;

	btree->n = ((btree->pager.page_size - 1) / (total + 1)) + 1;
	assert(key_len < 256);
	btree->key_len  = key_len;

	btree->min_key  = ((btree->n - 1) + ((btree->n - 1) % 2)) >> 1;
	btree->max_key  = btree->n - 1;

	btree->min_node = (btree->n + (btree->n % 2)) >> 1;
	btree->max_node = btree->n + 1;
	btree->data_len = total;
#ifdef DEBUG
	_print_btree_info(btree);
#endif
	btree->root = newBNode(btree, LEAF);
	if (!btree->root)
		return Bad;
	btree->compare = compare;

	return Ok;
}

void free_btree(BTree *btree)
{
	free(btree);
}

static uint8_t _get_descend_index(const BNode *curr, const void *key, const uint8_t len,
	int8_t (*compare)(const void *, const void *, const uint32_t))
{
	uint8_t low = 0, high = (uint8_t)curr->kcount, mid = (low + high) >> 1;
	while (low != high) {
		int8_t flag = compare(PTR(curr->key, mid, len), key, len);
		if (flag > 0) {
			high = mid;
		} else if (flag < 0) {
			low = mid + 1;
		} else {
			++mid;
			break;
		}
		mid = (low + high) >> 1;
	}
	return mid;
}

static BNode* _find_leaf(BTree *btree, const void *key)
{
	btree->curr_depth = 0;
	BNode *curr = btree->root;
	for (; curr->tag != LEAF;) {
		uint8_t index = _get_descend_index(curr, key, btree->key_len, btree->compare);
		btree->track[btree->curr_depth].node = curr;
		btree->track[btree->curr_depth].index = index;
		++btree->curr_depth;
		curr = curr->child[index];
	}

	return curr;
}

static uint8_t _get_index(const void *data, const uint16_t data_size, const uint8_t offset,
	const void *key, const uint8_t len,
	int8_t (*compare)(const void *, const void *, const uint32_t), bool insert)
{
	uint8_t low = 0, high = *(uint8_t *)(data++ + 0), mid = (high + low) >> 1;
	while (low != high) {
		uint8_t index = *(uint8_t *)(data + mid);
		int8_t flag = compare(PTR(data + offset, index, data_size), key, len);
		if (flag > 0) {
			high = mid;
		} else if (flag < 0) {
			low = mid + 1;
		} else {
			if (!insert) return mid;
			else				 return 0xFF;
		}
		mid = (low + high) >> 1;
	}
	if (insert) return mid;
	else				return 0xFF;
}

static void _insert_child(BNode *bnode, const uint8_t pos, BNode *child)
{
	if (bnode->ncount > pos)
		memmove(PTR(bnode->child, pos+1, 8), PTR(bnode->child, pos, 8), LEN(pos, bnode->ncount, 8));
	bnode->child[pos] = child;
	++bnode->ncount;
}

static void _insert_key(BNode *bnode, const uint8_t pos, const void *key, const uint8_t len)
{
	if (bnode->kcount > pos)
		memmove(PTR(bnode->key, pos+1, len), PTR(bnode->key, pos, len),
						LEN(pos, bnode->kcount, len));
	memcpy(PTR(bnode->key, pos, len), key, len);
	++bnode->kcount;
}

static void _insert_pair(BNode *bnode, const uint8_t pos,
	BNode *child, const void *key, const uint8_t len)
{
	_insert_key(bnode, pos, key, len);
	_insert_child(bnode, pos + 1, child);
}

static void _part_key(BNode *dst, BNode *src, const uint8_t beg, const uint8_t end,
	const uint8_t len)
{
	memcpy(PTR(dst->key, 0, len), PTR(src->key, beg, len), LEN(beg, end, len));
	dst->kcount = end - beg;
	src->kcount = (uint8_t)beg;
}

static void _part_child(BNode *dst, BNode *src, const uint8_t beg, const uint8_t end)
{
	memcpy(PTR(dst->child, 0, 8), PTR(src->child, beg, 8), LEN(beg, end, 8));
	dst->ncount = end - beg;
	src->ncount  = (uint8_t)beg;
}

static void _part_half(BNode *dst, BNode *src, const uint8_t beg, const uint8_t end,
	const uint8_t len)
{
	_part_key(dst, src, beg, end, len);
	_part_child(dst, src, beg, end + 1);
}

static status _insert_fixup(BTree *btree, BNode *left, char *key, BNode *right)
{
	BNode *bnode;
	uint8_t index;
	right->child = left->child;
	left->child  = (BNode **)right;
	if (btree->root != left) {
		bnode = btree->track[--btree->curr_depth].node;
		index = btree->track[btree->curr_depth].index;
	} else {
		bnode = newBNode(btree, NODE);
		if (!bnode) return Fatal;
		bnode->child[0] = left;
		++bnode->ncount;
		btree->root = bnode;
		index = 0;
	}
	_insert_pair(bnode, index, right, key, btree->key_len);

	while (bnode->kcount == btree->n) {
		left  = bnode;
		right = newBNode(btree, NODE);
		if (!right) return Fatal;
		if (bnode != btree->root) {
			bnode = btree->track[--btree->curr_depth].node;
			index = btree->track[btree->curr_depth].index;
		} else {
			bnode = newBNode(btree, NODE);
			if (!bnode) return Fatal;
			bnode->child[0] = left;
			++bnode->ncount;
			btree->root = bnode;
			index = 0;
		}
		_part_half(right, left, btree->min_key + 1, btree->n, btree->key_len);
		memcpy(key, PTR(left->key, btree->min_key, btree->key_len), btree->key_len);
		_insert_pair(bnode, index, right, key, btree->key_len);
	}
	return Ok;
}

status insert_data(BTree *btree, const void *val)
{
	BNode *leaf = _find_leaf(btree, val);
	if (leaf->page->index != leaf->index)
		leaf->page = get_page(&btree->pager, leaf->index);
	else
		++leaf->page->age;

	uint8_t pos = _get_index(	leaf->page->data, btree->data_len, btree->max_key,
														val, btree->key_len, btree->compare, true);
	if (pos != 0xFF) {
		if (KEY(leaf) != btree->max_key) {
			insert_to_page(leaf->page, btree->max_key, pos, val, btree->data_len);
		} else {
			leaf->page->status = WRITE;
			BNode *new = newBNode(btree, LEAF);
			char key[btree->key_len];
			if (pos >= btree->min_key) {
				split_page(new->page, leaf->page, btree->min_key, btree->n, btree->data_len);
				pos -= btree->min_key;
				if (!pos) memcpy(key, val, btree->key_len);
				else      memcpy(key, new->page + btree->n, btree->key_len);
				insert_to_page(new->page, btree->max_key, pos, val, btree->data_len);
			} else {
				split_page(new->page, leaf->page, btree->min_key-1, btree->n, btree->data_len);
				insert_to_page(leaf->page, btree->max_key, pos, val, btree->data_len);
			}
			++new->page->age;
			leaf->page->status = false;
			return _insert_fixup(btree, leaf, key, new);
		}
	} else {
		// alert("拥有该关键值的数据已存在 :(");
		return Bad;
	}
	return Ok;
}

static status _merge_leaf(BTree *btree, const uint8_t index,
	BNode *left, char *key, BNode *right)
{

}

static void _get_pair(BTree *btree, BNode *parent, const uint8_t index)
{
	char key[btree->key_len];
	if (index) {
		memcpy(key, PTR(parent->key, index-1, len), len);
		left  = bnode->child[index-1];
		right = bnode->child[index];
	} else {
		memcpy(key, PTR(parent->key, 0, len), len);
		left  = bnode->child[0];
		right = bnode->child[1];
	}
}

static status _delete_fixup(BTree *btree)
{
	BNode *bnode;
	uint8_t index;
	if (bnode != btree->root) {
		bnode = btree->track[--btree->curr_depth].node;
		index = btree->track[btree->curr_depth].index;
		_merge_or_redis_leaf(btree, bnode, index);
	} else {
		return Ok;
	}

	while (1) {
		if (bnode == btree->root) {
			if (bnode->tag == NODE && bnode->ncount == 1) {
				btree->root = bnode->child[0];
				free(bnode);
			}
			return Ok;
		}
		bnode = btree->track[--btree->curr_depth];

	}
}

status delete_data(BTree *btree, const void *key)
{
	BNode *leaf = _find_leaf(btree, key);
	if (leaf->page->index != leaf->index)
		leaf->page = get_page(&btree->pager, leaf->index);
	else
		++leaf->page->age;

	uint8_t pos = _get_index(	leaf->page->data, btree->data_len, btree->max_key,
														key, btree->key_len, btree->compare, false);
	if (pos != 0xFF) {
		delete_from_page(leaf->page, btree->n, pos, key, btree->data_len);
		if (KEY(leaf) < btree->min_key)
			_delete_fixup(btree);
	} else {
		// alert("不存在该关键值 :(");
		return Bad;
	}
	return Ok;
}
