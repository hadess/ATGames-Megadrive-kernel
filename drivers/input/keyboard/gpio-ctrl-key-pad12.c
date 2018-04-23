/*
 * gpio detection  driver
 *
 * Copyright (C) 2015 Rockchip Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/rockchip/iomap.h>
#include <linux/rockchip/grf.h>
#include <dt-bindings/pinctrl/rockchip-rk3036.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
//#include <linux/delay.h>


struct gpio_data {
	struct gpio_ctrl *parent;
	int id;
	int val;
	int output;
	const char *name;
	struct device dev;
};

struct gpio_ctrl {
	struct class_attribute cls_attr;
	struct device *dev;
	int num;
	int info;
	struct gpio_data *data;
	struct wake_lock wake_lock;
};
struct code_t {
	int index;
	int code;
	int event;
	int lastevent;
	int press;
};


static struct class *gpio_ctrl_class;
static struct hrtimer gpio_ctrl_keys_pad12_timer;
static struct hrtimer gpio_ctrl_pad3_timer;

struct input_dev *gpio_ctrl_key_pad12_input = NULL;


static int timerCount;
static enum hrtimer_restart gpio_ctrl_keys_pad12_poll(struct hrtimer *timer);

struct gpio_data *parse_pad12_data;
#define GPIO_KEYS_SAMPLE_TIME 15;
#define DPAD1_CLK_NUM 1
#define DPAD2_CLK_NUM 10

#define IO1			0
#define IO2			1
#define IO3			2
#define IO4			3
#define IO5			4
#define IO6			5
#define IO7			6
#define IO8			7
#define IO9			8
#define IO10			9
#define IO11			10
#define IO12			11
#define IO13			12
#define IO14			13
#define IO15			14
#define IO16			15
#define IO17			16
#define IO18			17
#define IO19			18
#define IO20			19
#define IO21			20

#define P1_D0		IO7
#define P1_D1		IO6
#define P1_D2		IO5
#define P1_D3		IO4
#define P1_D4		IO1
#define P1_SL		IO2
#define P1_D5		IO3
#define P1_GN		// GN
#define P1_5V		// 4.5 V

#define P2_D0		IO13
#define P2_D1		IO14
#define P2_D2		IO15
#define P2_D3		IO16
#define P2_D4		IO12
#define P2_SL		IO11
#define P2_D5		IO10
#define P2_GN		// GN
#define P2_5V		// 4.5 V

#define P3_D0		IO20
#define P3_D1		IO19
#define P3_D2		IO18
#define P3_D3		IO17
#define P3_SL		IO21

#define ACTION_DOWN			0
#define ACTION_UP				1

#define KEYCODE_MENU 			59
#define KEYCODE_HOME 			3
#define KEYCODE_BACK			4

#define KEYCODE_BUTTON_Z 		101
#define KEYCODE_BUTTON_Y 		100
#define KEYCODE_BUTTON_X 		99
#define KEYCODE_BUTTON_MODE 	110
#define KEYCODE_BUTTON_A 		96
#define KEYCODE_BUTTON_START 	108
#define KEYCODE_DPAD_UP		19
#define KEYCODE_DPAD_DOWN	20
#define KEYCODE_DPAD_LEFT		21
#define KEYCODE_DPAD_RIGHT 	22
#define KEYCODE_BUTTON_B 		97
#define KEYCODE_BUTTON_C 		98

#define KEYCODE_MEDIA_REWIND	89
#define KEYCODE_MEDIA_RECORD	130

#define _Z_ 		0ULL
#define _Y_ 		1ULL
#define _X_ 		2ULL
#define _MD_ 		3ULL
#define _A_ 		4ULL
#define _ST_ 		5ULL
#define _UP_		6ULL
#define _DW_		7ULL
#define _LF_		8ULL
#define _RG_ 		9ULL
#define _B_ 		10ULL
#define _C_ 		11ULL
#define _RS_		12ULL
#define _SV_		13ULL
#define SHIFT	(_SV_+1ULL)

int dpad12Count;


struct code_t codes12[] = {
            {0, KEY_F1,0,0,0},
            {1, KEY_F2,0,0,0},
            {2, KEY_F3,0,0,0},
            {3, KEY_MENU,0,0,0},
            {4, KEY_F4,0,0,0},
            {5, KEY_EJECTCD,0,0,0},
            {6, KEY_UP,0,0,0},
            {7, KEY_DOWN,0,0,0},
            {8, KEY_LEFT,0,0,0},
            {9, KEY_RIGHT,0,0,0},
            {10, KEY_F6,0,0,0},
            {11, KEY_F7,0,0,0},
            {12, KEY_REWIND,0,0,0},
            {13, KEY_RECORD,0,0,0},

            {14, KEY_F8,0,0,0},
            {15, KEY_F9,0,0,0},
            {16, KEY_F10,0,0,0},
            {17, KEY_MENU,0,0,0},
            {18, KEY_F11,0,0,0},
            {19, KEY_F12,0,0,0},
            {20, KEY_PAGEUP,0,0,0},
            {21, KEY_PAGEDOWN,0,0,0},
            {22, KEY_END,0,0,0},
            {23, KEY_FORWARD,0,0,0},
            {24, KEY_STOPCD,0,0,0},
            {25, KEY_LEFTMETA,0,0,0},
            {26, KEY_RIGHTMETA,0,0,0},
            {27, KEY_BREAK,0,0,0},
    };

static void clearKeyPx(int index)
{
	int groupLen = sizeof(codes12) /sizeof(struct code_t) / 2;
	int i ;
	int base = index * groupLen;
	
	for(i = base; i < base + groupLen; i++)
	{
		if(codes12[i].press == 1)
		{
			input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[i].code, 0);
			input_sync(gpio_ctrl_key_pad12_input);			
		}	
		codes12[i].event = 0;
		codes12[i].lastevent = 0;
		codes12[i].press = 0;
	}
}
static void clearAllKey(void)
{
	clearKeyPx(0);
	clearKeyPx(1);
}



/*
 *gpio_det_unregister_client - unregister a client notifier
 *@nb: notifier block to callback on events
 */

static ssize_t gpio_ctrl_info_show(struct class *class, struct class_attribute *attr,
				char *buf)
{
	struct gpio_ctrl *gpio_ctrl = container_of(attr, struct gpio_ctrl, cls_attr);;

	return sprintf(buf, "%d\n", gpio_ctrl->info);
}



static int gpio_ctrl_gpiovalue_show(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	struct gpio_data *gpiod = container_of(dev, struct gpio_data, dev);
	unsigned int val = gpio_get_value(gpiod->id)+(gpiod->val/2)*2;

	return sprintf(buf, "%d\n", val);
}

static ssize_t gpio_ctrl_gpiovalue_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct gpio_data *gpiod;
	int val,ret;
	unsigned int grf_value;
	gpiod = container_of(dev, struct gpio_data, dev);
	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return ret;
	if (val <= 0) {
		gpiod->val=0;
	} else if(val == 1) {
		gpiod->val = 1;
	} else if(val >= 2) {
		gpiod->val=2;
	}
	if(gpiod->val<2){
		gpio_direction_output(gpiod->id,gpiod->val );
		//gpio_set_value(gpiod->id,gpiod-val);
	}else{
		gpio_direction_input(gpiod->id);
	}
	//writel_relaxed(RK_GRF_VIRT + RK3036_GRF_GPIO1C_IOMUX,0x00);
	//grf_value=readl_relaxed(RK_GRF_VIRT + RK3036_GRF_GPIO1C_IOMUX);
//	dev_err(dev, "grf_value = %d,gpio_id=%d,gpio_val=%d\n",grf_value,gpiod->id,gpiod->val);
	return count;
}

static struct device_attribute gpio_ctrl_attrs[] = {
	__ATTR(gpio_ctrl_gpiovalue, 0777, gpio_ctrl_gpiovalue_show, gpio_ctrl_gpiovalue_store),
	__ATTR_NULL,
};


static int __init gpio_ctrl_class_init(void)
{
#if 0	
	gpio_ctrl_class = class_create(THIS_MODULE, "gpio-ctrl");
	if (IS_ERR(gpio_ctrl_class)) {
		pr_err("create gpio_detection class failed (%ld)\n",
		       PTR_ERR(gpio_ctrl_class));
		return PTR_ERR(gpio_ctrl_class);
	}
	gpio_ctrl_class->dev_attrs = gpio_ctrl_attrs;
	return 0;
#else
	return -1;
#endif	
}

static int gpio_ctrl_class_register(struct gpio_ctrl *gpiod_ctrl,
					 struct gpio_data *gpiod)
{
	int ret;

	gpiod->dev.class = gpio_ctrl_class;
	dev_set_name(&gpiod->dev, "%s", gpiod->name);
	dev_set_drvdata(&gpiod->dev, gpiod_ctrl);
	ret = device_register(&gpiod->dev);
	
	return ret;
}

static int gpio_ctrl_parse_dt(struct gpio_ctrl *gpiod_ctrl)
{
	struct gpio_data *data;
	struct device_node *node;
	struct gpio_data *gpiod;
	int ret;
	int i = 0;
	char *inoutput;
	int num = of_get_child_count(gpiod_ctrl->dev->of_node);
	int val;
	val = readl_relaxed(RK_GRF_VIRT + RK3036_GRF_SOC_CON0);
	val &=~0x800;
	writel_relaxed(val|0x08000000, RK_GRF_VIRT + RK3036_GRF_SOC_CON0);

	if (num == 0)
		return -ENODEV;
	parse_pad12_data = devm_kzalloc(gpiod_ctrl->dev, num * sizeof(*data), GFP_KERNEL);
	if (!parse_pad12_data)
		return -ENOMEM;
	for_each_child_of_node(gpiod_ctrl->dev->of_node, node) {
		enum of_gpio_flags flags;

		gpiod = &parse_pad12_data[i++];
		gpiod->parent = gpiod_ctrl;
		gpiod->id = of_get_gpio_flags(node, 0, &flags);
		if (!gpio_is_valid(gpiod->id)) {
			dev_err(gpiod_ctrl->dev, "failed to get gpio\n");
			num--;
			i--;
			continue;
		}
		gpiod->val = !(flags & OF_GPIO_ACTIVE_LOW);
		gpiod->name = of_get_property(node, "label", NULL);
		devm_gpio_request(gpiod_ctrl->dev,gpiod->id,gpiod->name);
		inoutput= of_get_property(node, "inoutput", NULL);
		gpiod->output = (strcmp(inoutput,"output")==0)?1:0;
		gpio_direction_input(gpiod->id);
		
		//gpio_set_value(gpiod->id,gpiod->val );
	}
	gpiod_ctrl->num = num;
	gpiod_ctrl->data = parse_pad12_data;

	gpio_direction_output(parse_pad12_data[DPAD1_CLK_NUM].id,0); //add by he
	gpio_direction_output(parse_pad12_data[DPAD2_CLK_NUM].id,0); //add by he
	return 0;
}

static struct gpio_ctrl *gpiod_ctrl_save = NULL;

static int gpio_ctrl_probe(struct platform_device *pdev)
{
	struct gpio_ctrl *gpiod_ctrl;
	struct gpio_data *gpiod;
	int val;
	int i;
	int j;
	int ret;
	int error;
	struct input_dev *input = NULL;
	printk("%s,%s\n",__func__,__func__);

	gpiod_ctrl = devm_kzalloc(&pdev->dev, sizeof(*gpiod_ctrl), GFP_KERNEL);
	gpiod_ctrl_save = gpiod_ctrl;
	if (!gpiod_ctrl)
		return -ENOMEM;
	gpiod_ctrl->dev = &pdev->dev;
	gpiod_ctrl->cls_attr.attr.name = "info";
	gpiod_ctrl->cls_attr.attr.mode = S_IRUGO;
	gpiod_ctrl->cls_attr.show = gpio_ctrl_info_show;
	dev_set_name(gpiod_ctrl->dev, "gpio_ctrl_pad12");
	if (!pdev->dev.of_node)
		return -EINVAL;
	gpio_ctrl_parse_dt(gpiod_ctrl);
	input = devm_input_allocate_device(gpiod_ctrl->dev);
	if (!input) {
		error = -ENOMEM;
		return error;
	}
	input->name = "gpio-ctrl-keypad-pad12";//pdev->name;
	input->phys = "gpio-ctrl-keys-pad12/input5";
	input->dev.parent = gpiod_ctrl->dev;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	
	//__set_bit(EV_REP, input->evbit);
	//__set_bit(EV_KEY, input->evbit);
	input->evbit[0] = BIT_MASK(EV_KEY);	
	for (j = 0; j < 27; j++) {
		input_set_capability(input, EV_KEY, codes12[j].code);
	}
	
	
	error = input_register_device(input);
	if (error) {
		pr_err("gpio-keys: Unable to register input device, "
			"error: %d\n", error);
		return error;
	}
	gpio_ctrl_key_pad12_input = input;

	printk("%s",__func__);

	hrtimer_init(&gpio_ctrl_keys_pad12_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gpio_ctrl_keys_pad12_timer.function = gpio_ctrl_keys_pad12_poll;

	//hrtimer_start(&gpio_ctrl_keys_pad12_timer, ktime_set(1, 10000), HRTIMER_MODE_REL);



	return 0;
}

static int gpio_ctrl_remove(struct platform_device *dev)
{
	return 0;
}

static void gpio_ctrl_shutdown(struct platform_device *dev)
{
}

void wait_gpio_output()
{
	volatile int timeout = 0xa0;   //lowest latency : 0x70  (0x50 是临界点，部分按键无效)
	while(timeout--);
}

int get_cpsr_asm(void)
{
	int cpsr;
	__asm__ __volatile__(
	"mrs    r0, cpsr\n\t"
	"mov		%0, r0"
	:
	: "r"(cpsr));
	//printk("cpsr = %x\n", cpsr);
	return cpsr;
}


int testpad12 = 0;
extern int test_ctrl_key;
static int first_test_flag = 1;;
static int key_Attached[2] = {0xffff, 0xffff};  // 0xffff  -- invalid value; 0 --- unattached; 1 ---- attached
unsigned int wirePadStatus[2] ={0,0}; // 0 : 没有插入   3： 3keys 手柄插入， 6： 6keys 手柄插入
/****
	
Sel   D0 D1 D2 D3 D4 D5

Low   UP DW LO LO A  ST
High  UP DW LF RG B  C
Low   UP DW LO LO A  ST
High  UP DW LF RG B  C
Low   LO LO LO LO A  ST
High  Z  Y  X  MD HI HI
Low   HI HI HI HI A  ST
High  UP DW LF RG B  C
Low   UP DW LO LO A  ST
****/
static enum hrtimer_restart gpio_ctrl_keys_pad12_poll(struct hrtimer *timer)
{
	//printk("en sc.\n");
//	printk( "%s timerCount = %d\n",__func__,gpio_get_value(parse_pad12_data[1].id));
    int save, now;
	int Dpad1_Val0;
	int Dpad1_Val1;
	int Dpad1_Val2;
	int Dpad1_Val3;
	int Dpad1_Val4;
	int Dpad1_Val5;
	int DPAD_CLK_NUM;
	int DPAD_PD0,DPAD_PD1,DPAD_PD2,DPAD_PD3,DPAD_PD4,DPAD_PD5;
	int keyCode[28];
	int isAttach;
	int j=0;
	int *pLastAttach, *pWirePadStatus;
	unsigned int  delayForTimer = 0;  // pulg in , it is 500 * 1000000 ns;
	//int Dpad1_Val0;
/****
dpad2
****/
	//for test
	//printk("{");
	#if 0
	if(first_test_flag)
	{
		first_test_flag = 0;
		printk("--find embbed condition in wire(%d, %d, %s,0x%x)\n",in_irq(),in_softirq(), current->comm, get_cpsr_asm());
	}
	testpad12 = 1;
	if(test_ctrl_key)
	{
		printk("find embbed condition in wire(%d, %d, %s, 0x%x)\n",in_irq(),in_softirq(), current->comm, get_cpsr_asm());

	}
	#endif
	
	dpad12Count++;
	if(dpad12Count>=2){
		DPAD_PD0=P1_D0;
		DPAD_PD1=P1_D1;
		DPAD_PD2=P1_D2;
		DPAD_PD3=P1_D3;
		DPAD_PD4=P1_D4;
		DPAD_PD5=P1_D5;
		DPAD_CLK_NUM=DPAD1_CLK_NUM;
		dpad12Count=0;
		pLastAttach = &key_Attached[0]; 
		pWirePadStatus = &wirePadStatus[0];
	}else{
		DPAD_PD0=P2_D0;
		DPAD_PD1=P2_D1;
		DPAD_PD2=P2_D2;
		DPAD_PD3=P2_D3;
		DPAD_PD4=P2_D4;
		DPAD_PD5=P2_D5;
		DPAD_CLK_NUM=DPAD2_CLK_NUM;
		pLastAttach = &key_Attached[1]; 
		pWirePadStatus = &wirePadStatus[1];
	}
	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,0);  //by he 2017/4/15 16:03:46
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,0); //  1
	wait_gpio_output();
#if 0	
	gpio_direction_input(parse_pad12_data[DPAD_PD0].id);
	gpio_direction_input(parse_pad12_data[DPAD_PD1].id);
	gpio_direction_input(parse_pad12_data[DPAD_PD2].id);
	gpio_direction_input(parse_pad12_data[DPAD_PD3].id);
	gpio_direction_input(parse_pad12_data[DPAD_PD4].id);
	gpio_direction_input(parse_pad12_data[DPAD_PD5].id);
#endif	
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);
	//printk("%s, P2_D0=%d \n",__func__,Dpad1_Val0);
	isAttach=0;
	
	//add for 3keys pads
	if(Dpad1_Val0 || Dpad1_Val1) //down key and up key cannot pressed together.
	{
		//printk("find key\n");
		//{volatile int delay; for(delay=0; delay < 0x40; delay++);}	
		isAttach = 1;
	}
	
	//add for 3key's question.
	if((!isAttach) && (*pLastAttach == 1))
	{
		printk("find key unattached!!!\n");
		*pWirePadStatus = 0;  
		if(dpad12Count == 0)
		{
			clearKeyPx(0);
		}
		else
		{
			clearKeyPx(1);
		}
	}
	if((isAttach) && (*pLastAttach != 1))
	{	
		printk("find key attached...?\n");
		delayForTimer = 500 * 1000000;
	}
	
	*pLastAttach = isAttach;
	if(delayForTimer > 0)
		goto nextcycle;

	save = codes12[_UP_+dpad12Count*SHIFT].lastevent;
	now = codes12[_UP_+dpad12Count*SHIFT].event = !Dpad1_Val0;
	//codes12[_UP_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, upkey now=%d code=0x%x\n",__func__,now,codes12[_UP_+dpad12Count*SHIFT].code);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_UP_+dpad12Count*SHIFT].code, codes12[_UP_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_UP_+dpad12Count*SHIFT].press=1;
		}
		codes12[_UP_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_DW_+dpad12Count*SHIFT].lastevent;
	now = codes12[_DW_+dpad12Count*SHIFT].event = !Dpad1_Val1;
	//codes12[_DW_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, downkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_DW_+dpad12Count*SHIFT].code, codes12[_DW_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_DW_+dpad12Count*SHIFT].press=1;
		}
		codes12[_DW_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_A_+dpad12Count*SHIFT].lastevent;
	now = codes12[_A_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_A_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, akey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_A_+dpad12Count*SHIFT].code, codes12[_A_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_A_+dpad12Count*SHIFT].press=1;
		}
		codes12[_A_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_ST_+dpad12Count*SHIFT].lastevent;
	now = codes12[_ST_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_ST_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, stkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_ST_+dpad12Count*SHIFT].code, codes12[_ST_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_ST_+dpad12Count*SHIFT].press=1;
		}
		codes12[_ST_+dpad12Count*SHIFT].lastevent = now;
	}

	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,1);
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,1);   //  2
	wait_gpio_output();
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);
	
	save = codes12[_UP_+dpad12Count*SHIFT].lastevent;
	now = codes12[_UP_+dpad12Count*SHIFT].event = !Dpad1_Val0;
	//codes12[_UP_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, upkey now=%d code=0x%x\n",__func__,now,codes12[_UP_+dpad12Count*SHIFT].code);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_UP_+dpad12Count*SHIFT].code, codes12[_UP_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_UP_+dpad12Count*SHIFT].press=1;
		codes12[_UP_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_DW_+dpad12Count*SHIFT].lastevent;
	now = codes12[_DW_+dpad12Count*SHIFT].event = !Dpad1_Val1;
	//codes12[_DW_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, downkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_DW_+dpad12Count*SHIFT].code, codes12[_DW_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_DW_+dpad12Count*SHIFT].press=1;
		codes12[_DW_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_LF_+dpad12Count*SHIFT].lastevent;
	now = codes12[_LF_+dpad12Count*SHIFT].event = !Dpad1_Val2;
	//codes12[_LF_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, lfkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_LF_+dpad12Count*SHIFT].code, codes12[_LF_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_LF_+dpad12Count*SHIFT].press=1;
		codes12[_LF_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_RG_+dpad12Count*SHIFT].lastevent;
	now = codes12[_RG_+dpad12Count*SHIFT].event = !Dpad1_Val3;
	//codes12[_RG_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, rgkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_RG_+dpad12Count*SHIFT].code, codes12[_RG_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_RG_+dpad12Count*SHIFT].press=1;
		codes12[_RG_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_B_+dpad12Count*SHIFT].lastevent;
	now = codes12[_B_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_B_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, bkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_B_+dpad12Count*SHIFT].code, codes12[_B_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_B_+dpad12Count*SHIFT].press=1;
		codes12[_B_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_C_+dpad12Count*SHIFT].lastevent;
	now = codes12[_C_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_C_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, ckey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_C_+dpad12Count*SHIFT].code, codes12[_C_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_C_+dpad12Count*SHIFT].press=1;
		codes12[_C_+dpad12Count*SHIFT].lastevent = now;
	}
	
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,0);  //  3
	wait_gpio_output();
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);
	save = codes12[_UP_+dpad12Count*SHIFT].lastevent;
	now = codes12[_UP_+dpad12Count*SHIFT].event = !Dpad1_Val0;
	//codes12[_UP_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, upkey now=%d code=0x%x\n",__func__,now,codes12[_UP_+dpad12Count*SHIFT].code);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_UP_+dpad12Count*SHIFT].code, codes12[_UP_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_UP_+dpad12Count*SHIFT].press=1;
		}
		codes12[_UP_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_DW_+dpad12Count*SHIFT].lastevent;
	now = codes12[_DW_+dpad12Count*SHIFT].event = !Dpad1_Val1;
	//codes12[_DW_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, downkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_DW_+dpad12Count*SHIFT].code, codes12[_DW_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_DW_+dpad12Count*SHIFT].press=1;
		}
		codes12[_DW_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_A_+dpad12Count*SHIFT].lastevent;
	now = codes12[_A_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_A_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, akey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_A_+dpad12Count*SHIFT].code, codes12[_A_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_A_+dpad12Count*SHIFT].press=1;
		}
		codes12[_A_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_ST_+dpad12Count*SHIFT].lastevent;
	now = codes12[_ST_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_ST_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, stkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_ST_+dpad12Count*SHIFT].code, codes12[_ST_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_ST_+dpad12Count*SHIFT].press=1;
		}
		codes12[_ST_+dpad12Count*SHIFT].lastevent = now;
	}

	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,0);
	
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,1);   //  4
	wait_gpio_output();
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);
	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,1);
	save = codes12[_UP_+dpad12Count*SHIFT].lastevent;
	now = codes12[_UP_+dpad12Count*SHIFT].event = !Dpad1_Val0;
	//codes12[_UP_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, upkey now=%d code=0x%x\n",__func__,now,codes12[_UP_+dpad12Count*SHIFT].code);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_UP_+dpad12Count*SHIFT].code, codes12[_UP_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_UP_+dpad12Count*SHIFT].press=1;
		codes12[_UP_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_DW_+dpad12Count*SHIFT].lastevent;
	now = codes12[_DW_+dpad12Count*SHIFT].event = !Dpad1_Val1;
	//codes12[_DW_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, downkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_DW_+dpad12Count*SHIFT].code, codes12[_DW_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_DW_+dpad12Count*SHIFT].press=1;
		codes12[_DW_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_LF_+dpad12Count*SHIFT].lastevent;
	now = codes12[_LF_+dpad12Count*SHIFT].event = !Dpad1_Val2;
	//codes12[_LF_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, lfkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_LF_+dpad12Count*SHIFT].code, codes12[_LF_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_LF_+dpad12Count*SHIFT].press=1;
		codes12[_LF_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_RG_+dpad12Count*SHIFT].lastevent;
	now = codes12[_RG_+dpad12Count*SHIFT].event = !Dpad1_Val3;
	//codes12[_RG_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, rgkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_RG_+dpad12Count*SHIFT].code, codes12[_RG_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_RG_+dpad12Count*SHIFT].press=1;
		codes12[_RG_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_B_+dpad12Count*SHIFT].lastevent;
	now = codes12[_B_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_B_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, bkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_B_+dpad12Count*SHIFT].code, codes12[_B_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_B_+dpad12Count*SHIFT].press=1;
		codes12[_B_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_C_+dpad12Count*SHIFT].lastevent;
	now = codes12[_C_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_C_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, ckey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_C_+dpad12Count*SHIFT].code, codes12[_C_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		codes12[_C_+dpad12Count*SHIFT].press=1;
		codes12[_C_+dpad12Count*SHIFT].lastevent = now;
	}
	
	
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,0);   //  5
	wait_gpio_output(1);
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);
	if(isAttach==1 && ((Dpad1_Val0==0)&&(Dpad1_Val1==0)&&(Dpad1_Val2==0)&&(Dpad1_Val3==0))){
		//isAttach=1;
		*pWirePadStatus = 6;
	}else if(isAttach == 1){
		*pWirePadStatus = 3;	
		goto last_start_hrtimer;
	}	
	save = codes12[_A_+dpad12Count*SHIFT].lastevent;
	now = codes12[_A_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_A_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, akey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_A_+dpad12Count*SHIFT].code, codes12[_A_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_A_+dpad12Count*SHIFT].press=1;
		}
		codes12[_A_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_ST_+dpad12Count*SHIFT].lastevent;
	now = codes12[_ST_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_ST_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, stkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_ST_+dpad12Count*SHIFT].code, codes12[_ST_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val2==0)&&(Dpad1_Val3==0)){
			codes12[_ST_+dpad12Count*SHIFT].press=1;
		}
		codes12[_ST_+dpad12Count*SHIFT].lastevent = now;
	}
	
	

	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,0);
	
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,1);   //  6
	wait_gpio_output();
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);

	save = codes12[_Z_+dpad12Count*SHIFT].lastevent;
	now = codes12[_Z_+dpad12Count*SHIFT].event = !Dpad1_Val0;
	//codes12[_Z_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, zkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_Z_+dpad12Count*SHIFT].code, codes12[_Z_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val4==1)&&(Dpad1_Val5==1)){
			codes12[_Z_+dpad12Count*SHIFT].press=1;
		}
		codes12[_Z_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_Y_+dpad12Count*SHIFT].lastevent;
	now = codes12[_Y_+dpad12Count*SHIFT].event = !Dpad1_Val1;
	//codes12[_Y_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, ykey now=%d \n",__func__,now);
	//	input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_Y_+dpad12Count*SHIFT].code, codes12[_Y_+dpad12Count*SHIFT].event);
	//	input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val4==1)&&(Dpad1_Val5==1)){
			codes12[_Y_+dpad12Count*SHIFT].press=1;
		}
		codes12[_Y_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_X_+dpad12Count*SHIFT].lastevent;
	now = codes12[_X_+dpad12Count*SHIFT].event = !Dpad1_Val2;
	//codes12[_X_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, xkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_X_+dpad12Count*SHIFT].code, codes12[_X_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val4==1)&&(Dpad1_Val5==1)){
			codes12[_X_+dpad12Count*SHIFT].press=1;
		}
		codes12[_X_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_MD_+dpad12Count*SHIFT].lastevent;
	now = codes12[_MD_+dpad12Count*SHIFT].event = !Dpad1_Val3;
	//codes12[_MD_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, mdkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_MD_+dpad12Count*SHIFT].code, codes12[_MD_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if((Dpad1_Val4==1)&&(Dpad1_Val5==1)){
			codes12[_MD_+dpad12Count*SHIFT].press=1;
		}
		codes12[_MD_+dpad12Count*SHIFT].lastevent = now;
	}
	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,1);
	
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,0);   //  7
	wait_gpio_output();
	Dpad1_Val0 = gpio_get_value(parse_pad12_data[DPAD_PD0].id);
	Dpad1_Val1 = gpio_get_value(parse_pad12_data[DPAD_PD1].id);
	Dpad1_Val2 = gpio_get_value(parse_pad12_data[DPAD_PD2].id);
	Dpad1_Val3 = gpio_get_value(parse_pad12_data[DPAD_PD3].id);
	Dpad1_Val4 = gpio_get_value(parse_pad12_data[DPAD_PD4].id);
	Dpad1_Val5 = gpio_get_value(parse_pad12_data[DPAD_PD5].id);

	if((Dpad1_Val0==1)&&(Dpad1_Val1==1)&&(Dpad1_Val2==1)&&(Dpad1_Val3==1)){
		if(isAttach==1){
			isAttach=1;
		}
	}else{
		isAttach=0;
	}
	
	save = codes12[_A_+dpad12Count*SHIFT].lastevent;
	now = codes12[_A_+dpad12Count*SHIFT].event = !Dpad1_Val4;
	//codes12[_A_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, akey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_A_+dpad12Count*SHIFT].code, codes12[_A_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if(isAttach){
			codes12[_A_+dpad12Count*SHIFT].press=1;
		}
		codes12[_A_+dpad12Count*SHIFT].lastevent = now;
	}
	save = codes12[_ST_+dpad12Count*SHIFT].lastevent;
	now = codes12[_ST_+dpad12Count*SHIFT].event = !Dpad1_Val5;
	//codes12[_ST_+dpad12Count*SHIFT].press=0;
	if (now != save){
		//printk("%s, stkey now=%d \n",__func__,now);
		//input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[_ST_+dpad12Count*SHIFT].code, codes12[_ST_+dpad12Count*SHIFT].event);
		//input_sync(gpio_ctrl_key_pad12_input);
		if(isAttach){
			codes12[_ST_+dpad12Count*SHIFT].press=1;
		}
		codes12[_ST_+dpad12Count*SHIFT].lastevent = now;
	}
	

	//gpio_direction_output(parse_pad12_data[DPAD_CLK_NUM].id,0);
last_start_hrtimer:
	gpio_set_value(parse_pad12_data[DPAD_CLK_NUM].id,1);   //  8
	wait_gpio_output();
	for(j=dpad12Count*SHIFT;j<(dpad12Count+1)*SHIFT;j++){
		if(codes12[j].press==1){
			if(isAttach==1){
				input_event(gpio_ctrl_key_pad12_input, EV_KEY, codes12[j].code, codes12[j].event);
				input_sync(gpio_ctrl_key_pad12_input);
			}
			codes12[j].press=0;
		}
	}
	
nextcycle:	
	if(dpad12Count==1){
		hrtimer_start(timer, ktime_set(0, delayForTimer + 15000), HRTIMER_MODE_REL);
	}else{
		hrtimer_start(timer, ktime_set(0, delayForTimer + 15000000), HRTIMER_MODE_REL);
	}
	//printk("}");
	testpad12 = 0;
	return HRTIMER_NORESTART;
}

extern int gpio_bank_get_value(unsigned gpio);
extern int gpio_bank_set_value(unsigned gpio,int value, unsigned int mask);
extern int gpio_bank_set_direction(unsigned gpio,int dir, unsigned int mask);

void startScankey(){
	int ret;
	printk("startScankey begin %p..\n", gpiod_ctrl_save);
	dpad12Count = 0;
	key_Attached[0] = 0xffff;
	key_Attached[1] = 0xffff;
	wirePadStatus[0] = 0;
	wirePadStatus[1] = 0;
	clearAllKey();
	//init gpio status
	if(gpiod_ctrl_save != NULL)
	{
		/*gpio_direction_input(parse_pad12_data[P1_D0].id);
		gpio_direction_input(parse_pad12_data[P1_D1].id);
		gpio_direction_input(parse_pad12_data[P1_D2].id);
		gpio_direction_input(parse_pad12_data[P1_D3].id);
		gpio_direction_input(parse_pad12_data[P1_D4].id);
		gpio_direction_input(parse_pad12_data[P1_D5].id);

		gpio_direction_input(parse_pad12_data[P2_D0].id);
		gpio_direction_input(parse_pad12_data[P2_D1].id);
		gpio_direction_input(parse_pad12_data[P2_D2].id);
		gpio_direction_input(parse_pad12_data[P2_D3].id);
		gpio_direction_input(parse_pad12_data[P2_D4].id);
		gpio_direction_input(parse_pad12_data[P2_D5].id);


		gpio_direction_output(parse_pad12_data[DPAD1_CLK_NUM].id,0); //add by he
		gpio_direction_output(parse_pad12_data[DPAD2_CLK_NUM].id,0); //add by he*/
		gpio_bank_set_direction(2, 0, (1<< 7));  //p1-d4 2a7 
		gpio_bank_set_direction(2, 0, (1<< 9));  //p1-d5 2b1 
		gpio_bank_set_direction(2, 0, (1<< 10));  //p1-d3 2b2 
		gpio_bank_set_direction(2, 0, (1<< 11));  //p1-d2 2b3 
		gpio_bank_set_direction(2, 0, (1<< 12));  //p1-d1 2b4 
		gpio_bank_set_direction(2, 0, (1<< 13));  //p1-d0 2b5 
		
		gpio_bank_set_direction(2, 0, (1<< 14));  //p2-d5 2b6
		gpio_bank_set_direction(2, 0, (1<< 16));  //p2-d4 2c0
		gpio_bank_set_direction(2, 0, (1<< 17));  //p2-d0 2c1
		gpio_bank_set_direction(2, 0, (1<< 18));  //p2-d1 2c2
		gpio_bank_set_direction(2, 0, (1<< 19));  //p2-d2 2c3
		gpio_bank_set_direction(2, 0, (1<< 20));  //p2-d3 2c4
		 
		gpio_bank_set_direction(2, (1<<15), (1<<15));  //p2-scl 2b7		
		gpio_bank_set_direction(2, (1<< 8), (1<< 8));  //p1-scl 2b0
				
	}	
		//gpio_ctrl_parse_dt(gpiod_ctrl_save);
	
	ret = hrtimer_start(&gpio_ctrl_keys_pad12_timer, ktime_set(0, 15000), HRTIMER_MODE_REL);
	
	printk("ret = %d\n", ret);
}

void stopScankey(){
	hrtimer_cancel(&gpio_ctrl_keys_pad12_timer);
	
}


EXPORT_SYMBOL(startScankey);
EXPORT_SYMBOL(stopScankey);

#if defined(CONFIG_OF)
static const struct of_device_id gpio_ctrl_keys_of_match[] = {
	{ .compatible = "gpio-ctrl-keys-pad12", },
	{},
};
#endif

static struct platform_driver gpio_ctrl_keys_driver = {
	.driver     = {
		.name   = "gpio-ctrl-keys-pad12",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(gpio_ctrl_keys_of_match),
	},
	.probe      = gpio_ctrl_probe,
	.remove     = gpio_ctrl_remove,
	.shutdown   = gpio_ctrl_shutdown,
};

static int __init gpio_ctrl_keys_pad12_init(void)
{
	//if (!gpio_ctrl_class_init())
		return platform_driver_register(&gpio_ctrl_keys_driver);
	//else
		//return -1;
}

module_init(gpio_ctrl_keys_pad12_init);

static void __exit gpio_ctrl_keys_pad12_exit(void)
{
	platform_driver_unregister(&gpio_ctrl_keys_driver);
}

module_exit(gpio_ctrl_keys_pad12_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio-detection");
MODULE_AUTHOR("ROCKCHIP");
