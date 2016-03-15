#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
//#include <asm-generic/uaccess.h>

#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

#define ALL_LEDS_ON   0x7
#define ALL_LEDS_OFF  0
#define BUFFER_LENGTH PAGE_SIZE

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ProcFS Keyboard LEDs Kernel Module");
MODULE_AUTHOR("Illia & Fernandez");

struct tty_driver* kbd_driver= NULL;
static struct proc_dir_entry *proc_entry;
static char *led_mask;  // Space for the led mask

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void)
{
   printk(KERN_INFO "Procleds: loading\n");
   printk(KERN_INFO "Procleds: fgconsole is %x\n", fg_console);
   return vc_cons[fg_console].d->port.tty->driver;
}

/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask)
{
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}

static ssize_t procleds_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  int available_space = BUFFER_LENGTH-1;
  
  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
  
  if (len > available_space) {
    printk(KERN_INFO "Procleds: not enough space!!\n");
    return -ENOSPC;
  }
  
  /* Transfer data from user to kernel space */
  if (copy_from_user( &led_mask[0], buf, len ))  
    return -EFAULT;

  led_mask[len] = '\0'; /* Add the `\0' */  
  *off+=len;            /* Update the file pointer */

  unsigned int new_mask = 0x0;
  int i;
  for (i = 0; i < len - 1; i++) {
    switch (led_mask[i]){
      case '1': 
        new_mask |= (1 << 1);
        break;
      case '2':
        new_mask |= (1 << 2);
        break;
      case '3':
        new_mask |= (1);
        break;
    }
  }
  printk(KERN_INFO "Procleds: NEW MASK = %x\n", new_mask);
  
  if (set_leds(kbd_driver, new_mask))
     return -EPERM;
  else 
     return len;
}

static ssize_t procleds_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  return -EPERM; 
}

static const struct file_operations proc_entry_fops = {
    .read = procleds_read,
    .write = procleds_write,    
};

int init_procleds_module( void )
{
  int ret = 0;
  led_mask = (char *)vmalloc( BUFFER_LENGTH );

  if (!led_mask) {
    ret = -ENOMEM;
  } else {
    memset( led_mask, 0, BUFFER_LENGTH );
    proc_entry = proc_create( "leds", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
      ret = -ENOMEM;
      vfree(led_mask);
      printk(KERN_INFO "Procleds: Can't create /proc entry\n");
    } else {
      kbd_driver= get_kbd_driver_handler();
      printk(KERN_INFO "Procleds: Module loaded\n");
    }
  }
  return ret;
}


void exit_procleds_module( void )
{
  remove_proc_entry("leds", NULL);
  vfree(led_mask);
  printk(KERN_INFO "Procleds: Module unloaded.\n");
}


module_init( init_procleds_module );
module_exit( exit_procleds_module );

