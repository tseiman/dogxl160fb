/*
 * Author: Thomas 'tseiman' Schmidt <t (dot) schmidt (at) md-network (dot) de>
 * 
 * This file is part of the DOGXL160 project a framebuffer
 * driver for the EA DOGXL160 4 grey tone LCD display
 *
 * This file contains more or less the complete 
 * framebuffer and SPI handling (bit more seperation would be nice... )
 *
 * this project should come with the GNU Public License
 * in hope that it is somehow usefull and an example how
 * (not) to do things
 * 
 * Mon 24 Jun 2013 01:16:13 AM CEST
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "dogxl160fb.h"
#include "dogxl160fb_main.h"

#define DEF_WORK_DELAY HZ/16


static struct fb_fix_screeninfo dogxl160fb_fix __devinitdata = {
	.id 		= "DOGXL160", 
	.type 		= FB_TYPE_PACKED_PIXELS,
	.visual 	= FB_VISUAL_TRUECOLOR,
	.xpanstep 	= 0,
	.ypanstep 	= 0,
	.ywrapstep 	= 0, 
	.line_length 	= WIDTH*BPP_INPUT/8,
	.accel 		= FB_ACCEL_NONE,
};



static struct fb_var_screeninfo dogxl160fb_var __devinitdata = {
	.xres 		= WIDTH,
	.yres 		= HEIGHT,
	.xres_virtual 	= WIDTH,
	.yres_virtual 	= HEIGHT,
	.height         = -1,
	.width          = -1,
	.vmode          = FB_VMODE_NONINTERLACED,
	.bits_per_pixel = BPP_INPUT,
	.red            = { 11, 5, 0 },
	.green          = { 5, 6, 0 },
	.blue           = { 0, 5, 0 },
	.nonstd		= 0,
};


static struct dogxl160fb_platform_data dogxl160fb_data = {
       .rst_gpio       = GPIO_RST,
       .dc_gpio        = GPIO_DC,
};

static dogxl160fb_configuration dogxl160fb_driver_configuration={
	.redCorrect    = CORRECT_RED_STARTVALUE,
	.greenCorrect  = CORRECT_GREEN_STARTVALUE,
	.blueCorrect   = CORRECT_BLUE_STARTVALUE,
	.sumCorrect    = CORRECT_SUM_STARTVALUE
};




/* static unsigned char dogxl160fb_bla = 0; */


static void dogxl160fb_reset(struct dogxl160fb_par *par) { 

	/* Reset controller */
	printk(KERN_INFO "%s: resetting display controller with GPIO%d\n",DRVNAME, par->rst);

	gpio_set_value(par->rst, 0);
	udelay(10);
	gpio_set_value(par->rst, 1);
	mdelay(120); 

}




static void dogxl160fb_write_data(struct dogxl160fb_par *par, void *txbuf, size_t size)
{
    u8 * buf =  txbuf;

    if (!par->spi) {
	dev_err(par->info->device, "%s: (%s) par->spi is unexpectedly NULL\n", DRVNAME, __func__);
	return;
    }

    /* Set data mode */
    gpio_set_value(par->dc, 1);
    spi_write(par->spi,  buf, size);    


}

static void dogxl160fb_write_cmd(struct dogxl160fb_par *par, void *txbuf, size_t size) {
	int ret = 0;
/*    printk(KERN_INFO "%s: dogxl160fb_write_cmd()\n", DRVNAME);  */

	if (!par->spi) {
		dev_err(par->info->device, "%s: (%s) par->spi is unexpectedly NULL\n", DRVNAME, __func__);
		return;
	}


	/* Set command mode */
	gpio_set_value(par->dc, 0);

/*	ret = spi_write(par->, data); */
	if ((ret = spi_write(par->spi, txbuf, size)) < 0) 
	    pr_err("%s: write command failed with status %d\n", par->info->fix.id, ret); 

}


static void dogxl160fb_update_display(struct dogxl160fb_par *par)
{
	u8 set_cursor_sequence[] = { 0x00, 0x10, 0x60, };
	u8 *vmem = par->info->screen_base;

	u8 *dogxl160fb_data_buffer_in = (u8 *) par->ssbuf;


	int output_buffer_index = 0;

	int   row, col, line_preread_index; 
	u16 byte_buffer_packed[4];
	u16 byte_buffer_rgb;






    dogxl160fb_write_cmd(par, &set_cursor_sequence[0], 3);
    
    for(row = 0; row <= (HEIGHT - 4); row = row + 4) {
	for(col = 0; col < WIDTH; col = col + 1) {

	    for(line_preread_index = 0; line_preread_index < 4; ++line_preread_index) {
		byte_buffer_packed[line_preread_index] = 0;

		byte_buffer_rgb = 0;
		byte_buffer_rgb = (vmem[(((row + line_preread_index) * WIDTH ) + col) * (BPP_INPUT / 8)] << 8) | vmem[(((row + line_preread_index) * WIDTH ) + col) * (BPP_INPUT / 8) + 1];

/*		    (red * 77 + green * 151 + blue * 28) >> 8; */

/*
		byte_buffer_packed[line_preread_index] =  ((((byte_buffer_rgb >> 11) & 0x1f) *77
							 +  ((byte_buffer_rgb >> 5)  & 0x3f) *151
							 +  ((byte_buffer_rgb)       & 0x1f) *28 ) >> 6 )& 0x03;
*/
		byte_buffer_packed[line_preread_index] =   (((((int) ((byte_buffer_rgb >> 11) & 0x1f) * dogxl160fb_driver_configuration.redCorrect)
							    + ((int) ((byte_buffer_rgb >> 5)  & 0x3f) * dogxl160fb_driver_configuration.greenCorrect)
							    + ((int) ((byte_buffer_rgb)       & 0x1f) * dogxl160fb_driver_configuration.blueCorrect) ) / dogxl160fb_driver_configuration.sumCorrect)  >> 6 )& 0x03;


/* #ifdef __LITTLE_ENDIAN 
		 we have to swap bytes ... 
		byte_buffer_packed[line_preread_index] = ((byte_buffer_packed[line_preread_index] & 0x2) >> 1) | ((byte_buffer_packed[line_preread_index] & 0x1) << 1);
 #endif */
		
	    } 

	    dogxl160fb_data_buffer_in[output_buffer_index] =   (byte_buffer_packed[3] << 6) | (byte_buffer_packed[2] << 4) | (byte_buffer_packed[1] << 2) |  (byte_buffer_packed[0]);

	    ++output_buffer_index;

	}
    } 


    dogxl160fb_write_data(par, dogxl160fb_data_buffer_in, WIDTH*HEIGHT*BPP_OUTPUT/8);


}


static void dogxl160fb_deferred_io(struct fb_info *info, struct list_head *pagelist)
{
/*
    printk(KERN_INFO "%s: dogxl160fb_deferred_io()\n", DRVNAME); 
*/
	dogxl160fb_update_display(info->par); 
	
/*    schedule_delayed_work(&info->deferred_work, 0); */

}


static int dogxl160fb_init_display(struct dogxl160fb_par *par)
{

    u8 init_sequence[] = { 
	0xF1, 0x67, 0xC0, 0x40, 0x50, 0x2B,
	0x2F, 0xEB, 0x81, 0x5F, 0x89, 0xAF,
    }; 
    u8 *dogxl160fb_data_buffer;

    dogxl160fb_data_buffer = kmalloc(WIDTH*HEIGHT*BPP_OUTPUT/8, GFP_KERNEL); 
/*    dogxl160fb_data_buffer = vzalloc(PAGE_SIZE); */
    if (!dogxl160fb_data_buffer) {
	pr_err("%s: (%s) could not allocate memory\n", DRVNAME, __func__);
	return -ENOMEM;;
    }






    memset(dogxl160fb_data_buffer, 0xff,  WIDTH*HEIGHT*BPP_OUTPUT/8);


    printk(KERN_INFO "%s: initialize display\n",DRVNAME);
    printk(KERN_INFO "%s: size of array white_sequence is %d \n",DRVNAME, BUFFER_SIZE); 

	/* TODO: Need some error checking on gpios */

        /* Request GPIOs and initialize to default values */
    gpio_request_one(par->rst, GPIOF_OUT_INIT_HIGH, "DOGXL160 Reset Pin");
    gpio_request_one(par->dc, GPIOF_OUT_INIT_LOW, "DOGXL160 Data/Command Pin");

    dogxl160fb_reset(par);

    dogxl160fb_write_cmd(par, &init_sequence[0], 12);

/*    for(i = WIDTH*HEIGHT*BPP_OUTPUT/8; i > 0; i = i - BUFFER_SIZE) { */
    dogxl160fb_write_data(par, dogxl160fb_data_buffer, WIDTH*HEIGHT*BPP_OUTPUT/8); 
/*    }
    if(i > 0) dogxl160fb_write_data(par, dogxl160fb_data_buffer,  (WIDTH*HEIGHT*BPP_OUTPUT/8) - i);
*/
    kfree(dogxl160fb_data_buffer);
/*
??????
    for(i = WIDTH*HEIGHT*BPP/8; i > 0; i = i - ARRAY_SIZE(dogxl160fb_data_buffer)) {
	dogxl160fb_write_data(par, &dogxl160fb_data_buffer[0],  ARRAY_SIZE(dogxl160fb_data_buffer)); 
    }
    if(i > 0) dogxl160fb_write_data(par, &dogxl160fb_data_buffer[0],  (WIDTH*HEIGHT*BPP/8) - i);

    for(row = 0; row <= (HEIGHT - 4); row = row + 4) {
	for(col = 0; col <= WIDTH; col = col + 1) {
	    byte_buffer[0] = 
	}
    }
*/



/*	return spi_write(par->spi, txbuf, size); */

    return 0; 
}


void dogxl160fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{

/*	struct dogxl160fb_par *par = info->par; */
    printk(KERN_INFO "%s: dogxl160fb_fillrect()\n", DRVNAME); 

	sys_fillrect(info, rect);


/*	dogxl160fb_update_display(par); */
    schedule_delayed_work(&info->deferred_work, DEF_WORK_DELAY);

}


void dogxl160fb_copyarea(struct fb_info *info, const struct fb_copyarea *area) 
{

/*	struct dogxl160fb_par *par = info->par; */
    printk(KERN_INFO "%s: dogxl160fb_copyarea()\n", DRVNAME); 

	sys_copyarea(info, area);

/*	dogxl160fb_update_display(par);  */
    schedule_delayed_work(&info->deferred_work, DEF_WORK_DELAY);

}


void dogxl160fb_imageblit(struct fb_info *info, const struct fb_image *image) 
{

/*    struct dogxl160fb_par *par = info->par; */

    printk(KERN_INFO "%s: dogxl160fb_imageblit()\n", DRVNAME); 

    sys_imageblit(info, image);

/*    dogxl160fb_update_display(par); */
    schedule_delayed_work(&info->deferred_work, DEF_WORK_DELAY);

}


static ssize_t dogxl160fb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
{
/*	struct dogxl160fb_par *par = info->par; */
	unsigned long p = *ppos;
	void *dst;  
	int err = 0;
	unsigned long total_size; 

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		err = -EFAULT; 

/*	memcpy(dst,buf,count); */

	if  (!err)
		*ppos += count;

/*	dogxl160fb_update_display(par); */
    schedule_delayed_work(&info->deferred_work, DEF_WORK_DELAY);

	return (err) ? err : count;


}




static struct fb_deferred_io dogxl160fb_defio = {
	.delay		= HZ/30,
	.deferred_io	= dogxl160fb_deferred_io,
};

static struct fb_ops dogxl160fb_ops = {
	.owner		= THIS_MODULE,
	.fb_read	= fb_sys_read, 
	.fb_write	= dogxl160fb_write,
	.fb_fillrect	= dogxl160fb_fillrect,
	.fb_copyarea	= dogxl160fb_copyarea,
	.fb_imageblit	= dogxl160fb_imageblit,
};





static int __devinit dogxl160fb_probe (struct spi_device *spi)
{
	int chip = spi_get_device_id(spi)->driver_data; 
	struct dogxl160fb_platform_data *pdata = spi->dev.platform_data; 
	int vmem_size_in = WIDTH*HEIGHT*BPP_INPUT/8;  
	int vmem_size_out = WIDTH*HEIGHT*BPP_OUTPUT/8;  
	u8 *vmem; 
	struct fb_info *info;  
	struct dogxl160fb_par *par;
/*	struct dogxl160fb_configuration *driver_configuration; */
	int retval = -ENOMEM;


	printk(KERN_INFO "%s: enter dogxl160fb_probe (struct spi_device *spi) \n",DRVNAME );

	if (chip != DOGXL160_DISPLAY) {
		pr_err("%s: only the %s device is supported\n", DRVNAME, to_spi_driver(spi->dev.driver)->id_table->name);
		return -EINVAL;
	}

	if (!pdata) {
		pr_err("%s: platform data required for rst and dc info\n", DRVNAME);
		return -EINVAL;
	}



	printk(KERN_INFO "%s: valloc vmem_size=%d \n",DRVNAME , vmem_size_in);
	vmem = vzalloc(vmem_size_in);  
	if (!vmem) return retval; 

	info = framebuffer_alloc(sizeof(struct dogxl160fb_par), &spi->dev);
	if (!info) goto fballoc_fail;

	info->screen_base = (u8 __force __iomem *)vmem;
	info->fbops = &dogxl160fb_ops;
	info->fix = dogxl160fb_fix;
	info->fix.smem_len = vmem_size_in;
	info->var = dogxl160fb_var;

	/* Choose any packed pixel format as long as it's RGB565 */
	info->var.red.offset = 11;
	info->var.red.length = 5;
	info->var.green.offset = 5;
	info->var.green.length = 6;
	info->var.blue.offset = 0;
	info->var.blue.length = 5;
	info->var.transp.offset = 0;
	info->var.transp.length = 0; 
	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB; 
	info->fbdefio = &dogxl160fb_defio;
	printk(KERN_INFO "%s:fb_deferred_io_init(info); \n", DRVNAME );
	fb_deferred_io_init(info);

	printk(KERN_INFO "%s: par = info->par;\n", DRVNAME);
	par = info->par;
	par->info = info;
	par->spi = spi;
	printk(KERN_INFO "%s: pdata->rst_gpio, pdata=%p\n", DRVNAME, pdata);
	par->rst =  15;                                     /* ARRRGGG WORKAROUND ! pdata->rst_gpio; if driver is second time loaded */ 
	printk(KERN_INFO "%s: pdata->dc_gpio\n", DRVNAME ); /* this reference created in device_to_bus is gone since it is not initialized again */ 
	par->dc =  14;                                      /* FIXME ! pdata->dc_gpio;  */

/*	driver_configuration = kmalloc(sizeof(struct dogxl160fb_configuration),GFP_KERNEL);  
	if (!driver_configuration) goto configalloc_fail;  */
/*	pdata->driver_configuration = driver_configuration; */

	

	vmem = kmalloc(vmem_size_out,GFP_KERNEL); 
	if (!vmem) goto ssbufalloc_fail;
	par->ssbuf = vmem;

	retval = register_framebuffer(info);
	if (retval < 0) goto fbreg_fail;

	spi_set_drvdata(spi, info);

	retval = dogxl160fb_init_display(par);
	if (retval < 0) goto init_fail;
	printk(KERN_INFO"%s: fb%d: %s frame buffer device, using %d KiB of video memory\n",DRVNAME, info->node, info->fix.id, vmem_size_in);


	dogxl160fb_setupSysFS(info->dev,&dogxl160fb_driver_configuration);




	printk(KERN_INFO "%s: spi probe end\n", DRVNAME );


	return 0;

	/* TODO: release gpios on fail */
init_fail: 
	printk(KERN_ALERT "%s: probe init_fail\n", DRVNAME);
	spi_set_drvdata(spi, NULL); 
fbreg_fail: 
	printk(KERN_ALERT "%s: probe fbreg_fail\n", DRVNAME);
	framebuffer_release(info);  

ssbufalloc_fail: 
	printk(KERN_ALERT "%s: probe ssbuf alloc fail\n", DRVNAME);
	kfree(par->ssbuf); 
/*
configalloc_fail: 
	printk(KERN_ALERT "%s: probe config  alloc fail\n", DRVNAME);
	kfree(driver_configuration); */
fballoc_fail: 
	printk(KERN_ALERT "%s: probe fballoc_fail\n", DRVNAME);
	vfree(vmem);   /* FIXME - need to check whic memory needs to be freed */
	return retval; 
}

static int __devexit dogxl160fb_remove(struct spi_device *spi)
{
	struct fb_info *info = spi_get_drvdata(spi); 
	struct dogxl160fb_par *par;
/*	struct dogxl160fb_platform_data *pdata = spi->dev.platform_data;  */

	printk(KERN_INFO "%s: freeing resources\n", DRVNAME);


	dogxl160fb_destroySysFS(info->dev);
	
/*	if(pdata) {
	    kfree(pdata->driver_configuration);
	} */
	if (info) {
	    par = info->par;

	    if(par) {
		vfree(par->ssbuf);

		gpio_free(par->rst);
		gpio_free(par->dc);
	    }

/*	    fb_deferred_io_cleanup(info);  */
	    unregister_framebuffer(info); 

	    framebuffer_release(info); 
/*	    vfree(info->screen_base);  is this freed */

	}

	if(spi) {
	    spi_set_drvdata(spi, NULL);
	}

	return 0;
}


static int __init add_dogxl160fb_device_to_bus(void)
{
    struct spi_master *spi_master;
    struct spi_device *spi_device;
    struct device *pdev;
    char buff[64];
    int status = 0;
    
    printk(KERN_INFO "%s: adding device to bus\n", DRVNAME); 
 
    spi_master = spi_busnum_to_master(SPI_BUS);
    if (!spi_master) {
	printk(KERN_ALERT "%s: spi_busnum_to_master(%d) returned NULL\n",
	    DRVNAME, SPI_BUS);

/*	printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n"); */
	return -1;
    }
 
    spi_device = spi_alloc_device(spi_master);
    if (!spi_device) {
	put_device(&spi_master->dev);
	printk(KERN_ALERT "%s: spi_alloc_device() failed\n", DRVNAME);
	return -1;
    }
 
    /* specify a chip select line */
    spi_device->chip_select = SPI_BUS_CS1;
 
    /* Check whether this SPI bus.cs is already claimed */
    snprintf(buff, sizeof(buff), "%s.%u", 
	    dev_name(&spi_device->master->dev),
	    spi_device->chip_select);

    printk(KERN_INFO "%s: buff=%s\n", DRVNAME, buff );
 
    pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);


	printk(KERN_INFO "%s: ACTUAL PDATA reference=%p\n", DRVNAME, &dogxl160fb_data );


    if (pdev) {
	printk(KERN_INFO "%s: device already configured, reusing it (carefull it might be changed during devleopment)\n", DRVNAME );

	/* We are not going to use this spi_device, so free it */ 
	spi_dev_put(spi_device); 
	
	/* 
	 * There is already a device configured for this bus.cs combination.
	 * It's okay if it's us. This happens if we previously loaded then 
         * unloaded our driver. 
         * If it is not us, we complain and fail.
	 */
	if (pdev->driver && pdev->driver->name && 
		strcmp(DRVNAME, pdev->driver->name)) {
	    printk(KERN_ALERT "%s: Driver [%s] already registered for %s\n", DRVNAME, pdev->driver->name, buff);
	    status = -1;
	} 
    }  else { 
	spi_device->max_speed_hz = SPI_BUS_SPEED;
	spi_device->mode = SPI_MODE_0;
	spi_device->bits_per_word = 8;
	spi_device->irq = -1;
	spi_device->controller_state = NULL;
	spi_device->controller_data = NULL;
	spi_device->dev.platform_data = &dogxl160fb_data;
    
	printk(KERN_INFO "%s: Initialized pdata! pdata=%p\n", DRVNAME, &dogxl160fb_data );

	strlcpy(spi_device->modalias, DRVNAME, SPI_NAME_SIZE);
	status = spi_add_device(spi_device);
	
	if (status < 0) {	
	    spi_dev_put(spi_device);
	    printk(KERN_ALERT "%s: spi_add_device() failed: %d\n", 
		DRVNAME, status);		
	}				
    } 
  
    put_device(&spi_master->dev);
    printk(KERN_INFO "%s: add_dogxl160fb_device_to_bus(void) status=%d\n", DRVNAME, status );
 
    return status;
}



static const struct spi_device_id dogxl160fb_ids[] = {
 	{ DRVNAME, DOGXL160_DISPLAY }, 
	{ },
};


MODULE_DEVICE_TABLE(spi, dogxl160fb_ids); 

static struct spi_driver dogxl160fb_driver = {
	.driver = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
	.id_table = dogxl160fb_ids, 
	.probe  = dogxl160fb_probe,
	.remove = __devexit_p(dogxl160fb_remove),
};

static int __init dogxl160fb_init(void) 
{
	int ret;
	printk(KERN_INFO "%s: --------- init BUILD %s, %s -----------\n", DRVNAME, __DATE__ , __TIME__ );

	if((ret = add_dogxl160fb_device_to_bus())) {
	    printk(KERN_INFO "%s: failed to register SPI device ret=%d\n", DRVNAME, ret );
	}
	ret = spi_register_driver(&dogxl160fb_driver);
	printk(KERN_INFO "%s: register spi driver ret=%d\n", DRVNAME, ret ); 

	return ret;
}

static void __exit dogxl160fb_exit(void) 
{
	spi_unregister_driver(&dogxl160fb_driver);
	printk(KERN_INFO "%s: ----------exit -------- \n", DRVNAME );
}

/* ------------------------------------------------------------------------- */

module_init(dogxl160fb_init);
module_exit(dogxl160fb_exit);

MODULE_DESCRIPTION("FB driver for dogxl160fb display controller");
MODULE_AUTHOR("Thomas Schmidt");
MODULE_LICENSE("GPL");



