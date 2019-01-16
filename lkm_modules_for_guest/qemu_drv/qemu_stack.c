#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
//#include <linux/fd_table.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <asm/stat.h>
#include <linux/file.h>

#include "qemu_stack.h"


struct node* push(struct node* head, unsigned long data)
{
  struct node* tmp = (struct node*) vmalloc(sizeof(struct node));
  if(tmp == NULL){
    return -ENOMEM;
  }

  tmp->data = data;
  tmp->next = head;
  head = tmp;
  return head;
}


struct node* pop(struct node *head, unsigned long  *element)
{
  struct node* tmp = head;
  *element = head->data;
  head = head->next;
  vfree(tmp);
  return head;
}

int empty(struct node* head)
{
  return head == NULL ? 1 : 0;
}

void display(struct node* head)
{
  struct node *current;
  current = head;
  if(current!= NULL)
  {
    printk("Stack: ");
    do
    {
      printk("%lu ",current->data);
      current = current->next;
    }
    while (current!= NULL);
    printk("\n");
  }
  else
  {
    printk("The Stack is empty\n");
  }

}


