/*
 * linux/drivers/char/ircontrol.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/poll.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include<linux/timer.h> 
#include<linux/jiffies.h>
#include <linux/spinlock.h>

//////
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <asm/atomic.h>

#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/wakelock.h>
#include <linux/rockchip/iomap.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
//#include <asm/fpu.h>


/* 定义幻数 */
#define CW0881_IOC_MAGIC  'G'

/* 定义命令 */
#define CW0881_IOCGETRESULT _IOWR(CW0881_IOC_MAGIC, 77, int)
#define CW0881_IOTEST _IOWR(CW0881_IOC_MAGIC, 78, int)
#define CW0881_POWER_ON _IOWR(CW0881_IOC_MAGIC, 104, int)
#define CW0881_POWER_OFF _IOWR(CW0881_IOC_MAGIC, 105, int)

#define CW0881_STOP		_IOWR(CW0881_IOC_MAGIC, 100, int)
#define CW0881_START		_IOWR(CW0881_IOC_MAGIC, 101, int)
#define CW0881_READ_BYTE	_IOWR(CW0881_IOC_MAGIC, 102, int)
#define CW0881_WRITE_BYTE	_IOWR(CW0881_IOC_MAGIC, 103, int)
#define CW0881_ACK_NACK	_IOWR(CW0881_IOC_MAGIC, 106, int)


typedef unsigned char uchar;

static int bPass = 0;
unsigned char cw0881_work(void)	;

extern  uchar MTZ_test(void);
extern void init_gpio();
extern void deinit_gpio(void);
extern void cm_AckNak(uchar ucAck);
extern void i2c_stop(void);
extern void i2c_start(void);
extern uchar read_byte(void);
extern uchar write_byte(uchar byte);


static long cw0881_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case CW0881_IOCGETRESULT:
		{

			msleep(50);
			//if (copy_to_user((void *)arg, data, sizeof(int)*4))  return -EFAULT;
			//printk("bPass = %u\n", bPass);
			return 0;//bPass;
	    }
		break;
		case CW0881_IOTEST:
		{
			MTZ_test();
		}
		break;
		case CW0881_POWER_ON:
		{
			init_gpio();
		}
		break;
		case CW0881_POWER_OFF:
		{
			deinit_gpio();
		}
		break;
		case CW0881_START:
		{	
			i2c_start();
		}
		break;
		case CW0881_STOP:
		{
			i2c_stop();
		}
		break;
		case CW0881_READ_BYTE:
		{
			return read_byte();
		}
		break;
		case CW0881_WRITE_BYTE:
		{
			return write_byte(arg);
		}
		break;
		case CW0881_ACK_NACK:
		{
			cm_AckNak(arg);
		}
		break;
			
	}
	return 0;
}

static void initGpio(){

	
	//for test gpio 
	printk("gpio cw881 init 92, 94 ...\n");
	if (gpio_request(0,"gpio0"))			printk("0 gpio_request() err\n");
	if (gpio_request(1,"gpio1"))			printk("1 gpio_request() err\n");
	if (gpio_request(51,"gpio51"))			printk("51 gpio_request() err\n");  //gpio1_c3

	printk("gpio cw881 init 92, 94\n");
	gpio_direction_output(0, 0);
	gpio_direction_output(1, 0);
	gpio_direction_output(51, 0);
	msleep(200);
	printk("gpio cw881 init 92, 94 \n");

	//bPass= cw0881_work();	

}


static struct file_operations cw0881_ops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	=	cw0881_ioctl,
	
};

static struct miscdevice cw0881_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cw0881",
	.fops = &cw0881_ops,
	.mode   = 0666,
};

static int __init cw0881_dev_init(void) {
	int ret;
	initGpio();
	if((ret = misc_register(&cw0881_misc_dev))) {
		printk("cw0881: misc_register register failed\n");
	}


	//printk("cw0881 initialized\n");

	return ret;
}


static void __exit cw0881_dev_exit(void) {
	misc_deregister(&cw0881_misc_dev);	
}

module_init(cw0881_dev_init);
module_exit(cw0881_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OKL Inc.");
MODULE_DESCRIPTION("Cw0881 Driver");
