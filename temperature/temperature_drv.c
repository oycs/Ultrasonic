#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>

static struct class *temperature_class;
static struct class_device *temperature_class_device;

volatile unsigned long *GPFCON=NULL;
volatile unsigned long *GPFDAT=NULL;

static int temperature_drv_open(struct inode *inode, struct file *file)
{
	printk("open\n");
	*GPFCON|=(1<<4)|(1<<5)|(1<<6);
	return 0;
}

static ssize_t temperature_drv_write(struct file *file, const char __user *buf, size_t count, loff_t *loff)
{
	int num;
	if(copy_from_user(&num,buf,4))
	  return -1;
	switch(num)
	{
		case 1:
			*GPFDAT&=~(1<<4);
			break;
		case 2:
			*GPFDAT&=~(1<<5);
			break;
		case 3:
			*GPFDAT&=~(1<<6);
			break;
		case 4:
			*GPFDAT&=~((1<<4)|(1<<5)|(1<<6));
			break;
		case 5:
			*GPFDAT|=(1<<4)|(1<<5)|(1<<6);
			break;
		default:
			break;
	
	}
	return 0;
}

struct file_operations  temperature_drv_fops={
	.owner   = THIS_MODULE,
	.open    = temperature_drv_open,
	.write   = temperature_drv_write,
};
int major;
static int temperature_drv_init(void)
{
	major=register_chrdev(0,"temperature_drv",&temperature_drv_fops);
	
	temperature_class=class_create(THIS_MODULE,"temperaturedrv");
	if(IS_ERR(temperature_class))
		return PTR_ERR(temperature_class);
	temperature_class_device=class_device_create(temperature_class,NULL,MKDEV(major,0),NULL,"temperature");
	if(unlikely(IS_ERR(	temperature_class_device)))
		return PTR_ERR(	temperature_class_device);

	GPFCON=(unsigned long *)ioremap(0x56000050,16);                                       //GPF4,5,6
	GPFDAT=GPFCON+1;

	return 0;
}

static void temperature_drv_exit(void)
{
	unregister_chrdev(major,"temperature_drv");
	class_destroy(temperature_class);
	class_device_unregister(temperature_class_device);
	iounmap(GPFCON);
}

module_init(temperature_drv_init);
module_exit(temperature_drv_exit);
MODULE_LICENSE("GPL");
