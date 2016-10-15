/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-23 02:07:30
**/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "page.h"
#include "file.h"

static status _write_page(const Page *page, const uint16_t page_size)
{
	uint32_t index  = (uint32_t)page->index % MAX_PAGE_PER_FILE;
	uint32_t offset = index * page_size;
	if (pwrite(page->fd, page->data, page_size, offset) != page_size)
		fatal("页面写入失败 fd %2d  index %4d  offset %d :(", page->fd, index, offset);
	return Ok;
}

static status _read_page(const Page *page, const uint16_t page_size)
{
	uint32_t index  = (uint32_t)page->index % MAX_PAGE_PER_FILE;
	uint32_t offset = index * page_size;
	if (pread(page->fd, page->data, page_size, offset) < page_size)
		fatal("页面读取失败 fd %2d  index %4d  offset %d :(", page->fd, index, offset);
	return Ok;
}

static Page* newPage(const uint16_t page_size)
{
	Page *page = calloc(1, sizeof(Page));
	if (!page)
		warning("页面初始化失败 :(");

	page->data = calloc(page_size, 1);
	if (!page->data)
		warning("页面初始化失败 :(");

	return page;
}

static status _free_page(Page *page, const uint16_t page_size)
{
	if (page->dirty)
		if (_write_page(page, page_size) != Ok)
			return Bad;

	if (page->data)
		free(page->data);

	free(page);
	return Ok;
}

static status _init_bucket(PageBucket *bucket, uint8_t size, const uint16_t page_size)
{
	for (uint8_t i = 0; i != size; ++i) {
		bucket->page[i] = newPage(page_size);
		if (!bucket->page[i])
			return Bad;
	}

	uint8_t bit_size = size / 8 + ((size % 8) ? 1 : 0);
	bucket->bit  = (uint8_t *)calloc(bit_size, sizeof(uint8_t));
	if (!bucket->bit)
		warning("桶初始化失败 :(");

	bucket->size = size;
	return Ok;
}

static status _free_bucket(PageBucket *bucket, const uint16_t page_size)
{
	for (uint8_t i = 0; i != bucket->size; ++i) {
		if (bucket->page[i]) {
			if (_free_page(bucket->page[i], page_size) != Ok)
				return Bad;
		}
	}

	if (bucket->bit)
		free(bucket->bit);
	return Ok;
}

status init_pager(Pager *pager, uint16_t page_size)
{
	for (uint16_t i = 0; i != BUCKET_SIZE; ++i)
		if (_init_bucket(&pager->bucket[i], 16, page_size) != Ok)
			return Bad;
	pager->page_size = page_size;
	return Ok;
}

status free_pager(Pager *pager)
{
	for (uint16_t i = 0; i != BUCKET_SIZE; ++i)
		if (_free_bucket(&pager->bucket[i], pager->page_size) != Ok)
			return Bad;

	if (pager->data_fd)
		free(pager->data_fd);
	return Ok;
}

static bool _page_exist(const PageBucket *bucket, const uint8_t index)
{
	return (bucket->bit[index >> 3] & (1 << (index % 8)));
}

static Page* _get_page(Pager *pager, uint32_t bucket_index, int fd, uint32_t index, bool new)
{
	PageBucket *bucket = &pager->bucket[bucket_index];
	uint8_t size = bucket->size;
	uint8_t pos = size;
	uint16_t bit = 0xFFFF;
	for (uint8_t i = 0; i != size; ++i) {
		uint16_t tmp = bucket->page[i]->age;
		if (tmp < bit && !bucket->page[i]->status) {
			bit = tmp;
			pos = i;
		}
	}
	assert(pos != size);
	if (_page_exist(bucket, pos)) {
		if (bucket->page[pos]->dirty && _write_page(bucket->page[pos], pager->page_size) != Ok)
			return NULL;
	} else {
		bucket->bit[pos >> 3] |= (1 << (pos % 8));
	}
	bucket->page[pos]->dirty = false;
	bucket->page[pos]->age   = 0;
	bucket->page[pos]->fd    = fd;
	bucket->page[pos]->index = index;
	if (!new) {
		if (_read_page(bucket->page[pos], pager->page_size) != Ok)
			return NULL;
	} else {
		memset(bucket->page[pos]->data, 0, pager->page_size);
	}
	return bucket->page[pos];
}

Page *fresh_page(Pager *pager)
{
	uint32_t bucket_index = pager->data_page_num % BUCKET_SIZE;
	uint32_t index = pager->data_page_num++;

	if ((pager->data_page_num / MAX_PAGE_PER_FILE) == pager->data_file_num) {
		if (pager->data_file_num) {
			pager->data_fd = (int *)realloc(pager->data_fd, (pager->data_file_num+16) * sizeof(int));
			if (!pager->data_fd) warning("数据页面获取失败 :(");
		} else {
			pager->data_fd = (int *)calloc(16, sizeof(int));
			if (!pager->data_fd) warning("数据页面获取失败 :(");
		}
		pager->data_fd[pager->data_file_num] = new_file(pager->data_file_num, DATA);
		if (pager->data_fd < 0) return NULL;
		++pager->data_file_num;
	}
	int fd = pager->data_fd[index / MAX_PAGE_PER_FILE];

	return _get_page(pager, bucket_index, fd, index, true);
}

Page* get_page(Pager *pager, uint32_t index)
{
	uint32_t bucket_index = index % BUCKET_SIZE;
	int fd = pager->data_fd[index / MAX_PAGE_PER_FILE];
	return _get_page(pager, bucket_index, fd, index, false);
}

void insert_to_page(Page *page, const uint8_t max_key, const uint8_t pos,
	const void *val, const uint16_t len)
{
	void *data = page->data + 1;
	assert(max_key > pos);
	memmove(data + pos + 1, data + pos, (uint32_t)(max_key - pos - 1));
	uint8_t end = *(uint8_t *)(data - 1);
	*(uint8_t *)(data + pos) = end;
	data += max_key + (uint32_t)end * len;
	memcpy(data, val, len);
	++*(uint8_t *)(page->data + 0);
	page->dirty = true;
}

static void _fill_empty(uint8_t *start, const uint8_t emp,
	uint8_t *dst, uint8_t *src, const uint16_t len, uint8_t *pos,
	const uint8_t end)
{
	uint8_t i, id;
	for (i = *pos; i < end;) {
		if ((id = start[i]) >= end) {
			uint8_t *str = src + id * len;
			start[i] = emp;
			memcpy(dst, str, len);
			*pos = ++i;
			return ;
		}
		*pos = ++i;
	}
}

void split_page(Page *pdst, Page *psrc, const uint8_t beg, const uint8_t n, const uint16_t len)
{
	uint8_t num 	 = n - 1 - beg;
	uint8_t *ssrc  = psrc->data + 1;
	uint8_t *sdst  = pdst->data + 1;
	uint8_t *src = psrc->data + n;
	uint8_t *dst = pdst->data + n;
	uint8_t pos = 0;
	for (uint8_t i = 0; i < num; ++i)
		sdst[i] = i;
	for (uint8_t i = (uint8_t)beg, end = n - 1; i < end; ++i) {
		uint8_t id = ssrc[i];
		uint8_t *str = src + len * id;
		memcpy(dst, str, len);
		if (id < beg && pos < beg)
			_fill_empty(ssrc, id, str, src, len, &pos, beg);
		dst += len;
	}
	*(uint8_t *)(psrc->data + 0) = (uint8_t)beg;
	*(uint8_t *)(pdst->data + 0) = num;
	pdst->dirty = true;
	psrc->dirty = true;
}

void delete_from_page(Page *page, const uint8_t n, const uint8_t pos, const void *key,
	const uint16_t len)
{
	uint8_t *index = (uint8_t *)(page->data + 1);
	uint8_t idx  = index[pos];
	uint8_t num = --*(uint8_t *)(page->data + 0);

	void *data = page->data + n;

	if (idx != num)
		memcpy(data + (uint32_t)idx * len, data + (uint32_t)num * len, len);

	for (uint8_t i = 0; i != num; ++i) {
		if (i >= pos)
			index[i] = index[i + 1];
		if (index[i] == num)
			index[i] = idx;
	}
	page->dirty = true;
}

void merge_page(Page *left, Page *right, const uint8_t n, const uint16_t len)
{
	uint8_t lnum = *(uint8_t *)(left->data + 0);
	uint8_t rnum = *(uint8_t *)(right->data + 0);
	uint8_t *lindex = (uint8_t *)(left->data + 1);
	uint8_t *rindex = (uint8_t *)(right->data + 1);
	void *ldata = left->data + n;
	void *rdata = right->data + n;

	for (uint32_t i = 0, j = lnum; i < rnum; ++i, ++j) {
		lindex[j] = lnum + rindex[i];
		memcpy(ldata + j * len, rdata + i * len, len);
	}

	*(uint8_t *)(left->data + 0)  = lnum + rnum;

	left->dirty  = true;
}

void move_last_to_right(Page *left, Page *right, const uint8_t n, const uint16_t len)
{
	uint8_t *ldata = (uint8_t *)(left->data + 1);
	uint8_t *rdata = (uint8_t *)(right->data + 1);
	uint8_t lnum = --*(uint8_t *)(left->data + 0);
	uint8_t rnum = (*(uint8_t *)(right->data + 0))++;
	uint8_t pos  = ldata[lnum];

	memmove(rdata + 1, rdata, rnum);
	rdata[0] = rnum;
	memcpy(right->data + n + (uint32_t)rnum * len, left->data + n + (uint32_t)pos * len , len);

	for (uint8_t i = 0; i < lnum; ++i) {
		if (ldata[i] == lnum) {
			ldata[i] = pos;
			break;
		}
	}

	if (pos != lnum)
		memcpy(left->data + n + (uint32_t)pos * len, left->data + n + (uint32_t)lnum * len, len);

	left->dirty  = true;
	right->dirty = true;
}

void move_first_to_left(Page *left, Page *right, const uint8_t n, const uint16_t len)
{
	uint8_t *ldata = (uint8_t *)(left->data + 1);
	uint8_t *rdata = (uint8_t *)(right->data + 1);
	uint8_t lnum = (*(uint8_t *)(left->data + 0))++;
	uint8_t rnum = --*(uint8_t *)(right->data + 0);
	uint8_t pos  = rdata[0];

	ldata[lnum] = lnum;
	memcpy(left->data + n + (uint32_t)lnum * len, right->data + n + (uint32_t)pos * len , len);

	for (uint8_t i = 0; i < rnum; ++i) {
		rdata[i] = rdata[i + 1];
		if (rdata[i] == rnum)
			rdata[i] = pos;
	}

	if (pos != rnum)
		memcpy(right->data + n + (uint32_t)pos * len, right->data + n + (uint32_t)rnum * len, len);

	left->dirty  = true;
	right->dirty = true;
}

void scan_page(const Page *page, const uint8_t n, const uint16_t total, const uint8_t key_len)
{
	char buf[4096];
	uint32_t end = *(uint8_t *)(page->data + 0);
	sprintf(buf, "fd %2d  index %4d  total tuple %2d\n", page->fd, page->index, end);
	uint16_t len = strlen(buf);
	for (uint32_t i = 0; i != end; ++i) {
		char c[8];
		sprintf(c, "%d ", *(uint8_t *)(page->data + 1 + i));
		strcat(buf, c);
		len += strlen(c);
	}
	buf[len++] = '\n';
	for (uint32_t i = 0; i != end; ++i) {
		uint32_t index = *(uint8_t *)(page->data + 1 + i);
		snprintf(buf + len, key_len, "%s", (char *)(page->data + n + index * total));
		len += key_len - 1;
		snprintf(buf + len, 5, "%s", "     ");
		len += 4;
	}
	buf[len++] = '\0';
	puts(buf);
	getchar();
}
