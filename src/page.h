/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-23 01:19:56
**/

#ifndef _PAGE_H_
#define _PAGE_H_

#include "error.h"
#include "util.h"

#define  MAX_PAGE						0x2000
#define  BUCKET_SIZE			 (MAX_PAGE / 16)
#define  MAX_PAGE_PER_FILE	4096

#define  READ   1
#define  WRITE  2

typedef struct
{
	unsigned		dirty:1;
	unsigned	 status:2;
	unsigned			age:16;
	unsigned       fd:10;
	unsigned         :3;
	uint32_t 	 	index;
	void 				*data;
}Page;

typedef struct
{
	uint8_t  			 *bit;
	uint8_t   			size;
	Page 		 			 *page[16];
}PageBucket;

typedef struct
{
	int 		  	*data_fd;
	uint32_t		 data_page_num;
	uint32_t   	 data_file_num;
	uint16_t 		 page_size;
	PageBucket	 bucket[BUCKET_SIZE];
}Pager;

status init_pager(Pager *pager, uint16_t page_size);
status free_pager(Pager *pager);
Page*  fresh_page(Pager *pager);
Page*  get_page(Pager *pager, uint32_t index);
void	 insert_to_page(Page *page, const uint8_t end, const uint8_t pos,
											const void *val, const uint16_t len);
void 	 split_page(Page *pdst, Page *psrc, const uint8_t beg, const uint8_t end,
									const uint16_t len);
void   delete_from_page(Page *page, const uint8_t max_key, const uint8_t pos,
												const void *key, const uint16_t len);
void 	 merge_page(Page *left, Page *right, const uint8_t max, const uint16_t len);
void   move_last_to_right(Page *left, Page *right, const uint8_t pos, const uint8_t end,
													const uint16_t len);
void   move_first_to_left(Page *left, Page *right, const uint8_t pos, const uint8_t end,
													const uint16_t len);
void   scan_page(	const Page *page, const uint8_t max, const uint16_t total,
									const uint8_t key_len);

#endif /* _PAGE_H_ */