/*
 * Author: Thomas 'tseiman' Schmidt <t (dot) schmidt (at) md-network (dot) de>
 * 
 * This file is part of the DOGXL160 project a framebuffer
 * driver for the EA DOGXL160 4 grey tone LCD display
 *
 * This file contains some defines for the dogxl160fb module
 *
 * this project should come with the GNU Public License
 * in hope that it is somehow usefull and an example how
 * (not) to do things
 * 
 * Mon 24 Jun 2013 01:16:13 AM CEST
 */

#ifndef dogxl160fb_configuration
#include "dogxl160fb_sysfs.h"
#endif


#define SPI_BUS_SPEED 8000000
#define SPI_BUS 0
#define SPI_BUS_CS1 0

#define GPIO_RST 15
#define GPIO_DC  14

#define BUFFER_SIZE 832



#define WIDTH		160
#define HEIGHT		104
#define BPP_INPUT		16
#define BPP_OUTPUT		2

#define DOGXL160_DISPLAY		0	/* DOGXL160 Dot Matrix LCD */


#define DOGXL160_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


struct dogxl160fb_par {
	struct spi_device *spi;
	struct fb_info *info;
	u8 *ssbuf;
	int rst;
	int dc;
};


struct dogxl160fb_platform_data {
	int rst_gpio;
	int dc_gpio;
/*	struct dogxl160fb_configuration *driver_configuration; */
};
 
