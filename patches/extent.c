// This file is located in linux-5.15.0/mm/extent.c

#include <linux/extent.h>
#include <linux/printk.h>
#include <linux/sched.h>

void extent_init_passed() {
    if (!current->extent_init_passed) {
        printk(KERN_INFO "Extent Initial Page Fault: Passed | PID: %d (%s)!\n", current->pid, current->comm);
        current->extent_init_passed = true; 
    }
}