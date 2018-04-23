//===========================
// Code for Testing the CryptoMemory Library
//===========================
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
#include <linux/delay.h>

// CryptoMemory Library Include Files
#include "lowlevel.h" 

//#define BURN_MODE
#define DEBUG_MODE_


#ifndef BURN_MODE
#define printf
#endif

#ifdef DEBUG_MODE_
#define printf printk
#endif

extern void deinit_gpio(void);

// Global Variables

static uchar rwdata[16],cpdata[16],ucrandom[8],uckey[8];
uchar L1_state,state,L1_return,fuse,all_done;
uchar MTZ_test(void);
uchar cw0881_demo(void);	
uchar ucReturn;
uchar ucCi1[8]={0xFF,0xC1,0x06,0xB9,0x7B,0x28,0xDD,0xD5}; 
uchar ucSk1[8]={0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}; 

uchar ucdata[16];

#if 1 //new															//用户修改区 
uchar ucG0[8]={0x81,0x1A,0x13,0x37,0x9A,0xAD,0x1A,0x79};			//Secret Seed G0 
uchar ucG1[8]={0x52,0x3A,0x47,0xAE,0x89,0x62,0x41,0x61};			//Secret Seed G1 // modify
uchar ucG2[8]={0x34,0x27,0x89,0x20,0xD8,0x28,0x1B,0x24};			//Secret Seed G2
uchar ucG3[8]={0x55,0x89,0x02,0x9A,0xBA,0xDE,0xF0,0x18};			//Secret Seed G3
#else //old
uchar ucG0[8]={0x81,0x1A,0x15,0x37,0x9A,0xAC,0xDD,0xd9};                       //Secret Seed G0 
uchar ucG1[8]={0x56,0x3A,0x48,0xAE,0x89,0x92,0x43,0x6D};                       //Secret Seed G1 // modify
uchar ucG2[8]={0x35,0x47,0x09,0x90,0x88,0x98,0x1a,0x25};                       //Secret Seed G2
uchar ucG3[8]={0x88,0x89,0x09,0x9A,0xBC,0xDE,0xF0,0x18};                       //Secret Seed G3
#endif

#ifdef BURN_MODE
const uchar password_table[64]={									//用户修改区
						0xFF,0x00,0x11,0x22,0xFF,0x22,0x11,0x00,	//Passwords
						0xFF,0x6A,0x30,0xC5,0xFF,0x28,0xC0,0x2A,
						0xFF,0x22,0x33,0x44,0xFF,0x44,0x33,0x22,
						0xFF,0x34,0x50,0x95,0xFF,0x58,0x36,0x04, //modify						
						0xFF,0x44,0x55,0x66,0xFF,0x66,0x55,0x44,
						0xFF,0x55,0x66,0x77,0xFF,0x77,0x66,0x55,
						0xFF,0x66,0x77,0x88,0xFF,0x88,0x77,0x66,
						0xFF,0x8a,0xbc,0x10,0xFF,0xC8,0xA9,0xC5    
						};

uchar password[64]={
						0xFF,0x00,0x11,0x22,0xFF,0x22,0x11,0x00,
						0xFF,0x6A,0x30,0xC5,0xFF,0x28,0xC0,0x2A,
						0xFF,0x22,0x33,0x44,0xFF,0x44,0x33,0x22,
						0xFF,0x33,0x44,0x55,0xFF,0x55,0x44,0x33,
						0xFF,0x44,0x55,0x66,0xFF,0x66,0x55,0x44,
						0xFF,0x55,0x66,0x77,0xFF,0x77,0x66,0x55,
						0xFF,0x66,0x77,0x88,0xFF,0x88,0x77,0x66,
						0xFF,0xdd,0x42,0x97,0xFF,0xC0,0xB9,0xC8
						};
#endif						
						




uchar MTZ_test(void)
{
	rwdata[0]=0;rwdata[1]=0;	

	L1_return=cm_ReadConfigZone(0x0A,rwdata,2);
	printf("L1_return** = %d\n", L1_return);
	
	rwdata[0]=0xAA;rwdata[1]=0x55;
	cm_WriteConfigZone(0x0A,rwdata,2);
	rwdata[0]=0;rwdata[1]=0;
	L1_return=cm_ReadConfigZone(0x0A,rwdata,2);
	if(rwdata[0]!=0xAA || rwdata[1]!=0x55)
	{
		printk("MTZ_test fail!\n");
		return FAIL;
	}
	else
	{	
		printk("MTZ_test PASS\n");
		return PASS;
	}
}

extern  void init_gpio();
extern void gpio_test(void);

uchar cw0881_work(void)	
{	
	uchar i;
	
	// add for ac8317
	init_gpio();
	//gpio_test();

#ifndef DEBUG_MODE_
	L1_return=PASS;	
#else	
	L1_return=MTZ_test();
#endif
	
	state=3;L1_state=1;
#ifdef BURN_MODE	
	for(i=0;i<64;i++)  password[i]=password_table[i];
#endif
	
	if(L1_return != PASS)
	{
		printf("cw0881_work fail..\n");
		return FALSE;
	}
	else
		printf("cw0881_work success..\n");
		

#ifdef DEBUG_MODE_
	//for test
	deinit_gpio();
	return TRUE;
#endif	
	
#ifndef BURN_MODE
	L1_state = 3;
#endif	

	while(state==3)
		{
		switch(L1_state)
			{
#ifdef BURN_MODE  //只针对出厂芯片烧写。
			case 1:											// unlock configure zone 
#if 1			
				rwdata[0]=0x8a;//0xdd;
				rwdata[1]=0xbc;//0x42;
				rwdata[2]=0x10;//0x97;
#else //default output
				rwdata[0]=0xdd;
				rwdata[1]=0x42;
				rwdata[2]=0x97;
#endif				
				L1_return=cm_VerifyPassword(rwdata,7,0);	// 
				if(L1_return != SUCCESS)
					printf("case 1 fail %u\n", L1_return);
				else
					printf("case 1 success \n");
										
				L1_state=2;
				break;
			case 2: //烧写功能
				rwdata[0]=0x57;rwdata[1]=0x6B;	// AR0,PR0, rw-password needed, normal auth mode, encry required,AK=G1,PW=3 
				rwdata[2]=0xff;rwdata[3]=0xBC;	// AR1,PR1, rw-password free,  auth free, encry required disable,AK=G2,PW=4 
				rwdata[4]=0x57;rwdata[5]=0xCD;	// AR2,PR2, rw-password needed, normal auth mode, encry required,AK=G3,PW=5 
				rwdata[6]=0x57;rwdata[7]=0x1E;	// AR3,PR3, rw-password needed, normal auth mode, encry required,AK=G0,PW=6 
				cm_WriteConfigZone(0x20,rwdata,8);  //写ar pr 加密 配置数据
				cm_ReadConfigZone(0x20,rwdata,8);
				if(rwdata[0]!=0x57 || rwdata[7]!=0x1E) return FALSE;
				
				cm_WriteConfigZone(0x90,ucG0,8); // write secret bank   // write seed 
				cm_WriteConfigZone(0x98,ucG1,8);
				cm_WriteConfigZone(0xA0,ucG2,8);
				cm_WriteConfigZone(0xA8,ucG3,8);
		
				cm_WriteConfigZone(0xB0,password,16);		// write password bank 一次写2组
				cm_WriteConfigZone(0xC0,&password[16],16);
				cm_WriteConfigZone(0xD0,&password[32],16);
				cm_WriteConfigZone(0xE0,&password[48],16);

				L1_state=3;
				printf("case 2 finish. \n");
				
				msleep(100);
				deinit_gpio();
				return PASS;
				
				break;
#endif
			case 3:			// PR0, user zone 0 test
				cm_SetUserZone(0);	 //set user zone 0
				for (i = 0; i < 8; ++i) uckey[i] = ucG1[i];
				L1_return=cm_ActiveSecurity(1, uckey, 1);  //verify auth
				printf("%d, L1_return...! = %u\n", __LINE__, L1_return);
//if(L1_return != PASS) return FALSE;	
				//for (i = 0; i < 8; ++i) uckey[i] = ucG2[i];
				//L1_return=cm_ActiveSecurity(2, uckey, 1);  //verify auth
				//printf("%d, L1_return... = %u\n", __LINE__, L1_return);
							
				rwdata[0]=0x8a;
				rwdata[1]=0xbc;
				rwdata[2]=0x10;
				L1_return=cm_VerifyPassword(rwdata,7,0);	//
				printf("%d, L1_return.* = %u\n", __LINE__, L1_return);   //// error.....
//if(L1_return != PASS) return FALSE;			
				rwdata[0]=0x34;rwdata[1]=0x50;rwdata[2]=0x95;
				L1_return=cm_VerifyPassword(rwdata,3,0);	// verify write password
				printf("%d, L1_return. = %u\n", __LINE__, L1_return);
//if(L1_return != PASS) return FALSE;			
			 	for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
			 	//cm_SetUserZone(0);
				cm_ReadUserZone(0x10, rwdata, 16);

				if(rwdata[0] != 0x53 || rwdata[4] != 0x53 || rwdata[8] != 0x53 || rwdata[15] != 0x53) //避免多次擦写。
				{
					printf("881 write user zone.*.\n");	
					//msleep(10);
					for (i = 0; i < 16; ++i) rwdata[i] = 0x4a;				
					L1_return = cm_WriteUserZone(0, rwdata, 16);	 //write test 1
					printf("cm_WriteUserZone ret = %d\n", L1_return);	
					for (i = 0; i < 16; ++i) rwdata[i] = 0x53;
					L1_return = cm_WriteUserZone(0x10, rwdata, 16);	 //write test 2
					msleep(1);
					printf("cm_WriteUserZone ret = %d\n", L1_return);									
				}

				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;	
				L1_return = cm_ReadUserZone(0, rwdata, 16);	//read 1						
				for (i = 0; i < 16; ++i) 
				{
					if (rwdata[i]!=0x4a)
					{
						printf("error read1 userzone: %d, %u(%d)\n", i, rwdata[i], L1_return);	 
						return FALSE;
					}
				}
				
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;	
				cm_ReadUserZone(0x10, rwdata, 16);	 //read 2		
				for (i = 0; i < 16; ++i)
				{ 
					if (rwdata[i]!=0x53) 
					{
						printf("error read2 userzone: %d, %x, %x\n", i, rwdata[i], rwdata[15]);	 
						return FALSE;
					}
				}
				
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;	//read 2 again
				cm_ReadUserZone(0x10, rwdata, 16);			
				for (i = 0; i < 16; ++i)
				{ 
					if (rwdata[i]!=0x53)
					{ 
						printf("error read2. userzone: %d, %u\n", i, rwdata[i]);	 
						return FALSE;
					}
				}	
					
				L1_state=4;

				printf("case 3 finish. \n");
				
				//simple do.
				deinit_gpio();
				return 1;
				break;
			case 4:		
				cm_RstDevice();	// device rst cryto
				cm_ResetCrypto();
				for (i = 0; i < 16; ++i) rwdata[i] = 0x22;
				cm_SetUserZone(1);   //set user zone 1
				cm_WriteUserZone(0, rwdata, 16);		
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				for (i = 0; i < 16; ++i) rwdata[i] = 0;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x22) return FALSE;	
				L1_state=5;
				break;
			case 5:			// PR2, user zone 2 test
				cm_SetUserZone(2);  //set user zone 2
				for (i = 0; i < 8; ++i) rwdata[i] = ucG3[i];
				cm_ActiveSecurity(3, rwdata, 1);  //verify auth
				rwdata[0]=0x55;rwdata[1]=0x66;rwdata[2]=0x77;
				cm_VerifyPassword(rwdata,5,0);	// verify write password			
				for (i = 0; i < 16; ++i) rwdata[i] = 0x33;				
				cm_WriteUserZone(0, rwdata, 16); 						
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x33) return FALSE;
				cm_RstDevice();	// device rst cryto
				cm_ResetCrypto();			
				L1_state=6;
				break;
			case 6:			// PR3, user zone 3 test
				cm_SetUserZone(2);  //set user zone 2
				for (i = 0; i < 8; ++i) rwdata[i] = ucG3[i];
				cm_ActiveSecurity(3, rwdata, 1);  //verify auth
				rwdata[0]=0x77;rwdata[1]=0x66;rwdata[2]=0x55;
				cm_VerifyPassword(rwdata,5,1);	// verify read password	
				rwdata[0]=0x77;rwdata[1]=0x66;rwdata[2]=0x55;
				cm_VerifyPassword(rwdata,5,1);	// verify read password again													
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x33) return FALSE;
				rwdata[0]=0x55;rwdata[1]=0x66;rwdata[2]=0x77;
				cm_VerifyPassword(rwdata,5,0);	// verify write password	
				for (i = 0; i < 16; ++i) rwdata[i] = 0x66;				
				cm_WriteUserZone(0, rwdata, 16); 						
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x66) return FALSE;

				cm_SetUserZone(2);  //set user zone 2
				for (i = 0; i < 8; ++i) rwdata[i] = ucG3[i];
				cm_ActiveSecurity(3, rwdata, 1);  //verify auth again 
				rwdata[0]=0x77;rwdata[1]=0x66;rwdata[2]=0x55;
				cm_VerifyPassword(rwdata,5,1);	// verify read password	
				rwdata[0]=0x77;rwdata[1]=0x66;rwdata[2]=0x55;
				cm_VerifyPassword(rwdata,5,1);	// verify read password again													
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x66) return FALSE;
				rwdata[0]=0x55;rwdata[1]=0x66;rwdata[2]=0x77;
				cm_VerifyPassword(rwdata,5,0);	// verify write password	
				for (i = 0; i < 16; ++i) rwdata[i] = 0x77;				
				cm_WriteUserZone(0, rwdata, 16); 						
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x77) return FALSE;
				L1_state=7;	
				break;							
			case 7:			// PR3, user zone 3 test
				cm_SetUserZone(3);  //set user zone 3
				for (i = 0; i < 8; ++i) rwdata[i] = ucG0[i];
				cm_ActiveSecurity(0, rwdata, 1);  //verify auth
				rwdata[0]=0x66;rwdata[1]=0x77;rwdata[2]=0x88;
				cm_VerifyPassword(rwdata,6,0);	// verify write password
				
				for (i = 0; i < 16; ++i) rwdata[i] = 0x44;

				cm_WriteUserZone(0, rwdata, 16);		
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;				
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x44) return FALSE;
				
				cm_SetUserZone(3); //set user zone 3 again
				for (i = 0; i < 16; ++i) rwdata[i] = 0x55;
				cm_WriteUserZone(0, rwdata, 16);	
				
				cm_SetUserZone(3);					
				for (i = 0; i < 16; ++i) rwdata[i] = 0x00;			
				cm_ReadUserZone(0, rwdata, 16);			
				for (i = 0; i < 16; ++i) if (rwdata[i]!=0x55) return FALSE;
				cm_RstDevice();	// device rst cryto
				cm_ResetCrypto();
				break;
			}
		}
		
	deinit_gpio();
	return L1_return;
}
