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


/* 定义幻数 */
#define CARDS_IOC_MAGIC  'k'

/* 定义命令 */
#define CARDS_IOCDATA   _IO(CARDS_IOC_MAGIC, 1)
#define CARDS_IOCGETSECTOR _IOWR(CARDS_IOC_MAGIC, 2, int)
#define CARDS_IOCSETDATA _IOW(CARDS_IOC_MAGIC, 3, int)
#define CARDS_IOCSWITCHKEY _IOWR(CARDS_IOC_MAGIC, 4, int)
#define CARDS_IOCSWITCHCARD _IOWR(CARDS_IOC_MAGIC, 5, int)
#define CARDS_IOCSETOFFSET _IOWR(CARDS_IOC_MAGIC, 6, int)

#define CARDS_IOCSTART_WIRELESSKEY_SCAN _IOWR(CARDS_IOC_MAGIC, 7, int)
#define CARDS_IOCSTOP_WIRELESSKEY_SCAN 	_IOWR(CARDS_IOC_MAGIC, 8, int)
#define CARDS_IOC_CARD_GPIO_INIT 			_IOWR(CARDS_IOC_MAGIC, 9, int)


//for gpio test
#define CARDS_IOC_OPERA_ONE_GPIO_WR _IOWR(CARDS_IOC_MAGIC, 20, int)
#define CARDS_IOC_OPERA_BUS_GPIO_WR  _IOWR(CARDS_IOC_MAGIC, 21, int)


#define CARDS_IOC_MAXNR 3

#define CARD_D0     2    //GPIO0_A2 //32   //GPIO1_A0
#define CARD_D1     3    //GPIO0_A3 //33   //GPIO1_A1
#define CARD_D2     8	 //GPIO0_B0 //34   //GPIO1_A2
#define CARD_D3     9	 //GPIO0_B1 //35   //GPIO1_A3
#define CARD_D4     11	 //GPIO0_B3 //37   //GPIO1_A5
#define CARD_D5     12	 //GPIO0_B4 //36   //GPIO1_A4
#define CARD_D6     13	 //GPIO0_B5 //94   //GPIO2_D6
#define CARD_D7     14	 //GPIO0_B6 //93   //GPIO2_D5
#define CARD_D8     16	 //GPIO0_C0  //92   //GPIO2_D4
#define CARD_D9     17	 //GPIO0_C1  //82   //GPIO2_C2
#define CARD_D10    18	 //GPIO0_C2  //81   //GPIO2_C1
#define CARD_D11    19	 //GPIO0_C3  //80   //GPIO2_C0
#define CARD_D12    20	 //GPIO0_C4  //89   //GPIO2_D1
#define CARD_D13    26   //GPIO0_D2  //78   //GPIO2_B6
#define CARD_D14    27   //GPIO0_D3  //77   //GPIO2_B5
#define CARD_D15    28   //GPIO0_D4  //76   //GPIO2_B4


#define CARD_A0     71   //GPIO2_A7 //53   //GPIO1_C5
#define CARD_A1     72	 //GPIO2_B0	//52   //GPIO1_C4
#define CARD_A2     73   //GPIO2_B1	//GPIO1_C1
#define CARD_A3     74   //GPIO2_B2	//		48   //GPIO1_C0
#define CARD_A4     75   //GPIO2_B3	//	47   //GPIO1_B7
#define CARD_A5     76   //GPIO2_B4	//8    //GPIO0_B0
#define CARD_A6     77   //GPIO2_B5	//13   //GPIO0_B5
#define CARD_A7     78   //GPIO2_B6	//9    //GPIO0_B1
#define CARD_A8     79   //GPIO2_B7	//27   //GPIO0_D3
#define CARD_A9     80	 //GPIO2_C0	//26   //GPIO0_D2
#define CARD_A10    81	 //GPIO2_C1 //14   //GPIO0_B6
#define CARD_A11    82	 //GPIO2_C2 //17   //GPIO0_C1
#define CARD_A12    83	 //GPIO2_C3 //84   //GPIO2_C4
#define CARD_A13    84	 //GPIO2_C4 //0    //GPIO0_A0
#define CARD_A14    85	 //GPIO2_C5 //1    //GPIO0_A1
#define CARD_A15    86	 //GPIO2_C6 //2    //GPIO0_A2

#define CARD_A16    87	 //GPIO2_C7 //3    //GPIO0_A3  ---
#define CARD_A17    89	 //GPIO2_D1		//89   //GPIO2_C5
#define CARD_A18    92	 //GPIO2_D4 //28   //GPIO0_D4
#define CARD_A19    93   //GPIO2_D5
#define CARD_A20    94   //GPIO2_D6 //73   //GPIO2_B1
#define CARD_A21    47   //GPIO1_B7	// 75   //GPIO2_B3

#define CARD_OE     48   //GPIO1_C0   //86   //GPIO2_C6

#define BANK_GPIO_OPERA

int io2keys;
static int cardaddress;
extern void startScankey();
extern void stopScankey();
extern void startWirelessScankey();
extern void stopWirelessScankey();

extern int gpio_bank_get_value(unsigned gpio);
extern int gpio_bank_set_value(unsigned gpio,int value, unsigned int mask);
extern int gpio_bank_set_direction(unsigned gpio,int dir, unsigned int mask);

static int getAddrValue(int tempaddr);
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

static void initGpio();
static void initGpioDirection();
/***
function: 上层调用CARDS_IOCGETSTOR,读入一个扇区512byte的字节做数据分析和处理。
          如果数据是对的，代表大卡有打开插入，依次读读取全部数据，读取完成，
		  调用CARDS_IOCSWITCHKEY，把IO切换成按键复用
****/
static long cards_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg){
		int i;
		unsigned char data[512];
        int offset;
		int offsetval;
		unsigned short value;
		switch(cmd){
			case CARDS_IOCGETSECTOR:{
				for(i=0;i<256;i++){
					value=getAddrValue(cardaddress);
					data[2*i+1] = LO_UINT16(value);
					data[2*i] = HI_UINT16(value);
					cardaddress++;
				}
				if (copy_to_user((void*)arg, data, sizeof(data))){
					return -EFAULT;
				}
			}
			break;
         
            case CARDS_IOCSETOFFSET:{
				if(copy_from_user(&offset, (void*)arg, sizeof(offset))) {
					printk("copy_from_user failed\n");
					return -EFAULT;
				}
				cardaddress = offset;
				/*offsetval = getAddrValue(offset);
				if (copy_to_user((void*)arg, &offsetval, sizeof(offsetval))){
					return -EFAULT;
				}*/
			}
			break;
			case CARDS_IOCSWITCHKEY:{
				cardaddress=0;
				startScankey();
			}
			break;
			
			case CARDS_IOCSWITCHCARD:{
				cardaddress=0;
				stopScankey();
				initGpio();
			}
			break;
			case CARDS_IOCSTART_WIRELESSKEY_SCAN:{
				//cardaddress=0;
				startWirelessScankey();
			}
			break;
			
			case CARDS_IOCSTOP_WIRELESSKEY_SCAN:{
				//cardaddress=0;
				stopWirelessScankey();
				initGpio();
			}			
			break;
			case CARDS_IOC_CARD_GPIO_INIT: {
				initGpioDirection();
			}
			break;
			case CARDS_IOC_OPERA_ONE_GPIO_WR: //format : byte 0: gpio number, byte 1: rw flag, byte 2: write data
			{
				//(unsigned char *)data = (unsigned char *)arg; 
				int gpio;
				int bRead;
				int value;
				int ret;
				char gpioname[20];
				
				printk("read/write one gpio test\n");
				if(arg == 0)
				{
					printk("error para!!!\n");
					break;
				}
				if (copy_from_user(data, (void*)arg, 3)){
					return -EFAULT;
				}
				gpio = data[0];
				bRead = data[1] ? 1 : 0;
				value = data[2] ? 1 : 0;
				printk("io %d, bRead %d, value %d\n", gpio, bRead, value);
				
				sprintf(gpioname, "gpio%d", gpio);
				if(gpio_request(gpio, 	gpioname) < 0)
				{
					gpio_free(gpio);
					ret = gpio_request(gpio, 	gpioname);
					printk("request gpio %d\n", gpio);
				}		
				if(bRead)
				{
					gpio_direction_input(gpio);
					value = gpio_get_value(gpio);
					data[2] = value;
					if (copy_to_user((void*)arg, data, 3)){
						return -EFAULT;
					}					
				}
				else
				{
					gpio_direction_output(gpio, value);
				}
				printk("read/write one gpio test END.\n");
			}
			break;
			case CARDS_IOC_OPERA_BUS_GPIO_WR:  //format : byte 0: gpio number, byte 1: rw flag, byte 2~3: reserve, byte 4~7: write value;
			{
				int gpio;
				int bRead;
				int value;
				int ret;
	
				printk("read/write bank gpio test\n");
				if(arg == 0)
				{
					printk("error para!!!\n");
					break;
				}
				if (copy_from_user(data, (void*)arg, 8)){
					return -EFAULT;
				}
				gpio = data[0]; //range 0~2
				bRead = data[1] ? 1 : 0;
				memcpy(&value, &data[4], 4);
				printk("io %d, bRead %d, value 0x%x\n", gpio, bRead, value);
				
				//no gpio_request() or direction setting , these should do aforehand				
				if(bRead)
				{
					gpio_bank_set_direction(gpio, 0x0, 0xffffffff); //set direction
					value = gpio_bank_get_value(gpio);
					
					memcpy(&data[4], &value, 4);
					if (copy_to_user((void*)arg, data, 8)){
						return -EFAULT;
					}					
				}
				else
				{
					gpio_bank_set_direction(gpio, 0xffffffff, 0xffffffff);  //set direction
					gpio_bank_set_value(gpio, value, 0xffffffff);
				}
				printk("read/write bank gpio test END..\n");

			}
			break;
		}
		return 0;
}


/******
               ___________________________________________
address[21:0] <_____________X______________X______________
            __|___________________________________________
data[15:0]  __|_______X_______________X_______________X___
            __|__          _______         ______
csn           |  \________/       \_______/      \________
              |   |              
            __|__ |        _______         ______
rdn           |  \|_______/       \_______/      \________
              |   |               |       |   
              |Tca|               |Trd    |
              
              
_________________________________________________________________________
            |min             |  typ          |  max           |unit      |
____________|________________|_______________|________________|__________|
Tca         |  0             |               |                |ns        |
____________|________________|_______________|________________|__________|
Trd         |  0.1           | 500           |                |us        |
____________|________________|_______________|________________|__________|

describe：
output address first, then pull CS & READ to low,
wait until 120ns
read data
then pull CS & READ high,
then read next address

******/
static int getAddrValue(int tempaddr){
	unsigned short data[16];
	unsigned int temp;
	unsigned short value;
	unsigned int wr_addr = 0;
#ifdef BANK_GPIO_OPERA 	
	unsigned int wr_addr_mask = 0xffffffff;//(0x1ffff << 7) | (1u << 25) | (7u << 28);
#endif	
	gpio_direction_output(CARD_OE,1);

#ifndef BANK_GPIO_OPERA 
	gpio_direction_output(CARD_A0,(tempaddr>>0)&0x1);
	gpio_direction_output(CARD_A1,(tempaddr>>1)&0x1);
	gpio_direction_output(CARD_A2,(tempaddr>>2)&0x1);
	gpio_direction_output(CARD_A3,(tempaddr>>3)&0x1);
	gpio_direction_output(CARD_A4,(tempaddr>>4)&0x1);
	gpio_direction_output(CARD_A5,(tempaddr>>5)&0x1);
	gpio_direction_output(CARD_A6,(tempaddr>>6)&0x1);
	gpio_direction_output(CARD_A7,(tempaddr>>7)&0x1);
	gpio_direction_output(CARD_A8,(tempaddr>>8)&0x1);
	gpio_direction_output(CARD_A9,(tempaddr>>9)&0x1);
	gpio_direction_output(CARD_A10,(tempaddr>>10)&0x1);
	gpio_direction_output(CARD_A11,(tempaddr>>11)&0x1);
	gpio_direction_output(CARD_A12,(tempaddr>>12)&0x1);
	gpio_direction_output(CARD_A13,(tempaddr>>13)&0x1);
	gpio_direction_output(CARD_A14,(tempaddr>>14)&0x1);
	gpio_direction_output(CARD_A15,(tempaddr>>15)&0x1);
	gpio_direction_output(CARD_A16,(tempaddr>>16)&0x1);
#else
	wr_addr |= (tempaddr & 0x1ffff) << 7;
#endif

#ifndef BANK_GPIO_OPERA 
	gpio_direction_output(CARD_A17,(tempaddr>>17)&0x1);
#else
	wr_addr |= ((tempaddr >> 17) & 0x1) << 25;	
#endif

#ifndef BANK_GPIO_OPERA 
	gpio_direction_output(CARD_A18,(tempaddr>>18)&0x1);
	gpio_direction_output(CARD_A19,(tempaddr>>19)&0x1);
	gpio_direction_output(CARD_A20,(tempaddr>>20)&0x1);
#else
	wr_addr |= ((tempaddr>>18) & 0x7) << 28;	
	gpio_bank_set_value(2, wr_addr, wr_addr_mask);
#endif

	gpio_direction_output(CARD_A21,(tempaddr>>21)&0x1);
	
	gpio_direction_output(CARD_OE,1);
	gpio_direction_output(CARD_OE,1);  //delay
	gpio_direction_output(CARD_OE,1);  //delay
#if 1
	gpio_direction_output(CARD_OE,0);  //delay
	gpio_direction_output(CARD_OE,0);  //delay
	gpio_direction_output(CARD_OE,0);  //delay
#else
	gpio_direction_output(CARD_OE,0);
	{int delay; for(delay=0; delay < 0x400; delay++);}
#endif	

#ifndef BANK_GPIO_OPERA 
	data[0]=gpio_get_value(CARD_D0);
	data[1]=gpio_get_value(CARD_D1);
	data[2]=gpio_get_value(CARD_D2);
	data[3]=gpio_get_value(CARD_D3);
	data[4]=gpio_get_value(CARD_D4);
	data[5]=gpio_get_value(CARD_D5);
	data[6]=gpio_get_value(CARD_D6);
	data[7]=gpio_get_value(CARD_D7);
	data[8]=gpio_get_value(CARD_D8);
	data[9]=gpio_get_value(CARD_D9);
	data[10]=gpio_get_value(CARD_D10);
	data[11]=gpio_get_value(CARD_D11);
	data[12]=gpio_get_value(CARD_D12);
	data[13]=gpio_get_value(CARD_D13);
	data[14]=gpio_get_value(CARD_D14);
	data[15]=gpio_get_value(CARD_D15);
	value=data[0]|(data[1]<<1)|(data[2]<<2)|(data[3]<<3)|(data[4]<<4)|(data[5]<<5)|(data[6]<<6)|(data[7]<<7)|(data[8]<<8)|(data[9]<<9)|(data[10]<<10)|(data[11]<<11)
		  |(data[12]<<12)|(data[13]<<13)|(data[14]<<14)|(data[15]<<15);
#else
	temp = gpio_bank_get_value(0);
	//value = ((temp & (0x3 << CARD_D0)) >>  2) | ((temp & (0x2f << CARD_D2)) >> 6) |  ((temp & (0x1f << CARD_D8)) >> 8) | ((temp & (0x7 << CARD_D13)) >> 13); 
	value = ((temp & (0x3 << CARD_D0)) >>  2) | ((temp & (0x3 << CARD_D2)) >> 6) | ((temp & (0xf << CARD_D4)) >> 7)  |  ((temp & (0x1f << CARD_D8)) >> 8) | ((temp & (0x7 << CARD_D13)) >> 13); 
	
	if(1){
		static int test_flag = 1;
		if(test_flag == 1)
		{
			test_flag = 0;
			printk("read= 0x%x, value=0x%x\n", temp, value);
		}
	}
#endif		  
	gpio_direction_output(CARD_OE,0);//delay
	gpio_direction_output(CARD_OE,0);//delay
	return value;
}

static void initGpioDirection()
{
//#ifdef BANK_GPIO_OPERA 		
	unsigned int wr_addr_mask = 0xffffffff;//(0x1ffff << 7) | (1u << 25) | (7u << 28);
	unsigned int rd_data_mask = 0xffffffff; //(1 << CARD_D0) | (1 << CARD_D1)  | (1 << CARD_D2)  | (1 << CARD_D3)  | (1 << CARD_D4)  | (1 << CARD_D5)  | (1 << CARD_D6)  | (1 << CARD_D7)  | (1 << CARD_D8)  | (1 << CARD_D9)  | (1 << CARD_D10)  | (1 << CARD_D11)  | (1 << CARD_D12)  | (1 << CARD_D13)  | (1 << CARD_D14)  | (1 << CARD_D15)  ;
//#endif	

//#ifndef BANK_GPIO_OPERA 
	gpio_direction_output(CARD_A0,0);
	gpio_direction_output(CARD_A1,0);
	gpio_direction_output(CARD_A2,0);
	gpio_direction_output(CARD_A3,0);
	gpio_direction_output(CARD_A4,0);
	gpio_direction_output(CARD_A5,0);
	gpio_direction_output(CARD_A6,0);
	gpio_direction_output(CARD_A7,0);
	gpio_direction_output(CARD_A8,0);
	gpio_direction_output(CARD_A9,0);
	gpio_direction_output(CARD_A10,0);
	gpio_direction_output(CARD_A11,0);
	gpio_direction_output(CARD_A12,0);
	gpio_direction_output(CARD_A13,0);
	gpio_direction_output(CARD_A14,0);
	gpio_direction_output(CARD_A15,0);
	gpio_direction_output(CARD_A16,0);
	gpio_direction_output(CARD_A17,0);
	gpio_direction_output(CARD_A18,0);
	gpio_direction_output(CARD_A19,0);
	gpio_direction_output(CARD_A20,0);
//#else
	gpio_bank_set_direction(2, wr_addr_mask, wr_addr_mask); // set output
//#endif	
	gpio_direction_output(CARD_A21,0);

//#ifndef BANK_GPIO_OPERA 
	gpio_direction_input(CARD_D0);
	gpio_direction_input(CARD_D1);
	gpio_direction_input(CARD_D2);
	gpio_direction_input(CARD_D3);
	gpio_direction_input(CARD_D4);
	gpio_direction_input(CARD_D5);
	gpio_direction_input(CARD_D6);
	gpio_direction_input(CARD_D7);
	gpio_direction_input(CARD_D8);
	gpio_direction_input(CARD_D9);
	gpio_direction_input(CARD_D10);
	gpio_direction_input(CARD_D11);
	gpio_direction_input(CARD_D12);
	gpio_direction_input(CARD_D13);
	gpio_direction_input(CARD_D14);
	gpio_direction_input(CARD_D15);
//#else
	gpio_bank_set_direction(0, 0, rd_data_mask); // set input
//#endif	
	
}

static void initGpio(){
	
	gpio_request(CARD_A0,"CARD_A0");
	gpio_request(CARD_A1,"CARD_A1");
	gpio_request(CARD_A2,"CARD_A2");
	gpio_request(CARD_A3,"CARD_A3");
	gpio_request(CARD_A4,"CARD_A4");
	gpio_request(CARD_A5,"CARD_A5");
	gpio_request(CARD_A6,"CARD_A6");
	gpio_request(CARD_A7,"CARD_A7");
	gpio_request(CARD_A8,"CARD_A8");
	gpio_request(CARD_A9,"CARD_A9");
	gpio_request(CARD_A10,"CARD_A10");
	gpio_request(CARD_A11,"CARD_A11");
	gpio_request(CARD_A12,"CARD_A12");
	gpio_request(CARD_A13,"CARD_A13");
	gpio_request(CARD_A14,"CARD_A14");
	gpio_request(CARD_A15,"CARD_A15");
	gpio_request(CARD_A16,"CARD_A16");
	gpio_request(CARD_A17,"CARD_A17");
	gpio_request(CARD_A18,"CARD_A18");
	gpio_request(CARD_A19,"CARD_A19");
	gpio_request(CARD_A20,"CARD_A20");
	gpio_request(CARD_A21,"CARD_A21");

	
	gpio_request(CARD_D0,"CARD_D0");
	gpio_request(CARD_D1,"CARD_D1");
	gpio_request(CARD_D2,"CARD_D2");
	gpio_request(CARD_D3,"CARD_D3");
	gpio_request(CARD_D4,"CARD_D4");
	gpio_request(CARD_D5,"CARD_D5");
	gpio_request(CARD_D6,"CARD_D6");
	gpio_request(CARD_D7,"CARD_D7");
	gpio_request(CARD_D8,"CARD_D8");
	gpio_request(CARD_D9,"CARD_D9");
	gpio_request(CARD_D10,"CARD_D10");
	gpio_request(CARD_D11,"CARD_D11");
	gpio_request(CARD_D12,"CARD_D12");
	gpio_request(CARD_D13,"CARD_D13");
	gpio_request(CARD_D14,"CARD_D14");
	gpio_request(CARD_D15,"CARD_D15");
	gpio_request(CARD_OE,"CARD_OE");
	
	initGpioDirection();
	
}


static struct file_operations cards_ops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	=	cards_ioctl,
	
};

static struct miscdevice cards_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cards",
	.fops = &cards_ops,
	.mode   = 0666,
};

static int __init cards_dev_init(void) {
	int ret;
#if 1  //disable CARDS	
	initGpio();
	if((ret = misc_register(&cards_misc_dev)))
	{
		printk("cards: misc_register register failed\n");
	}

	printk("cards initialized...\n");
#else

	ret = -1;
	printk("cards initialized fail..\n");
#endif
	return ret;
}

static void __exit cards_dev_exit(void) {
	misc_deregister(&cards_misc_dev);
}

module_init(cards_dev_init);
module_exit(cards_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DECO Inc.");
MODULE_DESCRIPTION("DECO IR control Driver");
