// This file is located in linux-5.15.0/include/linux/extent.h

#ifndef _LINUX_EXTENTS_H
#define _LINUX_EXTENTS_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>

struct page {
    phys_addr_t paddr;
    struct list_head node; 
};

struct extent {
    phys_addr_t paddr_start; 
    phys_addr_t paddr_end; 
    unsigned long id; 
    unsigned long num_pages; 
    struct list_head list;
    struct rb_node node; 
};

struct extent_table {
    struct rb_root root; 
    spinlock_t lock; 
    unsigned long next_id; 
    unsigned long total_extents;
};

struct extent_table *extent_table_alloc(void);
void extent_table_free(struct extent_table *et);
int  extent_table_insert_page(struct extent_table *et, phys_addr_t phys);
void extent_table_print_stats(struct extent_table *et, pid_t pid);

#endif