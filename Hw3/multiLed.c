#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static unsigned int gpioLED[2] = {18, 15};            ///< Default GPIO for the LED is 49
static unsigned int blinkPeriod = 1000;            ///< The blink period in ms
// static unsigned int numLEDs = 2;

static char ledName[7] = "ledGOGO";       ///< Null terminated default string -- just in case
static bool ledOn = 0;                      ///< Is the LED on or off? Used for flashing
enum modes { OFF, REDON, YELON, FLASH };              ///< The available LED modes -- static not useful here
static enum modes mode = FLASH;             ///< Default mode is flashing

//module_param(gpioLED, uint, S_IRUGO);       ///< Param desc. S_IRUGO can be read/not changed
//MODULE_PARM_DESC(gpioLED, " GPIO LED number (default=49)");     ///< parameter description

//module_param(blinkPeriod, uint, S_IRUGO);   ///< Param desc. S_IRUGO can be read/not changed
//MODULE_PARM_DESC(blinkPeriod, " LED blink period in ms (min=1, default=1000, max=10000)");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Min-Shiuan, Lee");
MODULE_DESCRIPTION("A simple Linux LED driver LKM for the BBB");
MODULE_VERSION("0.1");

/** @brief A callback function to display the LED mode
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer to which to write the number of presses
 *  @return return the number of characters of the mode string successfully displayed
 */
// 顯示狀態
static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   switch(mode){
      case OFF:   return sprintf(buf, "off\n");       // Display the state -- simplistic approach
      case REDON: return sprintf(buf, "Red on\n");
      case YELON: return sprintf(buf, "Yellow on\n");
      case FLASH: return sprintf(buf, "flash\n");
      default:    return sprintf(buf, "LKM Error\n"); // Cannot get here
   }
}

/** @brief A callback function to store the LED mode using the enum above */
// 儲存狀態
static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
   // the count-1 is important as otherwise the \n is used in the comparison
   if (strncmp(buf,"Red on",count-1)==0) { mode = REDON; }   // strncmp() compare with fixed number chars
   else if (strncmp(buf,"Yellow on",count-1)==0) { mode = YELON; }
   else if (strncmp(buf,"off",count-1)==0) { mode = OFF; }
   else if (strncmp(buf,"flash",count-1)==0) { mode = FLASH; }
   return count;
}

/** @brief A callback function to display the LED period */
static ssize_t period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   return sprintf(buf, "%d\n", blinkPeriod);
}

/** @brief A callback function to store the LED period value */
static ssize_t period_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
   unsigned int period;                     // Using a variable to validate the data sent
   sscanf(buf, "%du", &period);             // Read in the period as an unsigned int
   if ((period>1)&&(period<=10000)){        // Must be 2ms or greater, 10secs or less
      blinkPeriod = period;                 // Within range, assign to blinkPeriod variable
   }
   return period;
}
static struct kobj_attribute period_attr=__ATTR(blinkPeriod,0664,period_show,period_store);
static struct kobj_attribute mode_attr=__ATTR(mode,0664,mode_show,mode_store);
static struct attribute *ebb_attrs[] = {
   &period_attr.attr,                       // The period at which the LED flashes
   &mode_attr.attr,                         // Is the LED on or off?
   NULL,
};
static struct attribute_group attr_group = {
   .name  = ledName,                        // The name is generated in ebbLED_init()
   .attrs = ebb_attrs,                      // The attributes array defined just above
};

static struct kobject *ebb_kobj;            /// The pointer to the kobject
static struct task_struct *task;            /// The pointer to the thread task
static int flash(void *arg){
   printk(KERN_INFO "EBB LED: Thread has started running \n");
   while(!kthread_should_stop()){           // Returns true when kthread_stop() is called
      set_current_state(TASK_RUNNING);
      if (mode==FLASH) ledOn = !ledOn;      // Invert the LED state
      else if (mode==REDON){
         ledOn = true;
         gpio_set_value(gpioLED[0], ledOn);
         gpio_set_value(gpioLED[1], !ledOn);
      } 
      else if (mode==YELON){
         ledOn = true;
         gpio_set_value(gpioLED[0], !ledOn);
         gpio_set_value(gpioLED[1], ledOn);
      }
      else {
         ledOn = false;
         gpio_set_value(gpioLED[0], ledOn); // Use the LED state to light/turn off the LED
         gpio_set_value(gpioLED[1], !ledOn);
      }
      
      set_current_state(TASK_INTERRUPTIBLE);
      msleep(blinkPeriod/2);                // millisecond sleep for half of the period
   }
   printk(KERN_INFO "EBB LED: Thread has run to completion \n");
   return 0;
}

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbLED_init(void){
   int result = 0;

   printk(KERN_INFO "EBB LED: Initializing the EBB LED LKM\n");
   // sprintf(ledName, "led%d", gpioLED);      // Create the gpio115 name for /sys/ebb/led49

   ebb_kobj = kobject_create_and_add("ebb", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!ebb_kobj){
      printk(KERN_ALERT "EBB LED: failed to create kobject\n");
      return -ENOMEM;
   }
   // add the attributes to /sys/ebb/ -- for example, /sys/ebb/led49/ledOn

   // 上面好像是必做的 下面應該是判斷能不能執行

   result = sysfs_create_group(ebb_kobj, &attr_group);
   if(result) {
      printk(KERN_ALERT "EBB LED: failed to create sysfs group\n");
      kobject_put(ebb_kobj);                // clean up -- remove the kobject sysfs entry
      return result;
   }
   ledOn = true;
   gpio_request(gpioLED[0], "sysfs");          // gpioLED is 49 by default, request it
   gpio_direction_output(gpioLED[0], ledOn);   // Set the gpio to be in output mode and turn on
   gpio_export(gpioLED[0], false);  // causes gpio49 to appear in /sys/class/gpio
                                 // the second argument prevents the direction from being changed
   
   gpio_request(gpioLED[1], "sysfs");          // gpioLED is 49 by default, request it
   gpio_direction_output(gpioLED[1], ledOn);   // Set the gpio to be in output mode and turn on
   gpio_export(gpioLED[1], false);

   task = kthread_run(flash, NULL, "LED_flash_thread");  // Start the LED flashing thread
   if(IS_ERR(task)){                                     // Kthread name is LED_flash_thread
      printk(KERN_ALERT "EBB LED: failed to create the task\n");
      return PTR_ERR(task);
   }
   return result;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ebbLED_exit(void){
   kthread_stop(task);                      // Stop the LED flashing thread
   kobject_put(ebb_kobj);                   // clean up -- remove the kobject sysfs entry
   gpio_set_value(gpioLED[0], 0);              // Turn the LED off, indicates device was unloaded
   gpio_unexport(gpioLED[0]);                  // Unexport the Button GPIO
   gpio_free(gpioLED[0]);                      // Free the LED GPIO
   gpio_set_value(gpioLED[1], 0);              // Turn the LED off, indicates device was unloaded
   gpio_unexport(gpioLED[1]);                  // Unexport the Button GPIO
   gpio_free(gpioLED[1]);                      // Free the LED GPIO
   printk(KERN_INFO "EBB LED: Goodbye from the EBB LED LKM!\n");
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbLED_init);
module_exit(ebbLED_exit);
