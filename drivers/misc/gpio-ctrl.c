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


struct gpio_data {
	struct gpio_ctrl *parent;
	int id;
	int val;
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

static struct class *gpio_ctrl_class;

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
	int num = of_get_child_count(gpiod_ctrl->dev->of_node);

	if (num == 0)
		return -ENODEV;
	data = devm_kzalloc(gpiod_ctrl->dev, num * sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	for_each_child_of_node(gpiod_ctrl->dev->of_node, node) {
		enum of_gpio_flags flags;

		gpiod = &data[i++];
		gpiod->parent = gpiod_ctrl;
		gpiod->id = of_get_gpio_flags(node, 0, &flags);
		if (!gpio_is_valid(gpiod->id)) {
			dev_err(gpiod_ctrl->dev, "failed to get gpio\n");
			num--;
			i--;
			continue;
		}
		gpiod->val = !(flags & OF_GPIO_ACTIVE_LOW);
		//dev_err(gpiod_ctrl->dev, "flags=%d,gpio_id=%d,gpio_val=%d\n",flags,gpiod->id,gpiod->val);
		gpiod->name = of_get_property(node, "label", NULL);
		devm_gpio_request(gpiod_ctrl->dev,gpiod->id,gpiod->name);
		ret =gpio_direction_output(gpiod->id,gpiod->val );
		if(ret < 0){
			dev_err(gpiod_ctrl->dev, "gpio_id=%d,ret=%d\n",gpiod->id,ret);
		}
		//gpio_set_value(gpiod->id,gpiod->val );
	}
	gpiod_ctrl->num = num;
	gpiod_ctrl->data = data;

	return 0;
}

static int gpio_ctrl_probe(struct platform_device *pdev)
{
	struct gpio_ctrl *gpiod_ctrl;
	struct gpio_data *gpiod;
	int val;
	int i;
	int ret;

	gpiod_ctrl = devm_kzalloc(&pdev->dev, sizeof(*gpiod_ctrl), GFP_KERNEL);
	if (!gpiod_ctrl)
		return -ENOMEM;
	gpiod_ctrl->dev = &pdev->dev;
	gpiod_ctrl->cls_attr.attr.name = "info";
	gpiod_ctrl->cls_attr.attr.mode = S_IRUGO;
	gpiod_ctrl->cls_attr.show = gpio_ctrl_info_show;
	dev_set_name(gpiod_ctrl->dev, "gpio_ctrl");
	if (!pdev->dev.of_node)
		return -EINVAL;
	val = readl_relaxed(RK_GRF_VIRT + RK3036_GRF_SOC_CON0);
	val &=~0x800;
	writel_relaxed(val|0x08000000, RK_GRF_VIRT + RK3036_GRF_SOC_CON0);
	dev_err(gpiod_ctrl->dev, "grf_value = 0x%x\n",val);
	gpio_ctrl_parse_dt(gpiod_ctrl);
	wake_lock_init(&gpiod_ctrl->wake_lock, WAKE_LOCK_SUSPEND, "gpio_ctrl");
	for (i = 0; i < gpiod_ctrl->num; i++) {
		gpiod = &gpiod_ctrl->data[i];
		//gpio_set_value(gpiod->id,gpiod->val );
		int val=gpio_get_value(gpiod->id);
		//dev_err(gpiod_ctrl->dev, "gpio_id1111111=%d,gpio_val=%d\n",gpiod->id,val);
		ret = gpio_ctrl_class_register(gpiod_ctrl, gpiod);
		if (ret < 0)
			return ret;
	}
	class_create_file(gpio_ctrl_class, &gpiod_ctrl->cls_attr);

	dev_info(gpiod_ctrl->dev, "gpio detection driver probe success\n");

	return 0;
}

static int gpio_ctrl_remove(struct platform_device *dev)
{
	return 0;
}

static void gpio_ctrl_shutdown(struct platform_device *dev)
{
}

#if defined(CONFIG_OF)
static const struct of_device_id gpio_ctrl_of_match[] = {
	{ .compatible = "gpio-ctrl", },
	{},
};
#endif

static struct platform_driver gpio_ctrl_driver = {
	.driver     = {
		.name   = "gpio-ctrl",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(gpio_ctrl_of_match),
	},
	.probe      = gpio_ctrl_probe,
	.remove     = gpio_ctrl_remove,
	.shutdown   = gpio_ctrl_shutdown,
};

static int __init gpio_ctrl_init(void)
{
	if (!gpio_ctrl_class_init())
		return platform_driver_register(&gpio_ctrl_driver);
	else
		return -1;
}

fs_initcall_sync(gpio_ctrl_init);

static void __exit gpio_ctrl_exit(void)
{
	platform_driver_unregister(&gpio_ctrl_driver);
}

module_exit(gpio_ctrl_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio-detection");
MODULE_AUTHOR("ROCKCHIP");
