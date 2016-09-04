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
#define OFF(leaf, index)     (*((uint8_t *)(leaf->page->data + 1 + index)))

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

void free_bnode(BNode *bnode)
{
	free(bnode);
}

status init_btree(BTree 	*btree, uint16_t key_len, uint16_t total,
									int8_t (*compare)(const void *, const void *, const uint32_t))
{
	if (init_pager(&btree->pager, 4096) != Ok)
		return Bad;

	if ((total + 1) <= 16 || (total + 1) >= 2048) {
		free_pager(&btree->pager);
		fatal("单行表数据不满足限制: 大于16字节小于2KB :(");
	}

	btree->n = ((btree->pager.page_size - 1) / (total + 1)) + 1;
	btree->key_len  = key_len;

	btree->min_key  = ((btree->n - 1) + ((btree->n - 1) % 2)) >> 1;
	btree->max_key  = btree->n - 1;

	btree->min_node = (btree->n + (btree->n % 2)) >> 1;
	btree->max_node = btree->n + 1;
	btree->data_len = total;

	btree->root = newBNode(btree, LEAF);
	if (!btree->root) {
		free_pager(&btree->pager);
		return Bad;
	}
	btree->compare = compare;

	if (pthread_mutex_init(&btree->lock, NULL)) {
		free_pager(&btree->pager);
		warning("B+树互斥量初始化失败 :(");
	}
	return Ok;
}

status free_btree(BTree *btree)
{
	if (free_pager(&btree->pager) != Ok)
		return Bad;

	if (pthread_mutex_destroy(&btree->lock))
		alert("B+树互斥量销毁失败 :(");

	free(btree);
	return Ok;
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

static BNode* _find_leaf(BTree *btree, const void *key, Pair *stack, uint8_t *depth)
{
	BNode *curr = btree->root;
	for (; curr->tag != LEAF;) {
		uint8_t index = _get_descend_index(curr, key, btree->key_len, btree->compare);
		stack[*depth].node = curr;
		stack[*depth].index = index;
		++*depth;
		curr = curr->child[index];
	}

	if (curr->page->index != curr->index)
		curr->page = get_page(&btree->pager, curr->index);
	else
		++curr->page->age;

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

static void _insert_key(BNode *bnode, const uint8_t pos, const void *key, const uint8_t len)
{
	if (bnode->kcount > pos)
		memmove(PTR(bnode->key, pos+1, len), PTR(bnode->key, pos, len),
						LEN(pos, bnode->kcount, len));
	memcpy(PTR(bnode->key, pos, len), key, len);
	++bnode->kcount;
}

static void _insert_child(BNode *bnode, const uint8_t pos, BNode *child)
{
	if (bnode->ncount > pos)
		memmove(PTR(bnode->child, pos+1, PTR_SIZE), PTR(bnode->child, pos, PTR_SIZE),
						LEN(pos, bnode->ncount, PTR_SIZE));
	bnode->child[pos] = child;
	++bnode->ncount;
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
	src->kcount = beg - 1;
}

static void _part_child(BNode *dst, BNode *src, const uint8_t beg, const uint8_t end)
{
	memcpy(PTR(dst->child, 0, PTR_SIZE), PTR(src->child, beg, PTR_SIZE), LEN(beg, end, PTR_SIZE));
	dst->ncount = end - beg;
	src->ncount = (uint8_t)beg;
}

static void _part_half(BNode *dst, BNode *src, const uint8_t beg, const uint8_t end,
	const uint8_t len)
{
	_part_key(dst, src, beg, end, len);
	_part_child(dst, src, beg, end + 1);
}

static status _insert_fixup(BTree *btree, BNode *left, char *key, BNode *right,
	Pair *stack, uint8_t depth)
{
	BNode *bnode;
	uint8_t index;
	if (btree->root != left) {
		bnode = stack[--depth].node;
		index = stack[depth].index;
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
			bnode = stack[--depth].node;
			index = stack[depth].index;
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

static status _merge_or_redis_leaf(BTree *btree, BNode *parent, const uint8_t pos,
	const uint8_t index, char *key, bool *merge)
{
	BNode *left, *right;
	if (index) {
		left  = parent->child[index-1];
		right = parent->child[index];
		right->page->status = WRITE;
		if (left->index != left->page->index) left->page = get_page(&btree->pager, left->index);
		else ++left->page->age;
		right->page->status = false;
	} else {
		left  = parent->child[0];
		right = parent->child[1];
		left->page->status = WRITE;
		if (right->index != right->page->index) right->page = get_page(&btree->pager, right->index);
		else ++right->page->age;
		left->page->status = false;
	}
	*merge = ((KEY(left) + KEY(right)) <= btree->n);
	if (*merge) {
		if (index)
			delete_from_page(right->page, btree->n, pos, key, btree->data_len);
		else
			delete_from_page(left->page, btree->n, pos, key, btree->data_len);
		merge_page(left->page, right->page, btree->n, btree->data_len);
		left->child = right->child;
	} else {
		if (index)
			move_last_to_right(left->page, right->page, pos, btree->n, btree->data_len);
		else
			move_first_to_left(left->page, right->page, pos, btree->n, btree->data_len);
		memcpy(	key, PTR(right->page->data + btree->n, OFF(right, 0), btree->data_len),
						btree->key_len);
	}
	return Ok;
}

static void _delete_key(BNode *bnode, const uint8_t beg, const uint8_t key_len)
{
	--bnode->kcount;
	if (bnode->kcount > beg)
		memmove(PTR(bnode->key, beg, key_len), PTR(bnode->key, beg+1, key_len),
						LEN(beg, bnode->kcount, key_len));
}

static void _delete_child(BNode *bnode, const uint8_t beg)
{
	--bnode->ncount;
	if (bnode->ncount > beg)
		memmove(PTR(bnode->child, beg, PTR_SIZE), PTR(bnode->child, beg+1, PTR_SIZE),
						LEN(beg, bnode->ncount, PTR_SIZE));
}

static void _delete_pair(BNode *bnode, const uint8_t index, const uint8_t key_len)
{
	_delete_key(bnode, index, key_len);

	free_bnode(bnode->child[index+1]);
	_delete_child(bnode, index+1);
}

static void _merge_key(BNode *left, BNode *right, const uint8_t key_len)
{
	memcpy(	PTR(left->key, left->kcount, key_len), PTR(right->key, 0, key_len),
					LEN(0, right->kcount, key_len));
	left->kcount += right->kcount;
}

static void _merge_child(BNode *left, BNode *right)
{
	memcpy(	PTR(left->child, left->ncount, PTR_SIZE), PTR(right->child, 0, PTR_SIZE),
					LEN(0, right->ncount, PTR_SIZE));
	left->ncount += right->ncount;
}

static void _merge_half(BNode *left, const char *key, BNode *right, const uint8_t key_len)
{
	memcpy(PTR(left->key, left->kcount, key_len), key, key_len);
	++left->kcount;

	_merge_key(left, right, key_len);
	_merge_child(left, right);
}

static status _merge_or_redis_node(BTree *btree, BNode *parent, const uint8_t index,
	char *key, bool *merge)
{
	BNode *left, *right;
	char mid[btree->key_len];
	if (index) {
		memcpy(mid, PTR(parent->key, index-1, btree->key_len), btree->key_len);
		left  = parent->child[index-1];
		right = parent->child[index];
	} else {
		memcpy(mid, PTR(parent->key, 0, btree->key_len), btree->key_len);
		left  = parent->child[0];
		right = parent->child[1];
	}
	*merge = (left->ncount + right->ncount) < btree->max_node;
	if (*merge) {
		_merge_half(left, mid, right, btree->key_len);
	} else {
		if (index) {
			memcpy(key, PTR(left->key, --left->kcount, btree->key_len), btree->key_len);
			_insert_key(right, 0, mid, btree->key_len);
			_insert_child(right, 0, left->child[--left->ncount]);
		} else {
			memcpy(key, PTR(right->key, 0, btree->key_len), btree->key_len);
			_insert_pair(left, left->kcount, right->child[0], mid, btree->key_len);
			_delete_key(right, 0, btree->key_len);
			_delete_child(right, 0);
		}
	}
	return Ok;
}

static status _delete_fixup(BTree *btree, BNode *parent, Pair *stack, uint8_t depth)
{
	uint8_t index;
	char key[btree->key_len];
	bool merge;
	while (1) {
		if (parent == btree->root) {
			if (parent->ncount == 1) {
				btree->root = parent->child[0];
				free_bnode(parent);
			}
			break;
		}
		if (parent->ncount >= btree->min_node) break;
		parent = stack[--depth].node;
		index = stack[depth].index;
		_merge_or_redis_node(btree, parent, index, key, &merge);
		if (!merge) {
			memcpy(PTR(parent->key, (index ? (index-1) : 0), btree->key_len), key, btree->key_len);
			break;
		} else {
			_delete_pair(parent, (index ? (index-1) : 0), btree->key_len);
		}
	}
	return Ok;
}

status delete_data(BTree *btree, const void *key)
{
	uint8_t depth = 0;
	Pair stack[MAX_DEPTH];

	BNode *leaf = _find_leaf(btree, key, stack, &depth);

	uint8_t pos = _get_index(	leaf->page->data, btree->data_len, btree->max_key,
														key, btree->key_len, btree->compare, false);
	if (pos != 0xFF) {
		if (KEY(leaf) != btree->min_key || leaf == btree->root) {
			delete_from_page(leaf->page, btree->n, pos, key, btree->data_len);
		} else {
			BNode *parent = stack[--depth].node;
			uint8_t index = stack[depth].index;
			char key[btree->key_len];
			bool merge;
			_merge_or_redis_leaf(btree, parent, pos, index, key, &merge);
			if (!merge) {
				memcpy(PTR(parent->key, (index ? (index-1) : 0), btree->key_len), key, btree->key_len);
			} else {
				_delete_pair(parent, (index ? (index-1) : 0), btree->key_len);
				_delete_fixup(btree, parent, stack, depth);
			}
		}
		--btree->tuple;
	} else {
		alert("不存在该关键值 :(");
		return Bad;
	}
	return Ok;
}

status insert_data(void *tree, const void *val)
{
	BTree *btree = (BTree *)tree;
	uint8_t depth = 0;
	Pair stack[MAX_DEPTH];
	pthread_mutex_lock(&btree->lock);
	BNode *leaf = _find_leaf(btree, val, stack, &depth);

	uint8_t pos = _get_index(	leaf->page->data, btree->data_len, btree->max_key,
														val, btree->key_len, btree->compare, true);
	if (pos != 0xFF) {
		if (KEY(leaf) != btree->max_key) {
			insert_to_page(leaf->page, btree->max_key, pos, val, btree->data_len);
		} else {
			leaf->page->status = WRITE;
			BNode *new = newBNode(btree, LEAF);
			leaf->page->status = false;
			char key[btree->key_len];
			if (pos >= btree->min_key) {
				split_page(new->page, leaf->page, btree->min_key, btree->n, btree->data_len);
				pos -= btree->min_key;
				if (!pos) memcpy(key, val, btree->key_len);
				else      memcpy(key, new->page->data + btree->n, btree->key_len);
				insert_to_page(new->page, btree->max_key, pos, val, btree->data_len);
			} else {
				split_page(new->page, leaf->page, btree->min_key-1, btree->n, btree->data_len);
				memcpy(key, new->page->data + btree->n, btree->key_len);
				insert_to_page(leaf->page, btree->max_key, pos, val, btree->data_len);
			}
			new->child = leaf->child;
			leaf->child  = (BNode **)new;
			++new->page->age;
			_insert_fixup(btree, leaf, key, new, stack, depth);
		}
		++btree->tuple;
	} else {
		alert("拥有该关键值的数据已存在 :(");
		return Bad;
	}
	pthread_mutex_unlock(&btree->lock);
	return Ok;
}
