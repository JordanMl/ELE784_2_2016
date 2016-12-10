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
long ele784_grab(struct usb_interface *intf, struct usb_device *dev);
static void complete_callback(struct urb *urb);

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
	struct completion *done;
    int open_count;
	struct urb *myUrb[5];
};


unsigned int myStatus = 0;
unsigned int myLength = 42666;
unsigned int myLengthUsed = 0;
char myData[42666];

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

static int ele784_release (struct inode *inode, struct file *filp) {

	 printk(KERN_WARNING"ELE784 -> release \n\r");
	 return 0;
}

/* READ */
static ssize_t ele784_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){
    struct usb_cameraData *camData;
    struct usb_interface *intf;
    int i;

    intf = filp->private_data;
    camData = usb_get_intfdata(intf);

    printk(KERN_WARNING "ELE784 -> read -> attente de l'urb callback completion...\n");
    wait_for_completion(camData->done);
    printk(KERN_WARNING "ELE784 -> read -> Completion OK");

    count = copy_to_user(ubuf, myData, myLengthUsed);

    if(count < 0){
        count = -EFAULT;
    }

    for (i = 0; i < 5; i++){
        printk(KERN_WARNING "ELE784 -> read -> kill urb (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_kill_urb(camData->myUrb[i]);

        //desalocation du buffer de transfert
        printk(KERN_WARNING "ELE784 -> read -> usb_free_coherent (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_free_coherent(camData->udev, camData->myUrb[i]->transfer_buffer_length, camData->myUrb[i]->transfer_buffer,
                            camData->myUrb[i]->transfer_dma);

        camData->myUrb[i]->transfer_buffer_length = 0;
        camData->myUrb[i]->transfer_flags = URB_FREE_BUFFER;

        printk(KERN_WARNING "ELE784 -> read -> usb_free_urb (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_free_urb(camData->myUrb[i]);

        camData->myUrb[i] = NULL;
    }

	printk(KERN_WARNING"ELE784 -> read done \n\r");

   return myLengthUsed - count; //nombre de bytes retournes
}

/* IOCTL */
long ele784_ioctl (struct file *filp, unsigned int cmd, unsigned long arg){


    struct usb_interface *intf = filp->private_data;
    struct usb_cameraData *camData = usb_get_intfdata(intf);
    struct usb_device *dev = camData->udev;
    const struct usb_host_interface *iface_desc;
    char direction[4] = {0x00,0x00,0x00,0x00};
    long retval = 0;
    char reset = 0x03;

    iface_desc = intf->cur_altsetting;

    switch(cmd){

        case IOCTL_STREAMON : // Démarrer l'acquisition d'une image
                retval = (long)usb_control_msg(dev,usb_sndctrlpipe(dev, 0x00), 0x0B, (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),0x0004,0x0001,NULL,0,0);
				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMON (0x30) \n\r");
             break;
        case IOCTL_STREAMOFF : // Arrêter l'acquisition d'une image
                retval = (long)usb_control_msg(dev,usb_sndctrlpipe(dev, 0x00), 0x0B, (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),0x0000,0x0001,NULL,0,0);
				 printk(KERN_WARNING"ELE784 -> IOCTL_STREAMOFF (0x40) \n\r");
			    break;
		  case IOCTL_GRAB :
                 retval = ele784_grab(intf, dev);
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

			    printk(KERN_WARNING"ELE784 -> IOCTL_PANTILT, Mouvement, arg = %ld retval = %ld  \n\r",arg,retval);
			    break;
		  case IOCTL_PANTILT_RESEST : // Reset de la position de l'objectif
                retval = (long)usb_control_msg(dev,usb_sndctrlpipe(dev, 0x00), 0x01, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),0x0200,0x0900,&reset,1,0);
				 printk(KERN_WARNING"ELE784 -> IOCTL_RESEST (0x70) \n\r");
			    break;
          /*case IOCTL_GET : //Récupérer une valeur sur la caméra

				 printk(KERN_WARNING"ELE784 -> IOCTL_GET (0x10) \n\r");
                break;
          case IOCTL_SET : //Affecter une valeur sur la caméra

				 printk(KERN_WARNING"ELE784 -> IOCTL_SET (0x20) \n\r");
				 break;*/
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
	struct usb_cameraData *camData = NULL; //new private struct for each camera
	size_t buffer_size;
	int i;

	camData = kzalloc (sizeof(struct usb_cameraData), GFP_KERNEL); //Allocation structure du device

	camData->udev = usb_get_dev (interface_to_usbdev(interface));   //interface_to_usbdev : recupere la struc usb_device du pilote usb
    camData->interface = interface;

	iface_desc = interface->altsetting;

    if(iface_desc->desc.bInterfaceClass == CC_VIDEO){
        if(iface_desc->desc.bInterfaceSubClass == SC_VIDEOCONTROL){
        //if(iface_desc->desc.bInterfaceSubClass == SC_VIDEOSTREAMING){
            endpoint = &iface_desc->endpoint[0].desc;
            printk(KERN_WARNING"ele784_probe (%s:%u)\n Récupération du endpoint, numero du endpoint = %d", __FUNCTION__, __LINE__,iface_desc->desc.bNumEndpoints);
            buffer_size = usb_endpoint_maxp(endpoint);
            camData->control_size = buffer_size;
            camData->control_endpointAddr = endpoint->bEndpointAddress;
            camData->control_buffer = kmalloc(buffer_size, GFP_KERNEL);

            for(i = 0; i < 5; i++){
                camData->myUrb[i] = NULL;
            }

            camData->open_count = 0;
            camData->done = (struct completion *) kmalloc(sizeof(struct completion), GFP_KERNEL);
            init_completion(camData->done);

            //Sauvergarde du pointeur vers la structure perso dans l'interface de ce device
            usb_set_intfdata(interface, camData);

            //Enregistrement du device auprès de l'USB Core
            printk(KERN_WARNING"ele784_probe (%s:%u)\n Device registered", __FUNCTION__, __LINE__);
            usb_register_dev(interface, &ele784_class);

            usb_set_interface(camData->udev, 1, 4);
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

    struct usb_cameraData *camData; //private struct for each camera

	camData = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

    //Signifie au USB Core que le device retiré n'est plus associé à l'usb driver
	usb_deregister_dev(interface, &ele784_class);
	printk(KERN_WARNING"ele784_disconnect (%s:%u)\n", __FUNCTION__, __LINE__);

}


/* GRAB*/
long ele784_grab(struct usb_interface *intf, struct usb_device *dev) {
  int nbPackets;
  int myPacketSize;
  int size;
  int nbUrbs;
  int i;
  int ret;
  int j;

  struct usb_endpoint_descriptor *endpointDesc;
  struct usb_cameraData *camData = usb_get_intfdata(intf);

  //endpointDesc = &intf->cur_altsetting->endpoint[0].desc; //il faut pas enlever le & ?
  endpointDesc = &intf->cur_altsetting->endpoint[0].desc;

  nbPackets = 40;  // The number of isochronous packets this urb should contain
  myPacketSize = le16_to_cpu(endpointDesc->wMaxPacketSize);
  size = myPacketSize * nbPackets;
  nbUrbs = 5;

  for (i = 0; i < nbUrbs; ++i) {
    usb_free_urb(camData->myUrb[i]);
    camData->myUrb[i] = usb_alloc_urb(nbPackets, GFP_KERNEL);
    if (camData->myUrb[i] == NULL) {
      printk(KERN_WARNING "ele784_grab (%s:%u) | URB[%d] : NULL  \n",__FUNCTION__, __LINE__,i);
      return -ENOMEM;
    }

    camData->myUrb[i]->transfer_buffer = usb_alloc_coherent(dev, size, GFP_KERNEL, &camData->myUrb[i]->transfer_dma);

    if (camData->myUrb[i]->transfer_buffer == NULL) {
      printk(KERN_WARNING "ele784_grab (%s:%u) | URB[%d] free \n",__FUNCTION__, __LINE__,i);
      usb_free_urb(camData->myUrb[i]);
      return -ENOMEM;
    }

    camData->myUrb[i]->dev = dev;
    camData->myUrb[i]->context = camData;
    camData->myUrb[i]->pipe = usb_rcvisocpipe(dev, endpointDesc->bEndpointAddress);
    camData->myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
    camData->myUrb[i]->interval = endpointDesc->bInterval;
    camData->myUrb[i]->complete = complete_callback;
    camData->myUrb[i]->number_of_packets = nbPackets;
    camData->myUrb[i]->transfer_buffer_length = size;

    for (j = 0; j < nbPackets; ++j) {
      camData->myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
      camData->myUrb[i]->iso_frame_desc[j].length = myPacketSize;
    }
  }

  for(i = 0; i < nbUrbs; i++){
    if ((ret = usb_submit_urb(camData->myUrb[i], GFP_KERNEL)) < 0) {
      printk(KERN_WARNING "ele784_grab (%s:%u) | submit URB[%d]  \n",__FUNCTION__, __LINE__,i);
      return ret;
    }
  }

  return 0;


}

/*************************************************************************
 * Source: le pilote uvcvideo (voir le répertoire /home/ELE784/uvc
 *         pour les fichiers sources)
 ************************************************************************/

static void complete_callback(struct urb *urb){

	int ret;
	int i;
	unsigned char * data;
	unsigned int len;
	unsigned int maxlen;
	unsigned int nbytes;
	void * mem;

    struct usb_cameraData *camData = urb->context;

    printk("CameraUsbDriver - CALLBACK - Start\n");

	if(urb->status == 0){

		for (i = 0; i < urb->number_of_packets; ++i) {
			if(myStatus == 1){
				continue;
			}
			if (urb->iso_frame_desc[i].status < 0) {
				continue;
			}

			data = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
			if(data[1] & (1 << 6)){
				continue;
			}
			len = urb->iso_frame_desc[i].actual_length;
			if (len < 2 || data[0] < 2 || data[0] > len){
				continue;
			}

			len -= data[0];
			maxlen = myLength - myLengthUsed ;
			mem = myData + myLengthUsed;
			nbytes = min(len, maxlen);
			memcpy(mem, data + data[0], nbytes);
			myLengthUsed += nbytes;

			if (len > maxlen) {
                printk(KERN_WARNING "DONE\n");
				myStatus = 1; // DONE
			}

			/* Mark the buffer as done if the EOF marker is set. */
			if ((data[1] & (1 << 1)) && (myLengthUsed != 0)) {
                printk(KERN_WARNING "DONE\n");
				myStatus = 1; // DONE
			}
		}

		if (!(myStatus == 1)){
			if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
				printk(KERN_WARNING "Erreur dans submit URB code %d\n",ret);
			}
			printk(KERN_WARNING "RESUBMIT URB\n");
		}else{
			///////////////////////////////////////////////////////////////////////
			//  Synchronisation
			///////////////////////////////////////////////////////////////////////
			printk(KERN_WARNING "Synchronisation \n");
			camData->open_count += 1;
			myStatus = 0;
			if(camData->open_count == 5){
                complete(camData->done);
                camData->open_count = 0;
			}
		}
	}else{
		printk(KERN_WARNING "Pas rentré dans le callback");
	}
}

static struct usb_driver cameraUsb_driver = {
	.name =		"cameraUsbDriver",
	.probe =	ele784_probe,
	.disconnect =	ele784_disconnect,
	.id_table =	camera_id,
};

module_usb_driver(cameraUsb_driver);
