#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#include <linux/init.h>
#include <linux/fs.h>		
#include <linux/fs_struct.h>
//#include <linux/fd_table.h>
#include <linux/sched.h>		
#include <linux/seq_file.h>	
#include <asm/stat.h>
#include <linux/file.h>
#include <linux/rcupdate.h>
#include <linux/vmalloc.h>
//#include <linux/fdtable.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/mman.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/io.h>		/* phys_to_virt */
#include <asm/page.h>
#include <linux/highmem.h>


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Qemu zram driver Kernel Module");
MODULE_AUTHOR("R.K.");

static char* swap_space = NULL;
//static unsigned long *free_ll = NULL;

#define MEMORY_MAP_ADDRS    0x40000000	    // From 2GB to 4GB
#define MEMORY_MAP_SIZE	    0x80000000	    // 2 * 1024 * 1024 * 1024 = 2GB
const unsigned long two_g = 524288;      // 1.8 G
const unsigned long one_g = 262144;      // 1.8 G

struct _alis_page {
    unsigned long addr;
    struct list_head list; /* kernel's list structure */
};


static DEFINE_SPINLOCK(qemu_spinlock);
//static DEFINE_SPINLOCK(qemu_cache_spinlock);
//static unsigned long cache_flags;
static unsigned long flags;
extern char *reserved_mem_zebra;
extern char *reserved_mem_zebra_2;
#define MSG_SIZE (512)
static char *msg;
#define ourmin(a,b) (((a)<(b)) ? (a) : (b))



static struct proc_dir_entry* qemu_dir;
static struct proc_dir_entry* qemu_file;
static struct proc_dir_entry* length_file;
static struct proc_dir_entry* addr_file;

struct alis_node
{
  unsigned long data;
  struct alis_node* next;
};


static struct alis_node* alis_head;
static struct alis_node* alis_cache_head = NULL;

static unsigned long alis_counter;
static unsigned long alis_cache_counter;

struct alis_node* alis_push(struct alis_node* head, unsigned long data)
{
  struct alis_node* tmp = (struct alis_node*) kmalloc(sizeof(struct alis_node), (GFP_KERNEL & (~__GFP_RECLAIM)));
  if(tmp == NULL){
    printk("Not enough memory to create a ALIS NODE \n");
    return NULL;
  }

  tmp->data = data;
  tmp->next = head;
  head = tmp;
  alis_counter++;
  return head;
}



struct alis_node* alis_pop(struct alis_node *head, unsigned long  *element)
{
  struct alis_node* tmp = head;
  if(tmp != NULL) { 
  	*element = head->data;
    	head = head->next;
  	alis_counter--;
    	kfree(tmp);
  }
  return head;
}




int alis_empty(struct alis_node* head)
{
  return head == NULL ? 1 : 0;
}


void alis_del_all(struct alis_node* head){
  
  unsigned long element =0;
  while(alis_empty(head) == 0)
  {
    head = alis_pop(head,&element);
  }
}




void alis_display(struct alis_node* head)
{
  struct alis_node *tmp;
  tmp = head;
  if(tmp!= NULL)
  {
    printk("Stack: ");
    do
    {
      printk("%lu \n ",tmp->data);
      tmp = tmp->next;
    }
    while (tmp!= NULL);
    printk("\n");
  }
  else
  {
    printk("The Stack is empty\n");
  }

}





	static int 
addr_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", msg);
	return 0;
}

	static int
addr_open(struct inode *inode, struct file *file)
{
	return single_open(file, addr_show, NULL);
}



	static int 
length_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", msg);
	return 0;
}

	static int
length_open(struct inode *inode, struct file *file)
{
	return single_open(file, length_show, NULL);
}

	static int 
qemu_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", msg);
	return 0;
}

	static int
qemu_open(struct inode *inode, struct file *file)
{
	return single_open(file, qemu_show, NULL);
}


ssize_t addr_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{

	return count;
}

/*
void put_guard_cachepage(struct page *page){

    unsigned long phys_address = 0;
    unsigned long pfn = 0;

    pfn = page_to_pfn(page);
    alis_head = alis_push(alis_cache_head, pfn);

}
EXPORT_SYMBOL_GPL(put_guard_cachepage);
*/


void put_guard_page(struct page *page){

    //unsigned long phys_address = 0;
    unsigned long pfn = 0;

    pfn = page_to_pfn(page);
    spin_lock_irqsave(&qemu_spinlock, flags);
    alis_head = alis_push(alis_head, pfn);
    spin_unlock_irqrestore(&qemu_spinlock, flags);
    
    

}
EXPORT_SYMBOL_GPL(put_guard_page);


struct page * get_guard_page(void){

	unsigned long pfn_num = 0;
	//unsigned long phys_address = 0;
	void *tmp_ptr = NULL;
	void *tmp_page = NULL;
    	spin_lock_irqsave(&qemu_spinlock, flags);
	if(alis_empty(alis_head) == 0){
		
		alis_head = alis_pop(alis_head, &pfn_num);
    		spin_unlock_irqrestore(&qemu_spinlock, flags);
		tmp_page = pfn_to_page(pfn_num);
		tmp_ptr = kmap_atomic(tmp_page);
		kunmap_atomic(tmp_ptr);
		return  pfn_to_page(pfn_num);	
	}
	else{
	        printk(KERN_DEBUG "get_guard_page : no more guard page left\n");
    		spin_unlock_irqrestore(&qemu_spinlock, flags);
                return NULL;
	}

}
EXPORT_SYMBOL_GPL(get_guard_page);


/*
struct page * get_guard_cachepage(void){

	unsigned long pfn_num = 0;
	unsigned long phys_address = 0;
	if(alis_empty(alis_cache_head) == 0){
		
		alis_cache_head = alis_pop(alis_cache_head, &phys_address);
		pfn_num = __phys_to_pfn(phys_address);
		return  pfn_to_page(pfn_num);	
	}
	else{
	        printk(KERN_DEBUG "get_guard_page : no more guard page left\n");
                return NULL;
	}

}
EXPORT_SYMBOL_GPL(get_guard_cachepage);
*/


ssize_t qemu_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
        size_t len = count/(sizeof(unsigned long));
 	int i = 0;
	unsigned long *free_l = NULL;

	free_l =  (unsigned long *) vmalloc(sizeof(unsigned long) * len);
	memset(free_l, 0, sizeof(unsigned long) * len );
	copy_from_user(free_l, buf, sizeof(unsigned long) * len );

	alis_del_all(alis_head);
	alis_head = NULL;
	alis_counter = 0;

	for(i=len-1; i >= 0 ; i--){
	
		alis_head = alis_push(alis_head, free_l[i]);
    	}
        printk("Stack guard pages qemu %lu - %lu \n ", free_l[0], free_l[len-1]);

	if(free_l != NULL){
		vfree(free_l);
		
	}
	
	return count;
}



ssize_t length_write(struct file *filp,const char *buf,size_t count,loff_t *offp)

{

        size_t len = count/(sizeof(unsigned long));
 	int i = 0;
	unsigned long *free_l = NULL;

	free_l =  (unsigned long *) vmalloc(sizeof(unsigned long) * len);
	memset(free_l, 0, sizeof(unsigned long) * len );
	copy_from_user(free_l, buf, sizeof(unsigned long) * len );
	alis_del_all(alis_cache_head);
	alis_cache_head = NULL;
	alis_cache_counter = 0;

        printk("StackKKK cache qemu %lu - %lu length(%lu)\n ", free_l[0], free_l[len-1], len);
        printk("Stack cache qemu %lu - %lu length(%lu)\n ", free_l[0], free_l[len-1], len);
	for(i=len-1; i >= 0 ; i--){
	
		alis_cache_head = alis_push(alis_cache_head, free_l[i]);
    	}

	if(free_l != NULL){
		vfree(free_l);
		
	}
	
	return count;


}




static const struct file_operations length_fops = {
	.owner	= THIS_MODULE,
	.open	= length_open,
	.write	= length_write,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};

static const struct file_operations addr_fops = {
	.owner	= THIS_MODULE,
	.open	= addr_open,
	.write	= addr_write,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};





static const struct file_operations qemu_fops = {
	.owner	= THIS_MODULE,
	.open	= qemu_open,
	.write	= qemu_write,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

	static int __init 
qemu_drv_init(void)
{
        unsigned long address;
        unsigned long address_2;
        //unsigned long pfn;
        //unsigned long long int *back_store_ptr;
        unsigned long pa;
        int i = 0;

	qemu_dir = proc_mkdir("qemu", NULL);
	qemu_file = proc_create("mem", 0777, qemu_dir, &qemu_fops);
	length_file = proc_create("length", 0777, qemu_dir, &length_fops);
	addr_file = proc_create("addr", 0777, qemu_dir, &addr_fops);
	alis_head = NULL;
	alis_cache_head = NULL;
        alis_cache_counter = 0;

	if (!qemu_file) {
		return -ENOMEM;
	}


        alis_del_all(alis_head);
        alis_head = NULL;

        unsigned long pa1 = virt_to_phys(reserved_mem_zebra);
        //unsigned long pa2 = virt_to_phys(reserved_mem_zebra_2);
        unsigned long pfn1 = __phys_to_pfn(pa1);
        //unsigned long pfn2 = __phys_to_pfn(pa2);
        //struct page *pa1_page = pfn_to_page(pfn1);
        //struct page *pa2_page = pfn_to_page(pfn2);

        if(swap_space == NULL){
                address = reserved_mem_zebra;
                address_2  = reserved_mem_zebra_2;
                printk("Address in unsigned long address %lu\n", address);

                //int ik = 0;
                for(i=one_g -1 ; i >= 0; i--){
                        pa = virt_to_phys(address_2 + (i * 4096));
                        pfn1       =  __phys_to_pfn(pa);
                        alis_head = alis_push(alis_head, pfn1);
                }
                swap_space  = (char *) reserved_mem_zebra;
                for(i=one_g -1 ; i >= 0; i--){
                        pa = virt_to_phys(address + (i * 4096));
                        pfn1 = __phys_to_pfn(pa);
                        alis_head = alis_push(alis_head, pfn1);
                }
        }

        printk(KERN_DEBUG "Number of alis pages free %lu\n", alis_counter);
        printk(KERN_DEBUG "Number of alis cache pages free %lu\n", alis_cache_counter);
	return 0;
}

	static void __exit
qemu_drv_exit(void)
{

 	alis_del_all(alis_head);
 	alis_del_all(alis_cache_head);
        alis_head = NULL;
        alis_cache_head = NULL;
	remove_proc_entry("mem", qemu_dir);
	remove_proc_entry("length", qemu_dir);
	remove_proc_entry("addr", qemu_dir);
	remove_proc_entry("qemu", NULL);
}



/* Declare entry and exit functions */
module_init(qemu_drv_init);
module_exit(qemu_drv_exit);
