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

#include "usbvideo.h"
#include "cameraUsbDriver.h"

MODULE_LICENSE("Dual BSD/GPL");

//----Function prototypes-----
static int ele784_open (struct inode *inode, struct file *filp);
//static int ele784_release (struct inode *inode, struct file *filp);
static ssize_t ele784_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops);
long ele784_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static void ele784_cleanup(void);

static struct usb_device_id camera_id[] = {
{USB_DEVICE(0x046d, 0x08cc)}, //Vendor_id, Product_id
{USB_DEVICE(0x046d, 0x0994)},
{},
};
MODULE_DEVICE_TABLE(usb, camera_id); //usually used to support hot-plugging


struct usb_cameraData {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
    unsigned char           *control_buffer;	/* the buffer to receive data */
    size_t			control_size;		/* the size of the receive buffer */
	__u8			control_endpointAddr;	/* the address of the bulk in endpoint */
};


static struct usb_driver cameraUsb_driver;

/* OPEN */
static int ele784_open (struct inode *inode, struct file *filp){

    struct usb_interface *intf;
    int subminor;

    printk(KERN_WARNING "ELE784 -> Open \n\r");
    subminor = iminor(inode);

    //retrouve l'interface correspondant à un driver et a un Minor
    intf = usb_find_interface(&cameraUsb_driver, subminor);

    if (!intf) {
        printk(KERN_WARNING "ELE784 -> Open: Ne peux ouvrir le peripherique");
        return -ENODEV;
    }

    printk(KERN_WARNING "ELE784 -> Open terminé");
    filp->private_data = intf;
    return 0;

}

/* RELEASE */
/*
static int ele784_release (struct inode *inode, struct file *filp) {

	 printk(KERN_WARNING"ELE784 -> release \n\r");
	 return 0;
} */

/* READ */
static ssize_t ele784_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){

	printk(KERN_WARNING"ELE784 -> read \n\r");
   return 0;
}

/* IOCTL */
long ele784_ioctl (struct file *filp, unsigned int cmd, unsigned long arg){


    struct usb_interface *intf = filp->private_data;
    struct usb_cameraData *devData = usb_get_intfdata(intf);
    struct usb_device *dev = devData->udev;
    const struct usb_host_interface *iface_desc;
    char direction[4] = {0x00,0x00,0x00,0x00};
    long retval = 0;

    iface_desc = intf->cur_altsetting;

    switch(cmd){
        case IOCTL_GET : //Récupérer une valeur sur la caméra

				 printk(KERN_WARNING"ELE784 -> IOCTL_GET (0x10) \n\r");
             break;
        case IOCTL_SET : //Affecter une valeur sur la caméra

				 printk(KERN_WARNING"ELE784 -> IOCTL_SET (0x20) \n\r");
				 break;
        case IOCTL_STREAMON : // Démarrer l'acquisition d'une image

				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMON (0x30) \n\r");
             break;
        case IOCTL_STREAMOFF : // Arrêter l'acquisition d'une image

				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMOFF (0x40) \n\r");
			    break;
		  case IOCTL_GRAB :
				 printk(KERN_WARNING"ELE784 -> IOCTL_GRAB (0x50) \n\r");
			    break;
		  case IOCTL_PANTILT : // Modifier la position de l'objectif de la caméra
				 printk(KERN_WARNING"ELE784 -> IOCTL_PANTILT (0x60) arg = %d \n\r",(unsigned int)arg);
				 switch (arg){
                    case HAUT :     direction[0] = 0x00;
                                    direction[1] = 0x00;
                                    direction[2] = 0x80;
                                    direction[3] = 0xFF;
                                    break;
                    case BAS :      direction[0] = 0x00;
                                    direction[1] = 0x00;
                                    direction[2] = 0x80;
                                    direction[3] = 0x00;
                                    break;
                    case GAUCHE :   direction[0] = 0x80;
                                    direction[1] = 0x00;
                                    direction[2] = 0x00;
                                    direction[3] = 0x00;
                                    break;
                    case DROITE :   direction[0] = 0x80;
                                    direction[1] = 0xFF;
                                    direction[2] = 0x00;
                                    direction[3] = 0x00;
                                    break;                      //usb_sndctrlpipe(struct usb_device *dev, unsigned int endpoint)
				 }
			    retval = (long)usb_control_msg(dev,usb_sndctrlpipe(dev, 0x00), 0x01, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),0x0100,0x0900,&direction,4,0);

			    printk(KERN_WARNING"ELE784 -> IOCTL_PANTILT, Mouvement, arg = %d retval = %d  \n\r",arg,retval);
			    break;
		  case IOCTL_PANTILT_RESEST : // Reser de la position de l'objectif

				 printk(KERN_WARNING"ELE784 -> IOCTL_RESEST (0x70) \n\r");
			    break;

        default : return -ENOTTY;
    }

	 return retval;
}

struct file_operations ele784_fops =
{
    .owner = THIS_MODULE,
    .open = ele784_open,
    //.release = ele784_release,
    .read = ele784_read,
    .unlocked_ioctl = ele784_ioctl,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver ele784_class = {
	 .name = "cameraEle784num%d",
	 .fops = &ele784_fops,
	 .minor_base = 0,
};

/* PROBE */
static int ele784_probe(struct usb_interface *interface,const struct usb_device_id *devid){
	const struct usb_host_interface *iface_desc;
	const struct usb_endpoint_descriptor *endpoint;
	struct usb_cameraData *dev = NULL; //new private struct for each camera
	size_t buffer_size;

	dev = kzalloc (sizeof(struct usb_cameraData), GFP_KERNEL); //Allocation structure du device

	dev->udev = usb_get_dev (interface_to_usbdev(interface));   //interface_to_usbdev : recupere la struc usb_device du pilote usb
    dev->interface = interface;

	iface_desc = interface->cur_altsetting; //Récupère les réglages courants

    if(iface_desc->desc.bInterfaceClass == CC_VIDEO){
        if(iface_desc->desc.bInterfaceSubClass == SC_VIDEOCONTROL){
            endpoint = &iface_desc->endpoint[0].desc;
            printk(KERN_WARNING"ele784_probe (%s:%u)\n Récupération du endpoint, numero du endpoint = %d", __FUNCTION__, __LINE__,iface_desc->desc.bNumEndpoints);
            buffer_size = usb_endpoint_maxp(endpoint);
            dev->control_size = buffer_size;
            dev->control_endpointAddr = endpoint->bEndpointAddress;
            dev->control_buffer = kmalloc(buffer_size, GFP_KERNEL);
            //Sauvergarde du pointeur vers la structure perso dans l'interface de ce device
            usb_set_intfdata(interface, dev);

            //Enregistrement du device auprès de l'USB Core
            printk(KERN_WARNING"ele784_probe (%s:%u)\n Device registered", __FUNCTION__, __LINE__);
            usb_register_dev(interface, &ele784_class);
        }
        else{
            printk(KERN_WARNING"ele784_probe (%s:%u)\n Pas la bonne SubClass", __FUNCTION__, __LINE__);
        }

    }
    else{
        printk(KERN_WARNING"ele784_probe (%s:%u)\n Pas la bonne Class", __FUNCTION__, __LINE__);
    }

    return 0;

}


/* DISCONNECT */
static void ele784_disconnect(struct usb_interface *interface){

    struct usb_cameraData *dev; //private struct for each camera

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

    //Signifie au USB Core que le device retiré n'est plus associé à l'usb driver
	usb_deregister_dev(interface, &ele784_class);
	printk(KERN_WARNING"ele784_probe (%s:%u)\n Pas la bonne Class", __FUNCTION__, __LINE__);

}


static struct usb_driver cameraUsb_driver = {
	.name =		"cameraUsbDriver",
	.probe =	ele784_probe,
	.disconnect =	ele784_disconnect,
	.id_table =	camera_id,
};

module_usb_driver(cameraUsb_driver);
