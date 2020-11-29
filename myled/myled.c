#include <linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>
#include<linux/io.h>

MODULE_AUTHOR("Natsume Takafumi");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char*buf, size_t count, loff_t* pos)
{
	 char c;
	 if (copy_from_user(&c, buf, sizeof(char)))
		 return - EFAULT;
	 if (c == '0') {
		 gpio_base[10] = 1 << 25;
		 gpio_base[7] = 1 << 16;
	  } else if (c == '1') {
		 gpio_base[7] = 1 << 25;
		 gpio_base[10] = 1 << 16;
	  }
 //      printk(KERN_INFO "receive %c\n",c);
	 return 1;
				
}

static ssize_t sushi_read(struct file* filp, char*buf, size_t count, loff_t* pos)
{
       	int size = 0;
	 char sushi[] = {'s','u','s','h','i', 0x0A};
	 if (copy_to_user(buf+size, (const char *)sushi, sizeof(sushi))){
		 printk(KERN_ERR "sushi : copy_to_user failed.\n");
		 return -EFAULT;
	 }

	 size += sizeof(sushi);
	 return size;
}

static struct file_operations led_fops = {
	 .owner = THIS_MODULE,
	 .write = led_write,
	 .read = sushi_read

};

static int __init init_mod(void)
{
	int retval;
	retval = alloc_chrdev_region(&dev, 0, 1, "myled");
 	if (retval < 0) {
		 printk(KERN_ERR "alloc_chrdev_region failed. \n");
		 return retval;
	} 

	printk(KERN_INFO "%s is loaded. major:%d\n", __FILE__, MAJOR(dev));
	cdev_init(&cdv, &led_fops);
	retval = cdev_add(&cdv, dev, 1);
	if (retval < 0) {
		printk(KERN_ERR "cdev_add failed.major:%d, minor:%d\n", MAJOR(dev), MINOR(dev));
							
	       	return retval;
	 }
	 cls = class_create(THIS_MODULE, "myled");
	 if(IS_ERR(cls)) {
		printk(KERN_ERR "class create failed.");
		return PTR_ERR(cls);
	 }

	 device_create(cls, NULL, dev, NULL, "myled%d", MINOR(dev));

	 gpio_base = ioremap_nocache(0xfe200000, 0xA0);

	 const u32 led1 = 25;
	 const u32 index1 = led1 / 10;
	 const u32 shift1 = (led1 % 10)*3;
	 const u32 mask1 = ~(0x7 << shift1);
	 gpio_base[index1] = (gpio_base[index1] & mask1) | (0x1 << shift1);

	 const u32 led2 = 16;
         const u32 index2 = led2 / 10;
         const u32 shift2 = (led2 % 10)*3;
         const u32 mask2 = ~(0x7 << shift2);
         gpio_base[index2] = (gpio_base[index2] & mask2) | (0x1 << shift2);

	 return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev,1);
	printk(KERN_INFO "%s is unloaded. major:%d\n", __FILE__, MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
