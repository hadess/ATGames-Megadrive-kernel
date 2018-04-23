//=====================
//#include "stm32f10x.h"
//=====================
//Define IO
#ifndef CUSTOM_H_
#define CUSTOM_H_

//typedef unsigned int u32;
//typedef unsigned char uchar;

#define SCL_H         write_gpio(SCL_GPIO_PIN, 1)//GPIOD->BSRR = GPIO_Pin_7			//输出口，输出高电平
#define SCL_L         write_gpio(SCL_GPIO_PIN, 0)//GPIOD->BRR  = GPIO_Pin_7  		//输出口，输出低电平
#define SDA_H         write_gpio(SDA_GPIO_PIN, 1)//GPIOD->BSRR = GPIO_Pin_12			//输入口，依靠外部上拉电阻驱动高电平
#define SDA_L         write_gpio(SDA_GPIO_PIN, 0)//GPIOD->BRR  = GPIO_Pin_12			//输出口，输出低电平
#define SDA_R         read_gpio(SDA_GPIO_PIN)//GPIOD->IDR  & GPIO_Pin_12			//输入口，同SDA_H，区分接收应答位和



#endif