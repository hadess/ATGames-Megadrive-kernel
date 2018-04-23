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
#include <linux/delay.h>


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
	int initflag; 
};


static struct class *gpio_ctrl_class;
static struct hrtimer gpio_ctrl_keys_timer;
static struct hrtimer gpio_ctrl_pad3_timer;

struct delayed_work gpio_poll_work;
struct input_dev *gpio_ctrl_key_input = NULL;


static int timerCount;
static enum hrtimer_restart gpio_ctrl_keys_poll(struct hrtimer *timer);

struct gpio_data *parse_data;
#define GPIO_KEYS_SAMPLE_TIME 15;
#define DPAD1_CLK_NUM 1
#define DPAD2_CLK_NUM 10


#define IO17			0
#define IO18			1
#define IO19			2
#define IO20			3
#define IO21			4
#define IO22			5




#define P3_D0		IO20
#define P3_D1		IO19
#define P3_D2		IO18
#define P3_D3		IO17
#define P3_SL		IO21
#define P3_POWER_UP	IO22

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

int dpadCount;
int dpad3Init=0;


struct code_t codes[] = {
            {0, KEY_F1,0,0, 1},
            {1, KEY_F2,0,0, 1},
            {2, KEY_F3,0,0, 1},
            {3, KEY_MENU,0,0,1},
            {4, KEY_F4,0,0,1},
            {5, KEY_EJECTCD,0,0,1},
            {6, KEY_UP,0,0,1},
            {7, KEY_DOWN,0,0,1},
            {8, KEY_LEFT,0,0,1},
            {9, KEY_RIGHT,0,0,1},
            {10, KEY_F6,0,0,1},
            {11, KEY_F7,0,0,1},
            {12, KEY_REWIND,0,0,1},
            {13, KEY_RECORD,0,0,1},

            {14, KEY_F8,0,0,1},
            {15, KEY_F9,0,0,1},
            {16, KEY_F10,0,0,1},
            {17, KEY_MENU,0,0,1},
            {18, KEY_F11,0,0,1},
            {19, KEY_F12,0,0,1},
            {20, KEY_PAGEUP,0,0,1},
            {21, KEY_PAGEDOWN,0,0,1},
            {22, KEY_END,0,0,1},
            {23, KEY_FORWARD,0,0,1},
            {24, KEY_STOPCD,0,0,1},
            {25, KEY_LEFTMETA,0,0,1},
            {26, KEY_RIGHTMETA,0,0,1},
            {27, KEY_BREAK,0,0,1},
    };





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
	gpio_ctrl_class = class_create(THIS_MODULE, "gpio-ctrl");
	if (IS_ERR(gpio_ctrl_class)) {
		pr_err("create gpio_detection class failed (%ld)\n",
		       PTR_ERR(gpio_ctrl_class));
		return PTR_ERR(gpio_ctrl_class);
	}
	gpio_ctrl_class->dev_attrs = gpio_ctrl_attrs;
	return 0;
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
	parse_data = devm_kzalloc(gpiod_ctrl->dev, num * sizeof(*data), GFP_KERNEL);
	if (!parse_data)
		return -ENOMEM;
	for_each_child_of_node(gpiod_ctrl->dev->of_node, node) {
		enum of_gpio_flags flags;

		gpiod = &parse_data[i++];
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
		//printk("%s,%d\n",__func__,gpiod->output);
		if(gpiod->output){
			//printk("11111111111111111%s,gpio%d=%d,%d,%d\n",__func__,num,gpiod->val,gpiod->id,gpiod->output);
            gpio_direction_output(gpiod->id,gpiod->val);
		}else{
			//printk("2222222222222222%s,gpio%d=%d,%d,%d\n",__func__,num,gpiod->val,gpiod->id,gpiod->output);
		    gpio_direction_input(gpiod->id);
        }
		
		//gpio_set_value(gpiod->id,gpiod->val );
	}
	gpiod_ctrl->num = num;
	gpiod_ctrl->data = parse_data;

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

	gpiod_ctrl = devm_kzalloc(&pdev->dev, sizeof(*gpiod_ctrl), GFP_KERNEL);
	gpiod_ctrl_save = gpiod_ctrl;
	if (!gpiod_ctrl)
		return -ENOMEM;
	gpiod_ctrl->dev = &pdev->dev;
	gpiod_ctrl->cls_attr.attr.name = "info";
	gpiod_ctrl->cls_attr.attr.mode = S_IRUGO;
	gpiod_ctrl->cls_attr.show = gpio_ctrl_info_show;
	dev_set_name(gpiod_ctrl->dev, "gpio_ctrl");
	if (!pdev->dev.of_node)
		return -EINVAL;
	gpio_ctrl_parse_dt(gpiod_ctrl);
	input = devm_input_allocate_device(gpiod_ctrl->dev);
	if (!input) {
		error = -ENOMEM;
		return error;
	}
	input->name = "gpio-ctrl-keypad";//pdev->name;
	input->phys = "gpio-ctrl-keys/input4";
	input->dev.parent = gpiod_ctrl->dev;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	
	//__set_bit(EV_REP, input->evbit);
	//__set_bit(EV_KEY, input->evbit);
	input->evbit[0] = BIT_MASK(EV_KEY);

	for (j = 0; j < 27; j++) {
		input_set_capability(input, EV_KEY, codes[j].code);
	}
	
	
	error = input_register_device(input);
	if (error) {
		pr_err("gpio-keys: Unable to register input device, "
			"error: %d\n", error);
		return error;
	}
	gpio_ctrl_key_input = input;

	printk("%s",__func__);

	hrtimer_init(&gpio_ctrl_keys_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gpio_ctrl_keys_timer.function = gpio_ctrl_keys_poll;
	//hrtimer_start(&gpio_ctrl_keys_timer, ktime_set(3, 1000), HRTIMER_MODE_REL);
	return 0;
}

static int gpio_ctrl_remove(struct platform_device *dev)
{
	return 0;
}

static void gpio_ctrl_shutdown(struct platform_device *dev)
{
}

int test_ctrl_key = 0;
extern int  testpad12 ;
static int flag_enterWirelessTestMode = 0;
static int test_step = 0;
static int clkL = 0;   //clock  level status :high/low
static struct completion my_completion;
static unsigned int ch_id;
static unsigned int test_result_wireless = -1;

#define MAX_CHECK_KEYS_TIMES 30

static enum hrtimer_restart test_mode_poll(struct hrtimer *timer){
	
	unsigned int value = 0;	
	static int last_ch_id = 0;
	test_step++;

	
	if(test_step == 8) //clock6 set cmd id 0101
	{	
		gpio_direction_output(parse_data[P3_D3].id, 0);
		gpio_direction_output(parse_data[P3_D2].id, 1);
		gpio_direction_output(parse_data[P3_D1].id, 0);
		gpio_direction_output(parse_data[P3_D0].id, 1);
		//gpio_direction_output(parse_data[P3_SL].id,0);
	}
	//udelay(1000);
	//clock7 -----  set channel id data: ch_id
	if(test_step == 10) //clock7
	{
		gpio_direction_output(parse_data[P3_D3].id, (ch_id & (1u << 3)) ? 1 : 0);
		gpio_direction_output(parse_data[P3_D2].id, (ch_id & (1u << 2)) ? 1 : 0);
		gpio_direction_output(parse_data[P3_D1].id, (ch_id & (1u << 1)) ? 1 : 0);
		gpio_direction_output(parse_data[P3_D0].id, (ch_id & (1u << 0)) ? 1 : 0);
		//gpio_direction_output(parse_data[P3_SL].id, 1);
		last_ch_id = ch_id;
	}
	//udelay(1000);

	//clock 9  //if receive 0110 ,it is success
	//gpio_direction_output(parse_data[P3_SL].id, 1);
	//udelay(500);
	if(test_step == 15) //clock9.5
	{
		//set as input
		gpio_direction_input(parse_data[P3_D3].id);
		gpio_direction_input(parse_data[P3_D2].id);
		gpio_direction_input(parse_data[P3_D1].id);
		gpio_direction_input(parse_data[P3_D0].id);		
		
		value = gpio_get_value(parse_data[P3_D3].id) ? (1u << 3) : 0;
		value |= gpio_get_value(parse_data[P3_D2].id) ? (1u << 2) : 0;
		value |= gpio_get_value(parse_data[P3_D1].id) ? (1u << 1) : 0;
		value |= gpio_get_value(parse_data[P3_D0].id) ? (1u << 0) : 0;
		if((ch_id == 0) && (value != 5))  //for check cmd
		{
			printk("step 9: received error data: %u != 5\n", value);
			test_result_wireless = -1;
			//return;
		}
	}
	//udelay(500);
	//clock10 ---- get channel_id for 2.4G and check
	//gpio_direction_output(parse_data[P3_SL].id, 0);
	//udelay(500);
	if(test_step == 17) //clock10.5
	{
		value = gpio_get_value(parse_data[P3_D3].id) ? (1u << 3) : 0;
		value |= gpio_get_value(parse_data[P3_D2].id) ? (1u << 2) : 0;
		value |= gpio_get_value(parse_data[P3_D1].id) ? (1u << 1) : 0;
		value |= gpio_get_value(parse_data[P3_D0].id) ? (1u << 0) : 0;
		if(value != ch_id)
		{
			printk("step 10: received error data: %u != %u, %u\n", value, ch_id, last_ch_id);
			test_result_wireless = -1;
			//return;
		}
	}
	//udelay(500);
	

	
	if((test_step % 2) == 0)
	{
		clkL = !clkL;
		gpio_direction_output(parse_data[P3_SL].id, clkL);
	}	
	
	if(test_step <= 32  && test_result_wireless != -1)  // 16??2¡§D?
		hrtimer_start(timer, ktime_set(0, 500000), HRTIMER_MODE_REL);
	else
	{
		//test_result_wireless = 0;
		complete(&my_completion);
	}	
	return HRTIMER_NORESTART;
}

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
static enum hrtimer_restart gpio_ctrl_keys_poll(struct hrtimer *timer){
	int Dpad3_Val0;
	int Dpad3_Val1;
	int Dpad3_Val2;
	int Dpad3_Val3;
	int save, now;
	if(flag_enterWirelessTestMode) return test_mode_poll(timer);

	//for test
	//printk("<");
	test_ctrl_key = 1;
	if(testpad12) printk("find embbed condition in wireless(%d, %d, %s)\n", in_irq(),in_softirq(), current->comm);
	
	if(dpad3Init==0){
		//printk("%s1111111 %d, %d\n",__func__,in_irq(),in_softirq());
		//gpio_direction_output(parse_data[8].id,1);
		gpio_direction_output(parse_data[P3_SL].id,1);
		dpad3Init = 1;
		hrtimer_start(timer, ktime_set(3, 6000), HRTIMER_MODE_REL);
	}else if(dpad3Init==1){
		//printk("%s222222 %d\n",__func__,dpad3Init);
		gpio_direction_input(parse_data[P3_D0].id);
		gpio_direction_input(parse_data[P3_D1].id);
		gpio_direction_input(parse_data[P3_D2].id);
		gpio_direction_input(parse_data[P3_D3].id);
		//hrtimer_start(timer, ktime_set(1, 300000), HRTIMER_MODE_REL);
		dpad3Init=2;
	}else{
		//printk("(%d, %d)\n",in_irq(),in_softirq());
		//printk("%s33333 %d\n",__func__,dpad3Init);
		//hrtimer_start(timer, ktime_set(0, 500000), HRTIMER_MODE_REL);
	}
	if(timerCount==0){
		gpio_direction_output(parse_data[P3_SL].id,0);
	}
	if(timerCount==1){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_UP_].lastevent;
		now = codes[_UP_].event = !Dpad3_Val0;
		//if(now > 0)printk("upkey now=%d \n", now); 
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_UP_].initflag ==1)
			{ codes[_UP_].initflag = 0;}
			else
			{	
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_UP_].code, codes[_UP_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_UP_].lastevent = now;
		}
		save = codes[_DW_].lastevent;
		now = codes[_DW_].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_DW_].initflag ==1)
			{ codes[_DW_].initflag = 0;}
			else
			{	
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_DW_].code, codes[_DW_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_DW_].lastevent = now;
		}
		save = codes[_LF_].lastevent;
		now = codes[_LF_].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_LF_].initflag ==1)
			{ codes[_LF_].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_LF_].code, codes[_LF_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_LF_].lastevent = now;
		}
		save = codes[_RG_].lastevent;
		now = codes[_RG_].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_RG_].initflag ==1)
			{ codes[_RG_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_RG_].code, codes[_RG_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_RG_].lastevent = now;
		}
	}else if(timerCount==2){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_A_].lastevent;
		now = codes[_A_].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_A_].initflag ==1)
			{ codes[_A_].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_A_].code, codes[_A_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_A_].lastevent = now;
		}
		save = codes[_B_].lastevent;
		now = codes[_B_].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_B_].initflag ==1)
			{ codes[_B_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_B_].code, codes[_B_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_B_].lastevent = now;
		}
		save = codes[_C_].lastevent;
		now = codes[_C_].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_C_].initflag ==1)
			{ codes[_C_].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_C_].code, codes[_C_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_C_].lastevent = now;
		}
		save = codes[_X_].lastevent;
		now = codes[_X_].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_X_].initflag ==1)
			{ codes[_X_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_X_].code, codes[_X_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_X_].lastevent = now;
		}
		
	}else if(timerCount==3){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_Y_].lastevent;
		now = codes[_Y_].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_Y_].initflag ==1)
			{ codes[_Y_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_Y_].code, codes[_Y_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_Y_].lastevent = now;
		}
		save = codes[_ST_].lastevent;
		now = codes[_ST_].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_ST_].initflag ==1)
			{ codes[_ST_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_ST_].code, codes[_ST_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_ST_].lastevent = now;
		}
		save = codes[_MD_].lastevent;
		now = codes[_MD_].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_MD_].initflag ==1)
			{ codes[_MD_].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_MD_].code, codes[_MD_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_MD_].lastevent = now;
		}
		save = codes[_Z_].lastevent;
		now = codes[_Z_].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_Z_].initflag ==1)
			{ codes[_Z_].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_Z_].code, codes[_Z_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_Z_].lastevent = now;
		}
	}else if(timerCount==4){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_RS_].lastevent;
		now = codes[_RS_].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_RS_].initflag ==1)
			{ codes[_RS_].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_RS_].code, codes[_RS_].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_RS_].lastevent = now;
		}		
	}else if(timerCount==5){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_UP_+SHIFT].lastevent;
		now = codes[_UP_+SHIFT].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_UP_+SHIFT].initflag ==1)
			{ codes[_UP_+SHIFT].initflag = 0;}
			else
			{
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_UP_+SHIFT].code, codes[_UP_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_UP_+SHIFT].lastevent = now;
		}
		save = codes[_DW_+SHIFT].lastevent;
		now = codes[_DW_+SHIFT].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_DW_+SHIFT].initflag ==1)
			{ codes[_DW_+SHIFT].initflag = 0;}
			else
			{	
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_DW_+SHIFT].code, codes[_DW_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_DW_+SHIFT].lastevent = now;
		}
		save = codes[_LF_+SHIFT].lastevent;
		now = codes[_LF_+SHIFT].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_LF_+SHIFT].initflag ==1)
			{ codes[_LF_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_LF_+SHIFT].code, codes[_LF_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_LF_+SHIFT].lastevent = now;
		}
		save = codes[_RG_+SHIFT].lastevent;
		now = codes[_RG_+SHIFT].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_RG_+SHIFT].initflag ==1)
			{ codes[_RG_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_RG_+SHIFT].code, codes[_RG_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_RG_+SHIFT].lastevent = now;
		}
	}else if(timerCount==6){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_A_+SHIFT].lastevent;
		now = codes[_A_+SHIFT].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_A_+SHIFT].initflag ==1)
			{ codes[_A_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_A_+SHIFT].code, codes[_A_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_A_+SHIFT].lastevent = now;
		}
		save = codes[_B_+SHIFT].lastevent;
		now = codes[_B_+SHIFT].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_B_+SHIFT].initflag ==1)
			{ codes[_B_+SHIFT].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_B_+SHIFT].code, codes[_B_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_B_+SHIFT].lastevent = now;
		}
		save = codes[_C_+SHIFT].lastevent;
		now = codes[_C_+SHIFT].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_C_+SHIFT].initflag ==1)
			{ codes[_C_+SHIFT].initflag = 0;}
			else
			{
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_C_+SHIFT].code, codes[_C_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_C_+SHIFT].lastevent = now;
		}
		save = codes[_X_+SHIFT].lastevent;
		now = codes[_X_+SHIFT].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_X_+SHIFT].initflag ==1)
			{ codes[_X_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_X_+SHIFT].code, codes[_X_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_X_+SHIFT].lastevent = now;
		}
		
	}else if(timerCount==7){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_Y_+SHIFT].lastevent;
		now = codes[_Y_+SHIFT].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_Y_+SHIFT].initflag ==1)
			{ codes[_Y_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_Y_+SHIFT].code, codes[_Y_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_Y_+SHIFT].lastevent = now;
		}
		save = codes[_ST_+SHIFT].lastevent;
		now = codes[_ST_+SHIFT].event = !Dpad3_Val1;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_ST_+SHIFT].initflag ==1)
			{ codes[_ST_+SHIFT].initflag = 0;}
			else
			{			
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_ST_+SHIFT].code, codes[_ST_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_ST_+SHIFT].lastevent = now;
		}
		save = codes[_MD_+SHIFT].lastevent;
		now = codes[_MD_+SHIFT].event = !Dpad3_Val2;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_MD_+SHIFT].initflag ==1)
			{ codes[_MD_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_MD_+SHIFT].code, codes[_MD_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_MD_+SHIFT].lastevent = now;
		}
		save = codes[_Z_+SHIFT].lastevent;
		now = codes[_Z_+SHIFT].event = !Dpad3_Val3;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_Z_+SHIFT].initflag ==1)
			{ codes[_Z_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_Z_+SHIFT].code, codes[_Z_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_Z_+SHIFT].lastevent = now;
		}
	}else if(timerCount==8){
		Dpad3_Val0 = gpio_get_value(parse_data[P3_D0].id);
		Dpad3_Val1 = gpio_get_value(parse_data[P3_D1].id);
		Dpad3_Val2 = gpio_get_value(parse_data[P3_D2].id);
		Dpad3_Val3 = gpio_get_value(parse_data[P3_D3].id);
		save = codes[_RS_+SHIFT].lastevent;
		now = codes[_RS_+SHIFT].event = !Dpad3_Val0;
		if (now != save){
			//printk("%s, xkey now=%d \n",__func__,now);
			if(codes[_RS_+SHIFT].initflag ==1)
			{ codes[_RS_+SHIFT].initflag = 0;}
			else
			{				
			input_event(gpio_ctrl_key_input, EV_KEY, codes[_RS_+SHIFT].code, codes[_RS_+SHIFT].event);
			input_sync(gpio_ctrl_key_input);
			}
			codes[_RS_+SHIFT].lastevent = now;
		}
		
	}
	if((timerCount<10)&&(timerCount>=1)){
		// udelay(50);
		gpio_direction_output(parse_data[P3_SL].id,timerCount%2);
           //     udelay(10);
	}
	if(dpad3Init==2){
		hrtimer_start(timer, ktime_set(0, 400000 + 10 * 1000), HRTIMER_MODE_REL);
	}
	timerCount++;
	if(timerCount>36){
		timerCount=0;
	}
	//printk(">");
	test_ctrl_key = 0;

	return HRTIMER_NORESTART;
}


#if defined(CONFIG_OF)
static const struct of_device_id gpio_ctrl_keys_of_match[] = {
	{ .compatible = "gpio-ctrl-keys", },
	{},
};
#endif

static struct platform_driver gpio_ctrl_keys_driver = {
	.driver     = {
		.name   = "gpio-ctrl-keys",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(gpio_ctrl_keys_of_match),
	},
	.probe      = gpio_ctrl_probe,
	.remove     = gpio_ctrl_remove,
	.shutdown   = gpio_ctrl_shutdown,
};

static int __init gpio_ctrl_keys_init(void){
	return platform_driver_register(&gpio_ctrl_keys_driver);
}

module_init(gpio_ctrl_keys_init);

static void __exit gpio_ctrl_keys_exit(void)
{
	platform_driver_unregister(&gpio_ctrl_keys_driver);
}

static void initKeyTable(void)
{
	int i = 0;
	for(; i < sizeof(codes) / sizeof(codes[0]); i++)
	{
		codes[i].event = 0;
		codes[i].lastevent = 0;
		codes[i].initflag = 0;
	}
}

extern int gpio_bank_set_value(unsigned gpio,int value, unsigned int mask);
extern int gpio_bank_set_direction(unsigned gpio,int dir, unsigned int mask);

static void initGpioStatus(void)
{
#if 0	
	gpio_direction_output(parse_data[P3_POWER_UP].id,0);
	gpio_direction_output(parse_data[P3_SL].id,0);	
	gpio_direction_output(parse_data[P3_D0].id,0);	
	gpio_direction_output(parse_data[P3_D1].id,0);	
	gpio_direction_output(parse_data[P3_D2].id,0);	
	gpio_direction_output(parse_data[P3_D3].id,0);	
#else
	gpio_bank_set_direction(1, 1<<0, 1<< 0);//D3
	gpio_bank_set_value(1, 0, 1<< 0);
	gpio_bank_set_direction(1, 1<< 1, 1<< 1);//D2
	gpio_bank_set_value(1, 0, 1<< 1);
	gpio_bank_set_direction(1, 1<< 2, 1<< 2);//D1
	gpio_bank_set_value(1, 0, 1<< 2);
	gpio_bank_set_direction(1, 1<< 3, 1<< 3);//D0
	gpio_bank_set_value(1, 0, 1<< 3);
	gpio_bank_set_direction(1, 1<< 4, 1<< 4);//CLK
	gpio_bank_set_value(1, 0, 1<< 4);
	gpio_bank_set_direction(1, 1<< 5, 1<< 5);//power control
	gpio_bank_set_value(1, 0, 1<< 5);

#endif	
}

/* add begin and exit interface */
static int wirelessStatus = 0;  //0 ---- close, 1---- open
void startWirelessScankey(){
	
	printk("startWirelessScankey begin %p..***_\n", gpiod_ctrl_save);
	if(wirelessStatus)
	{	 
		printk("WirelessScankey has begun, return directly\n");
		return;
	}
	wirelessStatus = 1;
	dpad3Init = 0;
	if(gpiod_ctrl_save != NULL)initGpioStatus();//gpio_ctrl_parse_dt(gpiod_ctrl_save);
	printk("startWirelessScankey line:%d..\n", __LINE__);
	{ //add according WeiGong of 2.4G modules
		msleep(145);
		gpio_direction_output(parse_data[P3_POWER_UP].id,1);
		msleep(100);
	}	
	initKeyTable();
	hrtimer_start(&gpio_ctrl_keys_timer, ktime_set(3, 1000), HRTIMER_MODE_REL);;
	printk("startWirelessScankey line:%d..\n", __LINE__);
}

void stopWirelessScankey(){
	if(wirelessStatus == 0)
	{	 
		printk("WirelessScankey has stop, return directly\n");
		return;
	}	
	hrtimer_cancel(&gpio_ctrl_keys_timer);
	gpio_direction_output(parse_data[P3_POWER_UP].id,0);
	if(gpiod_ctrl_save != NULL)initGpioStatus(); //gpio_ctrl_parse_dt(gpiod_ctrl_save);
	msleep(155);
	wirelessStatus = 0;
}

int enterWirelessTestMode(unsigned int channel_id)
{
	//unsigned int clkL = 0;
	if(flag_enterWirelessTestMode) return 0;  //avoid re-enter
	printk("enterWirelessTestMode: %u......\n", ch_id);
	ch_id = channel_id & 0xf;
	
	flag_enterWirelessTestMode = 1;
	test_step = 0;
	test_result_wireless = 0;

#if 1
	if(wirelessStatus)
	{
		stopWirelessScankey();
		//msleep(50);
	}
	//open 2.4G
	wirelessStatus = 1;
	dpad3Init = 0;
	if(gpiod_ctrl_save != NULL)initGpioStatus(); //gpio_ctrl_parse_dt(gpiod_ctrl_save);
	printk("startWirelessScankey line:%d..\n", __LINE__);	
	msleep(100+100);  // add time 
	//clock1
	gpio_direction_output(parse_data[P3_POWER_UP].id,1);
	gpio_direction_output(parse_data[P3_SL].id,1);
	gpio_direction_input(parse_data[P3_D0].id);
	gpio_direction_input(parse_data[P3_D1].id);
	gpio_direction_input(parse_data[P3_D2].id);
	gpio_direction_input(parse_data[P3_D3].id);
printk("startWirelessScankey sleep before: \n");
	msleep(60);  //range (50 ~ 120)ms
printk("startWirelessScankey sleep after: \n");
#endif

	init_completion(&my_completion);
	clkL = 0;
	gpio_direction_output(parse_data[P3_SL].id,0);
	hrtimer_start(&gpio_ctrl_keys_timer, ktime_set(0, 500000), HRTIMER_MODE_REL);

	//wait the test task exit:
	wait_for_completion_timeout(&my_completion, msecs_to_jiffies(MAX_CHECK_KEYS_TIMES));
	
	printk("WirelessTestMode end\n");

#if 1
	//enter key scan mode 
	initKeyTable();
	hrtimer_start(&gpio_ctrl_keys_timer, ktime_set(3, 1000), HRTIMER_MODE_REL);;
	flag_enterWirelessTestMode = 0;
	printk("startWirelessScankey line:%d(ret=%d)..\n", __LINE__, test_result_wireless);	
#endif
	
	return test_result_wireless;
	
}
/*****************************/

EXPORT_SYMBOL(startWirelessScankey);
EXPORT_SYMBOL(stopWirelessScankey);
EXPORT_SYMBOL(enterWirelessTestMode);
/**  **/

module_exit(gpio_ctrl_keys_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio-detection");
MODULE_AUTHOR("ROCKCHIP");
