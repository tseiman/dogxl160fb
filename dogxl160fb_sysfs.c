/*
 * Author: Thomas 'tseiman' Schmidt <t (dot) schmidt (at) md-network (dot) de>
 * 
 * This file is part of the DOGXL160 project a framebuffer
 * driver for the EA DOGXL160 4 grey tone LCD display
 *
 * This file contains the sysfs interface to pass some settings 
 * like RGB correction factors to the module
 *
 * this project should come with the GNU Public License
 * in hope that it is somehow usefull and an example how
 * (not) to do things
 * 
 * Mon 24 Jun 2013 01:16:13 AM CEST
 */


#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>

#include "dogxl160fb.h"
#include "dogxl160fb_sysfs.h"

static dogxl160fb_configuration *dogxl160fb_driver_configuration;

static ssize_t show_correctRed(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", dogxl160fb_driver_configuration->redCorrect);
}
static ssize_t set_correctRed(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {

    int tmp = 0; 
    sscanf(buf,"%d",  &tmp);

    if(tmp < 256 || tmp > -1) {
	if( tmp != dogxl160fb_driver_configuration->redCorrect)
	    dogxl160fb_driver_configuration->redCorrect = tmp;
    } else {
	pr_err("Invalid value for redCorrect %s ", buf );
    }

    return strlen(buf);

}
static DEVICE_ATTR(correctRed, 0666, show_correctRed, set_correctRed);


static ssize_t show_correctGreen(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", dogxl160fb_driver_configuration->greenCorrect);
}

static ssize_t set_correctGreen(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {

    int tmp = 0; 
    sscanf(buf,"%d",  &tmp);

    if(tmp < 256 || tmp > -1) {
	if( tmp != dogxl160fb_driver_configuration->greenCorrect)
	    dogxl160fb_driver_configuration->greenCorrect = tmp;
    } else {
	pr_err("Invalid value for greenCorrect %s ", buf );
    }

    return strlen(buf);

}
static DEVICE_ATTR(correctGreen, 0666, show_correctGreen, set_correctGreen);


static ssize_t show_correctBlue(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", dogxl160fb_driver_configuration->blueCorrect);
}
static ssize_t set_correctBlue(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {

    int tmp = 0; 
    sscanf(buf,"%d",  &tmp);

    if(tmp < 256 || tmp > -1) {
	if( tmp != dogxl160fb_driver_configuration->blueCorrect)
	    dogxl160fb_driver_configuration->blueCorrect = tmp;
    } else {
	pr_err("Invalid value for blueCorrect %s ", buf );
    }

    return strlen(buf);

}
static DEVICE_ATTR(correctBlue, 0666, show_correctBlue, set_correctBlue);



static ssize_t show_correctSum(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", dogxl160fb_driver_configuration->sumCorrect);
}
static ssize_t set_correctSum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {

    int tmp = 0; 
    sscanf(buf,"%d",  &tmp);

    if(tmp < 256 || tmp > 0) {
	if( tmp != dogxl160fb_driver_configuration->sumCorrect)
	    dogxl160fb_driver_configuration->sumCorrect = tmp;
    } else {
	pr_err("Invalid value for sumCorrect %s ", buf );
    }

    return strlen(buf);

}
static DEVICE_ATTR(correctSum, 0666, show_correctSum, set_correctSum);




int dogxl160fb_setupSysFS(struct device *dev, struct dogxl160fb_configuration *drv_conf) {

    printk(KERN_INFO "%s: adding sysfs entries\n", DRVNAME); 

    dogxl160fb_driver_configuration = drv_conf;
    device_create_file(dev,&dev_attr_correctRed);
    device_create_file(dev,&dev_attr_correctGreen);
    device_create_file(dev,&dev_attr_correctBlue);
    device_create_file(dev,&dev_attr_correctSum);

    return 0;
}



int dogxl160fb_destroySysFS(struct device *dev) {

    printk(KERN_INFO "%s: removing sysfs entries\n", DRVNAME); 

    device_remove_file(dev,&dev_attr_correctRed);
    device_remove_file(dev,&dev_attr_correctGreen);
    device_remove_file(dev,&dev_attr_correctBlue);
    device_remove_file(dev,&dev_attr_correctSum);

    return 0;
}

