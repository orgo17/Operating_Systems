#include <linux/kernel.h>
#include <asm/current.h>
#include <asm/errno.h>
#include <linux/sched.h>
#include <linux/list.h>


asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

asmlinkage long sys_set_weight(int weight) {
    if(weight < 0) {
        return -EINVAL;
    }
    current->weight = weight;
    return 0;
}

asmlinkage long sys_get_weight(void) {
    return current->weight;
}

long aux_get_leaf_children_sum(struct task_struct *task) {
    if(list_empty(&(task->children))) {
        return task->weight;
    }
    long leaf_sum = 0;
    struct list_head *list;
    list_for_each(list, &(task->children)) {
        struct task_struct *child_task = list_entry(list, struct task_struct, sibling);
        /* child_task now points to one of current’s children */
        leaf_sum += aux_get_leaf_children_sum(child_task);
    }
    return leaf_sum;
}

asmlinkage long sys_get_leaf_children_sum(void) {
    if(list_empty(&current->children)) {
        return -ECHILD;
    }
    long sum = aux_get_leaf_children_sum(current);
    return sum;
}

asmlinkage long sys_get_heaviest_ancestor(void) {
    struct task_struct *tmp = current;
    int max_weight = current->weight;
    long heaviest_pid = current->pid;
    while(tmp->pid != 1) {
        if(tmp->weight > max_weight) {
            max_weight = tmp->weight;
            heaviest_pid = tmp->pid;
        }
        tmp = tmp->real_parent;
    }
    return tmp->weight > max_weight ? tmp->pid : heaviest_pid;
}