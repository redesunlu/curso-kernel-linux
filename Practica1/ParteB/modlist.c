#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
//#include <asm-generic/uaccess.h>

#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

#define BUFFER_LENGTH PAGE_SIZE
#define MAX_SIZE_KBUF 100

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ProcFS ModList Kernel Module");
MODULE_AUTHOR("Illia & Fernandez");

struct list_head mylist; /* Lista enlazada */

/* Nodos de la lista */
typedef struct {
  int data;
  struct list_head links;
} list_item_t;

static struct proc_dir_entry *proc_entry;
static int *modlist;  // Space for the mod list

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  int kbuf[MAX_SIZE_KBUF];
  int val;
  
  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
  
  if (len > available_space) {
    printk(KERN_INFO "Modlist: not enough space!!\n");
    return -ENOSPC;
  }
  
  /* Transfer data from user to kernel space */
  if (copy_from_user( &kbuf[0], buf, len ))  
    return -EFAULT;

  mod_list[len] = '\0'; /* Add the `\0' */  
  *off+=len;            /* Update the file pointer */

  if (sscanf(kbuf, "add %i", &val) == 1) {
    list_item_t new_node = {
      .data = val,
      .links = LIST_HEAD_INIT(new_node.links),
    };

    list_add(&new_node->links, &mylist);   
  } else if (sscanf(kbuf, "remove %i", &val) == 1) {
    list_del();  
  } else if (sscanf(kbuf, "cleanup") == 1) {
    
  }

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  int nr_bytes;
  
  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;
    
  nr_bytes=strlen(modlist);
    
  if (len<nr_bytes)
    return -ENOSPC;
  
    /* Transfer data from the kernel to userspace */  
  if (copy_to_user(buf, modlist, nr_bytes))
    return -EINVAL;
    
  (*off)+=len;  /* Update the file pointer */

  return nr_bytes; 
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,    
};

int init_modlist_module( void )
{
  int ret = 0;
  modlist = (char *)vmalloc( BUFFER_LENGTH );

  if (!modlist) {
    ret = -ENOMEM;
  } else {
    memset( modlist, 0, BUFFER_LENGTH );
    proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
      ret = -ENOMEM;
      vfree(modlist);
      printk(KERN_INFO "Modlist: Can't create /proc entry\n");
    } else {
      INIT_LIST_HEAD(&mylist);
      printk(KERN_INFO "Modlist: Module loaded\n");
    }
  }
  return ret;
}


void exit_modlist_module( void )
{
  remove_proc_entry("modlist", NULL);
  vfree(modlist);
  printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );

