// This file is located in linux-5.15.0/mm/extent.c

#include <linux/extent.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>

void extent_init_passed(void) {
    if (!current->extent_init_passed) {
        printk(KERN_INFO "Extent Initial Page Fault: Passed | PID: %d (%s)!\n", current->pid, current->comm);
        current->extent_init_passed = true; 
    }
}

// Function to allocate an extent table when called
struct extent_table *extent_table_alloc(void)
{
    struct extent_table *et;

    et = kmalloc(sizeof(*et), GFP_KERNEL);
    if (!et)
    {
        printk(KERN_ERR "Extent Table was not able to be allocated\n");
        return NULL;
    }

    et->root = RB_ROOT; // Initialize the RB tree with an empty value
    spin_lock_init(&et->lock); // Initialize table spinlock
    et->next_id = 0; // Next Extent ID to be assigned
    et->total_extents = 0; // Counter for current number of extents in table

    return et;
}

// Function to free an extent table
void extent_table_free(struct extent_table *et)
{
    struct rb_node *node;

    if (!et)
    {
        //printk(KERN_ERR "Invalid table to free\n");
        return;
    }
    node = rb_first(&et->root); // Eliminate in the order of going to the
                                // leftmost node in the tree.
    while (node)
    {
        struct extent *ext = rb_entry(node, struct extent, node);
        struct rb_node *next = rb_next(node);
        struct extent_page *page, *tmp;

        list_for_each_entry_safe(page, tmp, &ext->list, node)
        {
            list_del(&page->node);
            kfree(page);
        }

        rb_erase(&ext->node, &et->root);
        kfree(ext);

        node = next;
    }
    kfree(et);
}

int extent_table_insert_page(struct extent_table *et, phys_addr_t phys)
{
    struct extent *curr;
    struct extent *prev = NULL, *next = NULL;
    struct extent *new_ext;
    struct extent_page *page;
    struct rb_node **link, *parent = NULL;
    int insert_case;

    // Switch case variables for use later in dealing with extents
    enum 
    {
        EXTENT_BRIDGE = 1,
        EXTENT_EXTEND_PREV = 2,
        EXTENT_EXTEND_NEXT = 3,
        EXTENT_NEW = 4
    };
    
    if (!et)
        return -EINVAL;

    page = kmalloc(sizeof(*page), GFP_KERNEL); // Allocate page node
    if (!page)
        return -ENOMEM;

    page->paddr = phys;
    INIT_LIST_HEAD(&page->node);

    spin_lock(&et->lock); // Set spinlock while setting table elements

    // Traverse the RB tree to find the correct position and track
    // previous and next candidate extents
    link = &et->root.rb_node;
    while(*link)
    {
        curr = rb_entry(*link, struct extent, node);
        parent = *link;
        if(phys < curr->paddr_start)
        {
            next = curr;
            link = &(*link)->rb_left;
        }
        else 
        {
            prev = curr;
            link = &(*link)->rb_right;
        }
    }

    /*
    Handling 4 cases for extents.
    1. Bridging two previously separate extents 
    2. Extending an extent forward
    3. Extending an extent backward
    4. Creating a new extent
    */

    if (prev && phys == prev->paddr_end + PAGE_SIZE &&
        next && phys + PAGE_SIZE == next->paddr_start) 
    {
        insert_case = EXTENT_BRIDGE;
    } 
    else if (prev && phys == prev->paddr_end + PAGE_SIZE) 
    {
        insert_case = EXTENT_EXTEND_PREV;
    } 
    else if (next && phys + PAGE_SIZE == next->paddr_start) 
    {
        insert_case = EXTENT_EXTEND_NEXT;
    } 
    else 
    {
        insert_case = EXTENT_NEW;
    }

    switch (insert_case) 
    {
    // Case 1: Bridging two previously separate extents
    case EXTENT_BRIDGE:
        list_add_tail(&page->node, &prev->list);
        list_splice_tail_init(&next->list, &prev->list);

        prev->paddr_end = next->paddr_end;
        prev->num_pages += 1 + next->num_pages;

        rb_erase(&next->node, &et->root);
        kfree(next);
        et->total_extents--;

        spin_unlock(&et->lock);
        return 0;

    // Case 2: Extending an extent forward 
    case EXTENT_EXTEND_PREV:
        list_add_tail(&page->node, &prev->list);
        prev->paddr_end = phys;
        prev->num_pages++;
        spin_unlock(&et->lock);
        return 0;

    // Case 3: Extending an extent backward
    case EXTENT_EXTEND_NEXT:
        rb_erase(&next->node, &et->root);

        next->paddr_start = phys;
        next->num_pages++;
        list_add(&page->node, &next->list);

        parent = NULL;
        link = &et->root.rb_node;
        while (*link) 
        {
            curr = rb_entry(*link, struct extent, node);
            parent = *link;

            if (next->paddr_start < curr->paddr_start)
                link = &(*link)->rb_left;
            else
                link = &(*link)->rb_right;
        }

        rb_link_node(&next->node, parent, link);
        rb_insert_color(&next->node, &et->root);
        spin_unlock(&et->lock);
        return 0;
    
    // Case 4: Proceed to outside the switch case since this case
    // requires significantly more overhead than the others.
    // Separated so as to not burden the other 3 cases with unnecessary overhead.
    case EXTENT_NEW:
        break;

    default:
        spin_unlock(&et->lock);
        kfree(page);
        return -EINVAL;
    }

    // For the following case where it isn't contiguous, 
    // spinlock must be unlocked before calling kmalloc. 
    spin_unlock(&et->lock);

    // Case 4: If not contiguous, create a new extent
    new_ext = kmalloc(sizeof(*new_ext), GFP_KERNEL);
    if (!new_ext) 
    {
        kfree(page);
        return -ENOMEM;
    }

    new_ext->paddr_start = phys;
    new_ext->paddr_end = phys;
    new_ext->num_pages = 1;
    INIT_LIST_HEAD(&new_ext->list);
    list_add_tail(&page->node, &new_ext->list);

    spin_lock(&et->lock);

    new_ext->id = et->next_id++;

    prev = NULL;
    next = NULL;
    parent = NULL;
    link = &et->root.rb_node;

    /*
    The following is just a repeat of the initial steps
    and also checking for cases 1-3. This is because
    adding a new extent from case 4 could possibly change things
    so the process needs to be repeated again to get 
    the most up to date extent count and RB tree structure. 
    */

    while (*link) 
    {
        curr = rb_entry(*link, struct extent, node);
        parent = *link;

        if (phys < curr->paddr_start) {
            next = curr;
            link = &(*link)->rb_left;
        } else {
            prev = curr;
            link = &(*link)->rb_right;
        }
    }

    if (prev && phys == prev->paddr_end + PAGE_SIZE &&
        next && phys + PAGE_SIZE == next->paddr_start) 
    {
        insert_case = EXTENT_BRIDGE;
    } 
    else if (prev && phys == prev->paddr_end + PAGE_SIZE) 
    {
        insert_case = EXTENT_EXTEND_PREV;
    } 
    else if (next && phys + PAGE_SIZE == next->paddr_start) 
    {
        insert_case = EXTENT_EXTEND_NEXT;
    } 
    else 
    {
        insert_case = EXTENT_NEW;
    }

    switch (insert_case) 
    {
    case EXTENT_EXTEND_PREV:
        list_add_tail(&page->node, &prev->list);
        prev->paddr_end = phys;
        prev->num_pages++;
        spin_unlock(&et->lock);
        kfree(new_ext);
        return 0;

    case EXTENT_EXTEND_NEXT:
        rb_erase(&next->node, &et->root);

        next->paddr_start = phys;
        next->num_pages++;
        list_add(&page->node, &next->list);

        parent = NULL;
        link = &et->root.rb_node;
        while (*link) {
            curr = rb_entry(*link, struct extent, node);
            parent = *link;

            if (next->paddr_start < curr->paddr_start)
                link = &(*link)->rb_left;
            else
                link = &(*link)->rb_right;
        }

        rb_link_node(&next->node, parent, link);
        rb_insert_color(&next->node, &et->root);

        spin_unlock(&et->lock);
        kfree(new_ext);
        return 0;

    case EXTENT_BRIDGE:
        list_add_tail(&page->node, &prev->list);
        list_splice_tail_init(&next->list, &prev->list);

        prev->paddr_end = next->paddr_end;
        prev->num_pages += 1 + next->num_pages;

        rb_erase(&next->node, &et->root);
        kfree(next);
        et->total_extents--;

        spin_unlock(&et->lock);
        kfree(new_ext);
        return 0;

    case EXTENT_NEW:
        rb_link_node(&new_ext->node, parent, link);
        rb_insert_color(&new_ext->node, &et->root);
        et->total_extents++;
        spin_unlock(&et->lock);
        return 0;

    default:
        spin_unlock(&et->lock);
        kfree(new_ext);
        return -EINVAL;
    }

}

void extent_table_print_stats(struct extent_table *et, pid_t pid)
{
    if (!et)
        return;

    printk(KERN_INFO "PID %d Total Extents: %lu\n", pid, et->total_extents);
}

void extent_table_check_create(struct task_struct *task)
{
    struct extent_table *et;
    if (likely(task->et))
        return;

    et = extent_table_alloc();
    if (!et)
    {
        printk(KERN_ERR "Failed to allocate extent table for pid=%d\n", task->pid);
        return;
    }

    /* 
    Try to assign the new extent table to task->et only if it is still NULL.
    If task->et is already set (another thread got there first), then do not
    overwrite it and free this newly created table to avoid a memory leak.
    */
    if (cmpxchg(&task->et, NULL, et) != NULL) 
        extent_table_free(et);
}