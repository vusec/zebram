#include "zram_cache.h"

static int lra_empty = 0;
extern unsigned int cache_size;

#define ZEBRA_NATIVE 1

#ifdef EVICTION_POLICY_RANDOM

#else
static long lra_ptr = 0;
#endif


static struct _cache_entry *lra_head = NULL;


struct _zebra_cache  zebra_cache;

int zebra_cache_init(){

	INIT_RADIX_TREE(&zebra_cache.pages, GFP_ATOMIC);

#ifdef EVICTION_POLICY_RANDOM
#else
	lra_ptr = 0;
#endif

        lra_head =  (struct  _cache_entry *) vmalloc(sizeof(struct _cache_entry) * cache_size);

        if (!lra_head) {
                return -1;
        }
	return 0;	
}



int fill_lra(void){
        int ret = -1;
        struct page *tmp_page = NULL;
        int kj = 0;

        if((lra_head != NULL) &&  (lra_empty == 0))
         {
                printk("Creating LRA Array\n");

                for(kj=0; kj < cache_size; kj++){
                        lra_head[kj].flag = EMPTY;
                        lra_head[kj].dirty = 0;
                        // tmp_page =  alloc_page(__GFP_HIGHMEM | GFP_NOIO | __GFP_ZERO);
#ifdef ZEBRA_NATIVE
                         tmp_page =  alloc_page(__GFP_HIGHMEM & (~__GFP_RECLAIM));
#else
                         tmp_page = get_guard_cachepage();// alloc_page(__GFP_HIGHMEM | GFP_NOIO | __GFP_ZERO);
#endif
                        if(!tmp_page)
                                printk("Could not allocate page for cache %d \n", kj);
                        lra_head[kj].page  = tmp_page; //get_guard_cachepage();
                        tmp_page = NULL;
                }
#ifdef EVICTION_POLICY_RANDOM

                printk(" Cache eviction policy is RANDOM_EVICTION lra addr : %p, len : %d and  (allocated count= %d)\n", lra_head, cache_size, kj);
#else
                printk(" Cache eviction policy is LEAST RECENTLY ADDED lra addr : %p, len : %d and lraptr is : %ld  (allocated count= %d)\n", lra_head, cache_size, lra_ptr, kj);
#endif
                lra_empty = 1;

                ret = 0;
        }
        else{

                pr_err("Error creating LRA\n");
                printk("Error creating LRA\n");
        }

        return ret;

}



static struct _cache_entry *zebra_lookup_cache_page(u32 index)
{
        struct _cache_entry *cache_entry = NULL;

        cache_entry = radix_tree_lookup(&zebra_cache.pages, index);


        if(cache_entry){ 
                if(index != cache_entry->blkid){
                                printk("BUG LOOKUP ERROR index %d != blkid %d\n", index,  cache_entry->blkid);
                                BUG_ON(cache_entry->blkid != index);
                                //BUG_ON(cache_entry &&  cache_entry->page && cache_entry->page->index != index);
                                return NULL;

                }
                else{
//                      trace_printk("zebra_lookup_cache_page %d == %d (blkid %d)\n", index, cache_entry->page->index, cache_entry->blkid);
                }
        }


        return cache_entry;
}




int zebra_delete_cache_page(u32 index){
        struct _cache_entry *cache_entry = NULL;
        //printk("DELETING index : %d\n", index);
//      spin_lock_irqsave(&zebra_lock, zebra_lock_flags);
        cache_entry = radix_tree_delete(&zebra_cache.pages, index);

        if(cache_entry){
                return 0;
        }
        else{
                printk("BUG DELETE deleted Wrong entry?  : %d flag is %d\n", index, cache_entry->flag);
                BUG_ON(!cache_entry);
                return -1;
        }

}

struct _cache_entry  *  zebra_find_free_slot(){


	struct _cache_entry  *evict_entry  = NULL;

#ifdef EVICTION_POLICY_RANDOM
        long index = get_random_int() % cache_size;
	evict_entry = &lra_head[index];
#else
	evict_entry = &lra_head[lra_ptr];
	lra_ptr = (lra_ptr + 1) % cache_size;
#endif
	
	while(evict_entry->flag == RESERVED){
#ifdef EVICTION_POLICY_RANDOM	
		index = get_random_int() % cache_size;
		evict_entry = &lra_head[index];
#else
                lra_ptr = (lra_ptr + 1) % cache_size;
                evict_entry = &lra_head[lra_ptr];
#endif
        }

	return evict_entry;

}

int zebra_update_cache(u32 index, struct page *page){
        int ret = -1;
        struct _cache_entry *cache_entry = NULL;
//        struct page *page = NULL;
        struct page * cache_page = NULL;
        void *user_mem = NULL;
        void  *cache_mem = NULL;


        cache_entry = zebra_lookup_cache_page(index);


        if(cache_entry && (cache_entry->flag != RESERVED)) {
//                page = bvec->bv_page;
                cache_page = cache_entry->page;
                cache_mem = kmap_atomic(cache_page);
                user_mem = kmap_atomic(page);

                memcpy(cache_mem, user_mem, PAGE_SIZE);

                kunmap_atomic(user_mem);
                kunmap_atomic(cache_mem);
                cache_entry->flag = VALID;
                cache_entry->dirty = 1;
                ret = 0;

        }

        return ret;
}




int zebra_insert_cache_page(struct  _cache_entry *cache_entry, struct page *page, u32 index, int write)
{

                struct  _cache_entry *tmp_cache_entry = NULL;
//                struct page *page = NULL;
                struct page * cache_page = NULL;
                void *user_mem = NULL;
                void  *cache_mem = NULL;

                //printk("Inserting %d == %d (blkid %d)\n", index, cache_entry->page->index, cache_entry->blkid);
                if((cache_entry->flag  != RESERVED) && (cache_entry->flag  != EMPTY) ){
                        printk("BUG INSERT? ??? index = %d, flag =  %d (blkid %d)\n", index, cache_entry->flag, cache_entry->blkid);
                        BUG_ON(cache_entry->flag != RESERVED);

                }

//                page = bvec->bv_page;
                cache_page = cache_entry->page;
                cache_mem = kmap_atomic(cache_page);
                user_mem = kmap_atomic(page);

                memcpy(cache_mem, user_mem, PAGE_SIZE);

                kunmap_atomic(user_mem);
                kunmap_atomic(cache_mem);

                cache_entry->page->index = index;
                cache_entry->blkid = index;
                cache_entry->flag = VALID;
                cache_entry->dirty = write;

// (radix_tree_preload(GFP_NOIO)) ???
/*              if (radix_tree_preload(GFP_ATOMIC)) {
                        printk("ERROR: radix_tree_preload failed inside flush_to_disk\n");
                        return -1;
                }
*/

                if (radix_tree_insert(&zebra_cache.pages, index, cache_entry)) {
                        printk("BUG INSERT ERROR %u == %u (blkid %u)\n", (unsigned int) index, (unsigned int)cache_entry->page->index, (unsigned int)cache_entry->blkid);
                        tmp_cache_entry  = radix_tree_lookup(&zebra_cache.pages, index);
                        BUG_ON(!tmp_cache_entry);
                        BUG_ON(!tmp_cache_entry->page);
                        BUG_ON(tmp_cache_entry->page->index != index);
                }

//              radix_tree_preload_end();

                return 0;

}





int zebra_read_from_cache(u32 index, struct page *page){
        int ret = -1;
        struct _cache_entry *cache_entry = NULL;
//        struct page *page = NULL;
        struct page * cache_page = NULL;
        void *user_mem = NULL;
        void  *cache_mem = NULL;


        cache_entry = zebra_lookup_cache_page(index);


        if(cache_entry && (cache_entry->flag == VALID)){
  //              page = bvec->bv_page;
                cache_page = cache_entry->page;

                user_mem = kmap_atomic(page);
                cache_mem = kmap_atomic(cache_page);


                memcpy(user_mem , cache_mem, PAGE_SIZE);

                kunmap_atomic(user_mem);
                kunmap_atomic(cache_mem);

                ret = 0;
        }

        return ret;
}




