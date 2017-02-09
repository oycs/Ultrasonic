#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>

static struct class *ultrasonic_class;
static struct class_device *ultrasonic_class_device;
static char gCount=1;
static volatile int ev_press = 0;
static wait_queue_head_t buttons_queue;
static irqreturn_t buttons_handler(int irq, void *dev_id)
{
	wake_up_interruptible(&buttons_queue);
	ev_press=1;
	gCount++;
	return IRQ_HANDLED;
}

static ssize_t ultrasonicdrv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char key_val[1];
	key_val[0]=gCount;
	wait_event_interruptible(buttons_queue,ev_press);
	ev_press=0;
	if(copy_to_user(buf,key_val,sizeof(key_val)))
		return -1;
	return sizeof(key_val);
}

static int ultrasonicdrv_open(struct inode *inode, struct file *file)
{
	if(request_irq(IRQ_EINT0,buttons_handler,IRQF_TRIGGER_RISING,"s2",NULL)!=0)
	{	
		printk("request error\n");
		return -1;
	}
	
	init_waitqueue_head(&buttons_queue);
	return 0;
}

int ultrasonicdrv_release(struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT0,NULL);
	return 0;
}

static struct file_operations ultrasonic_fops={
	.owner = THIS_MODULE,
	.read  = ultrasonicdrv_read,
	.open  = ultrasonicdrv_open,
	.release = ultrasonicdrv_release,
};

int major;
static int ultrasonic_drv_init(void)
{
	major=register_chrdev(0,"ultrasonicdrv",&ultrasonic_fops);
	ultrasonic_class=class_create(THIS_MODULE,"ultrasonicdrv");
	ultrasonic_class_device=class_device_create(ultrasonic_class,NULL,MKDEV(major,0),NULL,"ultrasonic");
	
	return 0;
}

static void ultrasonic_drv_exit(void)
{
	unregister_chrdev(major,"ultrasonicdrv");
	class_destroy(ultrasonic_class);
	class_device_unregister(ultrasonic_class_device);
}

module_init(ultrasonic_drv_init);
module_exit(ultrasonic_drv_exit);
MODULE_LICENSE("GPL");

