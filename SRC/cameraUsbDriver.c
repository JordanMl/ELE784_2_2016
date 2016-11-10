///Includes d'origine
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

///Nouveaux includes :
#include <linux/rwsem.h>  ///semaphore lecteur/ecrivain
#include <linux/sched.h> // Required for task states (TASK_INTERRUPTIBLE etc )
#include <linux/ioctl.h> // Used for ioctl command

#include "cameraUsbDriver.h" 

MODULE_LICENSE("Dual BSD/GPL");


static struct usb_device_id camera_id[] = {
{USB_DEVICE(0x046d, 0x08cc)}, //Vendor_id, Product_id
{},
};
MODULE_DEVICE_TABLE(usb, camera_id);



//----Function prototypes-----
static int ele784_open (struct inode *inode, struct file *filp);
static int ele784_release (struct inode *inode, struct file *filp);
static ssize_t ele784_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops);
static int ele784_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static void ele784_cleanup(void);


static struct usb_driver cameraUsb_driver;

/* OPEN */
static int ele784_open (struct inode *inode, struct file *filp){
	
   printk(KERN_WARNING"ELE784 -> open \n\r");
	return 0;
}

/* RELEASE */
static int ele784_release (struct inode *inode, struct file *filp) {

	 printk(KERN_WARNING"ELE784 -> release \n\r");
	 return 0;
}

/* READ */
static ssize_t ele784_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){

	printk(KERN_WARNING"ELE784 -> read \n\r");
   return 0;
}

/* IOCTL */
static int ele784_ioctl (struct file *filp, unsigned int cmd, unsigned long arg){


    switch(cmd){
        case IOCTL_GET :  
				 printk(KERN_WARNING"ELE784 -> IOCTL_GET (0x10) \n\r");
             break;
        case IOCTL_SET :
				 printk(KERN_WARNING"ELE784 -> IOCTL_SET (0x20) \n\r");
				 break;
        case IOCTL_STREAMON :
				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMON (0x30) \n\r"); 
             break;
        case IOCTL_STREAMOFF : 
				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMOFF (0x40) \n\r"); 
			    break;
		  case IOCTL_GRAB : 
				 printk(KERN_WARNING"ELE784 -> IOCTL_GRAB (0x50) \n\r"); 
			    break;
		  case IOCTL_PANTILT : 
				 printk(KERN_WARNING"ELE784 -> IOCTL_PANTILT (0x60) \n\r"); 
			    break;
		  case IOCTL_PANTILT_RESEST : 
				 printk(KERN_WARNING"ELE784 -> IOCTL_RESEST (0x70) \n\r"); 
			    break;

        default : return -ENOTTY;
    }

	 return 0;
}

/* CLEANUP */
static void ele784_cleanup(void){

	// unregister_chrdev_region(…)
}


static struct usb_driver cameraUsb_driver = {
	.name =		"cameraUsbDriver",
	//.probe =	ele784_probe,
	.unlocked_ioctl = ele784_ioctl,
	.id_table =	camera_id,
};

module_usb_driver(cameraUsb_driver);
