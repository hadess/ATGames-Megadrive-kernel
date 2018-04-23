//==================================   


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

//#include <pinmux.h>
//#include <ac83xx_pinmux_table.h>
//#include <ac83xx_gpio_pinmux.h>

//#include <ac83xx_gpio.h>
//#include <gpio.h>

#include "lowlevel.h" 
#include "customer.h" 

//==================================
//extern void delay_us(u32 nus);
//extern void delay_ms(u32 nus);
#define delay_us(a)   udelay(3*a)
#define delay_ms(a)   msleep(a)

// Global Data
//static uchar  ucCM_G_Sk[8]/*,rand[8]*/;
//static uchar ucCM_Q_Ch[16], ucCM_Ci2[8];
static uchar ucCM_InsBuff[4];
static uchar ucCM_Encrypt;
static uchar ucCM_Authenticate;
static uchar ucGpaRegisters[Gpa_Regs];
static uchar ucRandRegisters[Rand_Regs];

void cm_AckNak(uchar ucAck);
void i2c_stop(void);
void i2c_start(void);
uchar read_byte(void);
uchar write_byte(uchar byte);
void cm_GPAencrypt(uchar ucEncrypt, uchar ucBuffer[16], uchar ucCount);
void cm_GPAdecrypt(uchar ucEncrypt, uchar ucBuffer[16], uchar ucCount);
void cm_GPAcmd3(uchar ucInsBuff[16]);
void cm_GPAcmd2(uchar ucInsBuff[16]);
void cm_GPAGenNF(uchar Count, uchar DataIn);
void cm_GPAGenN(uchar Count);
void cm_CalChecksum(uchar Ck_sum[2]);
void cm_AuthenEncryptCal(uchar Ci[8], uchar G_Sk[8], uchar Q[8], uchar Ch[8]);
uchar cm_GPAGen(uchar Datain);
uchar cm_VerifyPassword(uchar ucPassword[3], uchar ucSet, uchar ucRW);
uchar cm_ResetCrypto(void);
uchar cm_SendChecksum(void);
uchar cm_AuthenEncrypt(uchar ucKeySet, uchar Buffer[16],uchar ucEncrypt);
uchar cm_ActiveSecurity(uchar ucKeySet, uchar ucKey[16],uchar ucEncrypt);
uchar cm_RstDevice(void);
uchar cm_ReadUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
uchar cm_WriteUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
uchar cm_SetUserZone(uchar ucZoneNumber);
uchar cm_WriteConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
uchar cm_ReadConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
uchar cm_RandGen(uchar Datain);
//================================== 
static void write_gpio(int pin, int level);



//for gpio operation 
#define SCL_GPIO_PIN (0)
#define SDA_GPIO_PIN (1)
#define PWR_GPIO_PIN (51)


void init_gpio(void)
{
	//printf("init_gpio\n");
	//GPIO_MultiFun_Set(SCL_GPIO_PIN,PINMUX_LEVEL_GPIO_END_FLAG);
	//GPIO_MultiFun_Set(SDA_GPIO_PIN,PINMUX_LEVEL_GPIO_END_FLAG);
	write_gpio(PWR_GPIO_PIN, 1);
	delay_ms(10);
	
	write_gpio(SCL_GPIO_PIN, 1);
	write_gpio(SDA_GPIO_PIN, 1);
	delay_ms(1);
	
}

void deinit_gpio(void)
{
	write_gpio(PWR_GPIO_PIN, 0);
	write_gpio(SCL_GPIO_PIN, 0);
	write_gpio(SDA_GPIO_PIN, 0);	
}

void gpio_test(void)
{
	int test = 10;
	
	while(test--)
	{
#if 1		
		write_gpio(SCL_GPIO_PIN, 1);
		write_gpio(SDA_GPIO_PIN, 1);	
		printk("on\n");
		//delay_ms(500);
		mdelay(500);	
		write_gpio(SCL_GPIO_PIN, 0);
		write_gpio(SDA_GPIO_PIN, 0);	
		printk("off\n");
		//delay_ms(500);	
		mdelay(500);
#else
		int ret;
		write_gpio(SDA_GPIO_PIN, 1);
		ret = read_gpio(SDA_GPIO_PIN);
		printf("read ada..%d\n", ret);
		delay_ms(500);	
		write_gpio(SDA_GPIO_PIN, 1);
		ret = read_gpio(SDA_GPIO_PIN);
		printf("read ada..%d\n", ret);
		delay_ms(500);	
#endif		
		
	}
	
}

extern int gpio_bank_set_direction(unsigned gpio,int dir, unsigned int mask);

//dir 1: output, 0:input
int gpio_set_setdir(int pin, int dir)
{
	int bank = pin / 32;
	int index = pin % 32;
	
	if(bank <= 2)
	{
		gpio_bank_set_direction(bank, dir ? 0xffffffff : 0, 1u << index); 
		return 0;
	}
	else
		return -1;
}

int read_gpio(int pin)
{
	//gpio_direction_input(pin);
	
	gpio_set_setdir(pin, 0);
	return  gpio_get_value(pin);
}


void write_gpio(int pin, int level)
{
	if(level)
	{	
		//gpio_direction_output(pin, 1);
		
		gpio_set_value(pin, 1);
		gpio_set_setdir(pin, 1);
	}
	else		
	{
		//gpio_direction_output(pin, 0);
		
		gpio_set_value(pin, 0);
		gpio_set_setdir(pin, 1);
	}
}





//Read configZone
uchar cm_ReadConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount)
{
	int i;
    uchar ucEncrypt;
    ucCM_InsBuff[0] = 0xb6;
    ucCM_InsBuff[1] = 0x00;
    ucCM_InsBuff[2] = ucAddr;
    ucCM_InsBuff[3] = ucCount;	
    // Three bytes of the command must be included in the polynominals
    cm_GPAcmd2(ucCM_InsBuff);
	// Do the read
	i2c_start();
	if( write_byte(0xb6)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(ucAddr)!=ACK )	return FAIL_ACK;
	if( write_byte(ucCount)!=ACK )	return FAIL_ACK;

    for(i = 0; i < (ucCount-1); i++) {
        ucBuffer[i] = read_byte();
        cm_AckNak(1);
    }
	ucBuffer[i] = read_byte();
	cm_AckNak(0);
	i2c_stop();
    // Only password zone is ever encrypted
    ucEncrypt = ((ucAddr>= CM_PSW) && ucCM_Encrypt);
    // Include the data in the polynominals and decrypt if required
    cm_GPAdecrypt(ucEncrypt, ucBuffer, ucCount); 
    // Done
    return SUCCESS;	
}
//Write ConfigZone
uchar cm_WriteConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount)
{
	int i;
    uchar ucReturn, ucEncrypt;
    ucCM_InsBuff[0] = 0xb4;
    ucCM_InsBuff[1] = 0x00;
    ucCM_InsBuff[2] = ucAddr;
    ucCM_InsBuff[3] = ucCount;
    // Three bytes of the command must be included in the polynominals
    cm_GPAcmd2(ucCM_InsBuff);
    // Only password zone is ever encrypted
    ucEncrypt = ((ucAddr>= CM_PSW) && ucCM_Encrypt);
    // Include the data in the polynominals and encrypt if required
    cm_GPAencrypt(ucEncrypt, ucBuffer, ucCount); 
    // Do the write	
	i2c_start();
	if( write_byte(0xb4)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(ucAddr)!=ACK )	return FAIL_ACK;
	if( write_byte(ucCount)!=ACK )	return FAIL_ACK;
	
    for(i = 0; i < ucCount; i++) {
		if( write_byte(ucBuffer[i])!=ACK )	return FAIL_ACK;
    }
	i2c_stop();
	delay_ms(10);
	ucReturn = TRUE;
	return ucReturn;
}
// Verify Password
uchar cm_VerifyPassword(uchar ucPassword[3], uchar ucSet, uchar ucRW)
{
	uchar i, j;
    uchar ucReturn;
	uchar ucAddr;
	uchar ucCmdPassword[4] = {0xba, 0x00, 0x00, 0x03};
	uchar ucPSW[3];	
	// Build command and PAC address
    ucAddr = CM_PSW + (ucSet<<3);  //get write7 address
	ucCmdPassword[1] = ucSet;
	if (ucRW != CM_PWWRITE) {  //看是不是读
	  	ucCmdPassword[1] |= 0x10;
	  	ucAddr += 4;
	}	  
	// Deal with encryption if in authenticate mode
	for (j = 0; j<3; j++) {
	    // Encrypt the password
	    if(ucCM_Authenticate) {
    	    for(i = 0; i<5; i++) cm_GPAGen(ucPassword[j]);
    		ucPSW[j] = Gpa_byte;
	  	}
	    // Else just copy it
		else ucPSW[j] = ucPassword[j];
    }	  
	// Send the command
	i2c_start();
	if( write_byte(ucCmdPassword[0])!=ACK )	return FAIL_ACK;
	if( write_byte(ucCmdPassword[1])!=ACK )	return FAIL_ACK;
	if( write_byte(ucCmdPassword[2])!=ACK )	return FAIL_ACK;
	if( write_byte(ucCmdPassword[3])!=ACK )	return FAIL_ACK;

    for(i = 0; i < 3; i++) {
			if( write_byte(ucPSW[i])!=ACK )	return FAIL_ACK;
    }
	i2c_stop();
	delay_ms(10);
	ucReturn = cm_ReadConfigZone(ucAddr, ucPSW, 1);
	if (ucPSW[0]!= 0xFF) ucReturn = FAIL;
	if (ucCM_Authenticate && (ucReturn != SUCCESS)) cm_ResetCrypto();
    // Done
    return ucReturn;
}
//Set UserZone
uchar cm_SetUserZone(uchar ucZoneNumber)
{
	uchar ucReturn;
    // Only zone number is included in the polynomial
	cm_GPAGen(ucZoneNumber);	
    // Do the write	
	i2c_start();
	if( write_byte(0xb4)!=ACK )	return FAIL_ACK;
	if( write_byte(0x03)!=ACK )	return FAIL_ACK;
	if( write_byte(ucZoneNumber)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;

	i2c_stop();
	delay_ms(10);
	ucReturn = TRUE;
	return ucReturn;	
}
//Write UserZone
uchar cm_WriteUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount)
{
	int i;
	uchar ucReturn;	
	ucCM_InsBuff[0] = 0xb0;
    ucCM_InsBuff[1] = 0x00;
    ucCM_InsBuff[2] = ucAddr;
    ucCM_InsBuff[3] = ucCount;
    // Two bytes of the command must be included in the polynominals
    cm_GPAcmd2(ucCM_InsBuff);
    // Include the data in the polynominals and encrypt it required
    cm_GPAencrypt(ucCM_Encrypt, ucBuffer, ucCount); 
    // Do the write	
	i2c_start();
	if( write_byte(0xb0)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(ucAddr)!=ACK )	return FAIL_ACK;
	if( write_byte(ucCount)!=ACK )	return FAIL_ACK;

    for(i = 0; i < ucCount; i++) {
		if( write_byte(ucBuffer[i])!=ACK )	return FAIL_ACK;
    }
	i2c_stop();
	delay_ms(10);
	ucReturn = cm_SendChecksum();
	return ucReturn;
}
//Read UserZone
uchar cm_ReadUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount)
{
	int i;
	uchar ucReturn;	
	ucCM_InsBuff[0] = 0xb2;
    ucCM_InsBuff[1] = 0x00;
    ucCM_InsBuff[2] = ucAddr;
    ucCM_InsBuff[3] = ucCount;
    // Two bytes of the command must be included in the polynominals
    cm_GPAcmd2(ucCM_InsBuff);
	// Do the read
	i2c_start();
	if( write_byte(0xb2)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(ucAddr)!=ACK )	return FAIL_ACK;
	if( write_byte(ucCount)!=ACK )	return FAIL_ACK;

    for(i = 0; i < (ucCount-1); i++) {
        ucBuffer[i] = read_byte();
        cm_AckNak(1);
    }
	ucBuffer[i] = read_byte();
	cm_AckNak(0);
	i2c_stop();	
    // Include the data in the polynominals and decrypt it required
    cm_GPAdecrypt(ucCM_Encrypt, ucBuffer, ucCount); 
	ucReturn = TRUE;
	return ucReturn;
}
//Reset Device
uchar cm_RstDevice(void)
{
	int i;
    cm_GPAGenN(20);
	// Do the read
	i2c_start();
	if( write_byte(0xb6)!=ACK )	return FAIL_ACK;
	if( write_byte(0x02)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(0x02)!=ACK )	return FAIL_ACK;

    for(i = 0; i < 1; i++) {
        read_byte();
        cm_AckNak(1);
    }
	read_byte();
	cm_AckNak(0);
	i2c_stop();	
	cm_ResetCrypto();
    return SUCCESS;
}
//ActiveSecurity
uchar cm_ActiveSecurity(uchar ucKeySet, uchar ucKey[16], uchar ucEncrypt)
{
    uchar i;
	uchar ucCi[8];
	uchar ucCi2[8];
	uchar ucQ0[8];
	uchar ucQ1[8];
	uchar ucQdata[16];
    uchar ucAddrCi;
    ucAddrCi = 0x50 + (ucKeySet<<4);
    cm_ReadConfigZone(ucAddrCi, ucCi, 8); //Read Cix from Device
	for (i = 0; i < 8; ++i) {
		ucQ0[i] = cm_RandGen(ucCi[i]);
	}								      //generate random data
	cm_AuthenEncryptCal(ucCi, ucKey, ucQ0, ucQ1); //Calculate Q1
	for (i = 0; i < 8; ++i) ucQdata[i] = ucQ0[i];
	for (i = 0; i < 8; ++i) ucQdata[i+8] = ucQ1[i];
	cm_AuthenEncrypt(ucKeySet, ucQdata,0);//Send Q0 and Q1
	//Verify result
    cm_ReadConfigZone(ucAddrCi, ucCi2, 8); //Read Cix from Device again	
    for(i=0; i<8; i++) if (ucCi[i]!=ucCi2[i]) return FAIL;
	ucCM_Authenticate = TRUE;
	if (ucEncrypt){
		for (i = 0; i < 8; ++i) {
			ucQ0[i] = cm_RandGen(ucCi2[i]);
		}											      //generate random data
		cm_AuthenEncryptCal(ucCi, ucKey, ucQ0, ucQ1); //Calculate Q1 again
		for (i = 0; i < 8; ++i) ucQdata[i] = ucQ0[i];
		for (i = 0; i < 8; ++i) ucQdata[i+8] = ucQ1[i];
		cm_AuthenEncrypt(ucKeySet, ucQdata,1);//Send Q0 and Q1
		//Verify result
		cm_ReadConfigZone(ucAddrCi, ucCi2, 8); //Read Cix from Device	
		for(i=0; i<8; i++) if (ucCi[i]!=ucCi2[i]) return FAIL;	
        ucCM_Encrypt = TRUE;
		ucCM_Authenticate = TRUE;		
	}
	return PASS;
} 
//Send Q0 and Q1
uchar cm_AuthenEncrypt(uchar ucKeySet, uchar Buffer[16],uchar ucEncrypt)
{
	int i;
	i2c_start();
	if( write_byte(0xb8)!=ACK )	return FAIL_ACK;
	if(ucEncrypt){
		if( write_byte(ucKeySet+0x10)!=ACK )	return FAIL_ACK;
	}
	else {
		if( write_byte(ucKeySet)!=ACK )	return FAIL_ACK;
	}
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(0x10)!=ACK )	return FAIL_ACK;
    for(i = 0; i < 16; i++) {
		if( write_byte(Buffer[i])!=ACK )	return FAIL_ACK;
    }
	i2c_stop();
	delay_ms(20);
	// Done
    return TRUE;
}
//============================
uchar cm_SendChecksum(void)
{
    int i;
	uchar ucReturn;
	uchar ucChkSum[2];
	cm_CalChecksum(ucChkSum);
    // Do the write	
	i2c_start();
	if( write_byte(0xb4)!=ACK )	return FAIL_ACK;
	if( write_byte(0x02)!=ACK )	return FAIL_ACK;
	if( write_byte(0x00)!=ACK )	return FAIL_ACK;
	if( write_byte(0x02)!=ACK )	return FAIL_ACK;
    for(i = 0; i < 2; i++) {
		if( write_byte(ucChkSum[i])!=ACK )	return FAIL_ACK;
    }
	i2c_stop();
	delay_ms(10);	
	ucReturn = TRUE;		
	// Done
    return ucReturn;
}
uchar cm_ResetCrypto(void)
{
    uchar i;
	uchar ucReturn;   
    for (i = 0; i < Gpa_Regs; ++i) ucGpaRegisters[i] = 0;
    ucCM_Encrypt = ucCM_Authenticate = FALSE;
	ucReturn = TRUE;		
	// Done
    return ucReturn;

}

// Generate next value
uchar cm_GPAGen(uchar Datain)
{
	uchar Din_gpa;
	uchar Ri, Si, Ti;
	uchar R_sum, S_sum, T_sum;
	
	// Input Character
	Din_gpa = Datain^Gpa_byte;
	Ri = Din_gpa&0x1f;   			                //Ri[4:0] = Din_gpa[4:0]
	Si = ((Din_gpa<<3)&0x78)|((Din_gpa>>5)&0x07);   //Si[6:0] = {Din_gpa[3:0], Din_gpa[7:5]}
	Ti = (Din_gpa>>3)&0x1f;  		                //Ti[4:0] = Din_gpa[7:3];
       
	//R polynomial
	R_sum = cm_Mod(RD, cm_RotR(RG), CM_MOD_R);
	RG = RF;
	RF = RE;
	RE = RD;
	RD = RC^Ri;
	RC = RB;
	RB = RA;
	RA = R_sum;
	
	//S ploynomial
	S_sum = cm_Mod(SF, cm_RotS(SG), CM_MOD_S);
	SG = SF;
	SF = SE^Si;
	SE = SD;
	SD = SC;
	SC = SB;
	SB = SA;
	SA = S_sum;
	
	//T polynomial
	T_sum = cm_Mod(TE,TC,CM_MOD_T);
	TE = TD;
	TD = TC;
	TC = TB^Ti;
	TB = TA;
	TA = T_sum;

    // Output Stage
    Gpa_byte =(Gpa_byte<<4)&0xF0;                                  // shift gpa_byte left by 4
    Gpa_byte |= ((((RA^RE)&0x1F)&(~SA))|(((TA^TD)&0x1F)&SA))&0x0F; // concat 4 prev bits and 4 new bits
	return Gpa_byte;
}
// Generate random 
uchar cm_RandGen(uchar Datain)
{
	uchar Din_rand;
	uchar Ri, Si, Ti;
	uchar R_sum;
	
	// Input Character
	Din_rand = Datain^Rand_byte;
	Ri = Din_rand&0x3f;   			                //Ri[4:0] = Din_gpa[4:0]
	Si = ((Din_rand<<3)&0x98)|((Din_rand>>5)&0x37);   //Si[6:0] = {Din_gpa[3:0], Din_gpa[7:5]}
	Ti = (Din_rand>>3)&0x17;  		                //Ti[4:0] = Din_gpa[7:3];
       
	//R polynomial
	R_sum = cm_Mod(RDD, cm_RotR(RDG), CM_MOD_R);
	RDG = RDF;
	RDF = RDE^Si;
	RDE = RDD;
	RDD = RDC^RDB;
	RDC = RDB;
	RDB = RDA^Ti;
	RDA = R_sum^Ri;	

    // Output Stage
    Rand_byte =(Rand_byte<<4)&0xF0;                                  // shift gpa_byte left by 4
    Rand_byte |= ((((RDA^RDE)&0x1F)&(~RDA))|(((RDC^RDD)&0x1F)&RDB))&0x0F; // concat 4 prev bits and 4 new bits
    Rand_byte =Rand_byte^0x1f;
    Rand_byte =Rand_byte^Si;		  
	return Rand_byte;
}

// Do authenticate/encrypt chalange encryption
void cm_AuthenEncryptCal(uchar Ci[8], uchar G_Sk[8], uchar Q[8], uchar Ch[8])
{	
    uchar i, j;
    // Reset all registers
    cm_ResetCrypto();
    // Setup the cyptographic registers
    for(j = 0; j < 4; j++) {
	    for(i = 0; i<3; i++) cm_GPAGen(Ci[2*j]);	
	    for(i = 0; i<3; i++) cm_GPAGen(Ci[2*j+1]);
	    cm_GPAGen(Q[j]);
    }
    for(j = 0; j<4; j++ ) {
	    for(i = 0; i<3; i++) cm_GPAGen(G_Sk[2*j]);
	    for(i = 0; i<3; i++) cm_GPAGen(G_Sk[2*j+1]);
	    cm_GPAGen(Q[j+4]);
    }
    // begin to generate Ch
    cm_GPAGenN(6);                    // 6 0x00s
    Ch[0] = Gpa_byte;

    for (j = 1; j<8; j++) {
	    cm_GPAGenN(7);                // 7 0x00s
	    Ch[j] = Gpa_byte;
    }
    // then calculate new Ci and Sk, to compare with the new Ci and Sk read from eeprom
    Ci[0] = 0xff;		              // reset AAC 
    for(j = 1; j<8; j++) {
	    cm_GPAGenN(2);                // 2 0x00s
	      Ci[j] = Gpa_byte;
    }

    for(j = 0; j<8; j++) {
	     cm_GPAGenN(2);                // 2 0x00s
	     G_Sk[j] = Gpa_byte;
    }
	cm_GPAGenN(3);                    // 3 0x00s
}
// Calaculate Checksum
void cm_CalChecksum(uchar Ck_sum[2])
{
	cm_GPAGenN(15);                    // 15 0x00s
	Ck_sum[0] = Gpa_byte;
	cm_GPAGenN(5);                     // 5 0x00s
	Ck_sum[1] = Gpa_byte;	
}
// The following functions are "macros" for commonly done actions
// Clock some zeros into the state machine
void cm_GPAGenN(uchar Count)
{
    while(Count--) cm_GPAGen(0x00);
}
// Clock some zeros into the state machine, then clock in a byte of data
void cm_GPAGenNF(uchar Count, uchar DataIn)
{
    cm_GPAGenN(Count);                             // First ones are allways zeros
    cm_GPAGen(DataIn);                             // Final one is sometimes different
}
// Include 2 bytes of a command into a polynominal
void cm_GPAcmd2(uchar ucInsBuff[16])
{
	  cm_GPAGenNF(5, ucInsBuff[2]);
	  cm_GPAGenNF(5, ucInsBuff[3]);
}
// Include 3 bytes of a command into a polynominal
void cm_GPAcmd3(uchar ucInsBuff[16])
{
	  cm_GPAGenNF(5, ucInsBuff[1]);
	  cm_GPAcmd2(ucInsBuff);
}
// Include the data in the polynominals and decrypt it required
void cm_GPAdecrypt(uchar ucEncrypt, uchar ucBuffer[16], uchar ucCount)
{
	  uchar i;
	  for (i = 0; i < ucCount; ++i) {
          if (ucEncrypt) ucBuffer[i] = ucBuffer[i]^Gpa_byte;
 	      cm_GPAGen(ucBuffer[i]);
          cm_GPAGenN(5);        // 5 clocks with 0x00 data
    }
}
// Include the data in the polynominals and encrypt it required
void cm_GPAencrypt(uchar ucEncrypt, uchar ucBuffer[16], uchar ucCount)
{
    uchar i, ucData; 
  	for (i = 0; i<ucCount; i++) {
  		cm_GPAGenN(5);                          // 5 0x00s
  		ucData = ucBuffer[i];
  		if (ucEncrypt) ucBuffer[i] = ucBuffer[i]^Gpa_byte;
  		cm_GPAGen(ucData);
  	}
}
//============================
//I2C Write one byte  
uchar write_byte(uchar byte)  
{  
    uchar i;  
    for(i=0;i<8;i++)  
        {  
            SCL_L;  
            delay_us(2);
            if(byte&0x80)SDA_H;
            else SDA_L;			 
            delay_us(2);  
            SCL_H;    
            delay_us(4);   
            byte=byte<<1;  
        } 
	SCL_L;	
	SDA_R;
	delay_us(4);
	SCL_H;
	while(i>1) {               // loop waiting for ack (loop above left i == 8)        
        if (SDA_R) i--;        // if SDA is high level decrement retry counter
        else i = 0;
    } 		
	delay_us(4);
	SCL_L;
	delay_us(2); 
	
	if(i==0)	return( ACK );
	else		return( NACK );
}  
//I2C Read one byte
uchar read_byte(void)
{
	uchar i;
	uchar rByte = 0;
	SDA_H; //set SDA input
	for(i=0x80; i; i=i>>1)
	{
		SCL_L;
		SDA_R;
		delay_us(4);
		SCL_H;
		if (SDA_R) rByte |= i;
		delay_us(4);
	}
	return rByte;
}  
//------------------------------------------  
void i2c_start(void)  
{  
    SCL_L;  
    delay_us(2);  
    SDA_H;                              //a falling edge of sda when scl is high   
    delay_us(2);   
    SCL_H;        
    delay_us(2);                                                                         
    SDA_L;        
    delay_us(2);                                                                         
    SCL_L;                              //keep the SDA and SCL low;  
    delay_us(4);                                                                                
}  
//---------------------------------------  
void i2c_stop(void)  
{  
    SCL_L;                             //a upper edge of sda when scl is high  
    delay_us(2);  
    SDA_L;  
    delay_us(2);  
    SCL_H;  
    delay_us(2);  
    SDA_H;   
    delay_us(2);  
    SCL_L;  
    delay_us(2);  
    SDA_L;                            //keep the SDA and SCL low;         
    delay_us(4); 		              
}     
//---------------------------------------  
void cm_AckNak(uchar ucAck)
{
	SCL_L;
	if (ucAck) SDA_L;               // Low on data line indicates an ACK
	else       SDA_H;               // High on data line indicates an NACK
	delay_us(4);
	SCL_H;
	delay_us(4);
	SCL_L;
}

//--------------------------------------------
