#include "zram_drv.h"
#include <linux/types.h>
#include <linux/random.h>

#define LRA_MAX 12288
//#define  EVICTION_POLICY_RANDOM 1

enum state{RESERVED, VALID, EMPTY};


struct  _cache_entry{
        u32 blkid;
        struct page *page;
        enum state flag;
	int dirty ;
};


struct _zebra_cache{
        struct radix_tree_root  pages;

};

int zebra_cache_init(void);
int fill_lra(void);

int zebra_delete_cache_page(u32 index);
int zebra_read_from_cache(u32 index, struct page *page);
struct _cache_entry  *  zebra_find_free_slot(void);
int zebra_update_cache(u32 index, struct page *page);
int zebra_insert_cache_page(struct  _cache_entry *cache_entry, struct page *page, u32 index, int write);
extern struct page *  get_guard_cachepage(void);
extern int  put_guard_cachepage(struct page *page);
