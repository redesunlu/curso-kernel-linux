#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/stdio.h>
#include <linux/list.h>
//#include <asm-generic/uaccess.h>

#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/vt_kern.h>

#define MAX_SIZE_KBUF 100

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ProcFS ModList Kernel Module");
MODULE_AUTHOR("Illia & Fernandez");

struct list_head linked_list; /* Lista enlazada */

/* Nodos de la lista */
typedef struct {
  int data;
  struct list_head links;
} list_node_t;

static struct proc_dir_entry *proc_entry;

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  char kbuf[MAX_SIZE_KBUF];
  int val;

  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;

  if (len > MAX_SIZE_KBUF) {
    printk(KERN_INFO "Modlist: not enough space!!\n");
    return -ENOSPC;
  }

  /* Transfer data from user to kernel space */
  if (copy_from_user( &kbuf[0], buf, len ))
    return -EFAULT;

  kbuf[len] = '\0'; /* Add the `\0' */
  *off+=len;        /* Update the file pointer */

  if (sscanf(kbuf, "add %i", &val) == 1) {
    printk(KERN_INFO "Modlist: ADD %i\n", val);
    list_node_t* new_node;
    new_node = vmalloc(sizeof(list_node_t));

    if (new_node == NULL) {
      return -ENOMEM;
    }

    new_node->data = val;
    INIT_LIST_HEAD(&new_node->links);
    list_add_tail(&new_node->links, &linked_list);
  } else if (sscanf(kbuf, "remove %i", &val) == 1) {
    list_node_t *node;
    list_for_each_entry(node, &linked_list, links) {
      if (val == node-> data) {
        printk(KERN_INFO "Modlist: DEL %i\n", node->data);
        list_del(&node->links);
      }
    }
  } else if (sscanf(kbuf, "cleanup") == 1) {
    list_node_t *node;
    printk(KERN_INFO "Modlist: CLEAN\n");
    list_for_each_entry(node, &linked_list, links) {
      list_del(&node->links);
    }
  }

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  int nr_bytes;
  char kbuf[MAX_SIZE_KBUF];

  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;

  nr_bytes=strlen(kbuf);

  if (len<nr_bytes)
    return -ENOSPC;

    /* Transfer data from the kernel to userspace */
  if (copy_to_user(kbuf, buf, nr_bytes))
    return -EINVAL;

  (*off)+=len;  /* Update the file pointer */

  list_node_t *node;
  list_for_each_entry(node, &linked_list, links) {
    /* Muestra en dmesg, pero debería mostrar por stdout */
    sprintf(KERN_INFO "Modlist: %i\n", node->data);
  }

  return nr_bytes;
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,
};


int init_modlist_module( void )
{
  int ret = 0;
  proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO "Modlist: Can't create /proc entry\n");
  } else {
    INIT_LIST_HEAD(&linked_list);
    printk(KERN_INFO "Modlist: Module loaded\n");
  }

  return ret;
}


void exit_modlist_module( void )
{
  remove_proc_entry("modlist", NULL);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );

