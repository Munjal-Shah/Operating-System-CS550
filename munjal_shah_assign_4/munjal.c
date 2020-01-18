#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include<linux/miscdevice.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Munjal");
MODULE_DESCRIPTION("Assignment 4: Producer Consumer.");


/** Declaring semaphores and structs **/
static DEFINE_SEMAPHORE(full);
static DEFINE_SEMAPHORE(empty);  
static DEFINE_MUTEX(mutex);  
static DEFINE_MUTEX(lock); 

static char **my_data;  
int status, size;

module_param(size, int, S_IRUSR | S_IWUSR);


/** Declaring the functions **/
static ssize_t my_read(struct file *file, char __user *out, size_t size, loff_t *off);
static ssize_t my_write(struct file *file, const char *out, size_t size, loff_t *off);

static int my_open(struct inode *, struct file *);
static int my_close(struct inode *, struct file *);


/**
 * Declaring the file operation struct
 */
static struct file_operations my_fops = {
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_close
};


/**
 * Declaring a Device Struct
 */
static struct miscdevice my_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "my_device",
	.fops = &my_fops
};


/**
 * Registering the device with kernel
 */
int my_init_module(void) {
	
	int reg_status, i;
	
	sema_init(&empty, size);
	sema_init(&full, 0);
	mutex_init(&lock);
	
	printk(KERN_INFO "LinePipe: Initializing the LinePipe KO with pipe_size = %d \n", pipe_size);
	reg_status = misc_register(&my_misc_device);
	
	if(reg_status < 0) {
		printk(KERN_ALERT "LinePipe: Registration FAILED!");
		return reg_status;
	}
	
	my_data = kmalloc(sizeof(char *) * (size), GFP_KERNEL);

	for(i = 0; i < size; i++) {
		my_data[i] = kmalloc(sizeof(char *) * (100), GFP_KERNEL);
	}

	printk(KERN_INFO "Initialized\n");
	return 0;
}


/**
 * Unregistering the device when removing the module
 */
void my_cleanup_module(void) {
	printk(KERN_INFO "LinePipe: Exiting from the LinePipe KO\n");
	kfree(my_data);
	misc_deregister(&my_misc_device);
}


/**
 * Read function of the driver
 * @return
 * @param file
 * @param out
 * @param size
 * @param off
 */
static ssize_t my_read(struct file *file, char *out, size_t size, loff_t *off) {
	
	int read_pos = 0, read_size_message, read_count = 0;
	
	printk(KERN_INFO "LinePipe: Reading from the pipe...\n");
	
	if(down_interruptible (&full) < 0) {
		return -1;
	}

	if(mutex_lock_interruptible (&lock) < 0) {
		return 0;
	}

	if(read_pos == size) {
		read_pos = 0;
	}

	status = copy_to_user(out, my_data[read_pos], size);
	read_count++;
    read_size_message = strlen(my_data[read_pos]);
	read_pos++;
	mutex_unlock(&lock); 
	up(&empty); 
	
	return read_size_message;
}


/**
 * Read function of the driver
 * @return
 * @param file
 * @param buffer
 * @param len
 * @param off
 */
static ssize_t my_write(struct file *filp, const char *buff, size_t len, loff_t * off) {
	
	int write_pos = 0, write_size_message;
	
	printk(KERN_INFO "LinePipe: Writing to the pipe...\n");
	
	if(down_interruptible(&empty) < 0)  
		return -1;

	if(mutex_lock_interruptible(&lock) < 0)   
		return -1;
		
	if(write_pos == size)
		write_pos = 0;
	
	status = copy_from_user(my_data[write_pos], buff, len);
	write_size_message = strlen(my_data[write_pos]);
	write_pos++;	
	mutex_unlock(&lock);	
	up(&full); 
	
	return write_size_message;
}


/**
 * Opens the character driver
 * @return
 * @param inode
 * @param file
 */
static int my_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "LinePipe: Device Opened\n");
	return 0;
}


/**
 * Called when the driver is closed
 * @return
 * @param inode
 * @param file
 */
static int my_close(struct inode *inode, struct file *file) {
	printk(KERN_INFO "LinePipe: Device Closed!\n");
	return 0;
}


/** Register init and exit process **/
module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_LICENSE("GPL");
