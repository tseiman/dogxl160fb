/*
 * Author: Thomas 'tseiman' Schmidt <t (dot) schmidt (at) md-network (dot) de>
 * 
 * This file is part of the DOGXL160 project a framebuffer
 * driver for the EA DOGXL160 4 grey tone LCD display
 *
 * This file is header for the sysfs interface from dogxl160fb module
 *
 * this project should come with the GNU Public License
 * in hope that it is somehow usefull and an example how
 * (not) to do things
 * 
 * Mon 24 Jun 2013 01:23:21 AM CEST
 */


#include <linux/device.h>


typedef struct dogxl160fb_configuration{
	int		redCorrect;
	int		greenCorrect;
	int		blueCorrect;
	int		sumCorrect;
}  dogxl160fb_configuration;

/*
dogxl160fb_configuration dogxl160fb_driver_configuration={
	.redCorrect = 3,
	.greenCorrect = 4,
	.blueCorrect = 2

};
*/

int dogxl160fb_setupSysFS(struct device *dev,struct dogxl160fb_configuration *drv_conf);

int dogxl160fb_destroySysFS(struct device *dev);



