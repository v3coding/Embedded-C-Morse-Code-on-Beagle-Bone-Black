#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/leds.h>

#define MY_DEVICE_FILE "morse-code"
#define LED_ON_TIME_MS 200
#define LED_OFF_TIME_MS 200
#define CHAR_GAP_MS 600

#define DOT_TIME_MS 200
#define DASH_TIME_MS 600
#define GAP_TIME_MS 200
#define LETTER_GAP_MS 600
#define WORD_GAP_MS 1400

DEFINE_LED_TRIGGER(ledtrig_demo);
static char outputBuffer[4096];
static size_t outputBufferLength = 0;

static unsigned short morsecode_codes[] = {
    0xB800, // A
    0xEA80, // B
    0xEBA0, // C
    0xEA00, // D
    0x8000, // E
    0xAE80, // F
    0xEE80, // G
    0xAA00, // H
    0xA000, // I
    0xBBB8, // J
    0xEB80, // K
    0xBA80, // L
    0xEE00, // M
    0xE800, // N
    0xEEE0, // O
    0xBBA0, // P
    0xEEB8, // Q
    0xBA00, // R
    0xA800, // S
    0xE000, // T
    0xAE00, // U
    0xAB80, // V
    0xBB80, // W
    0xEAE0, // X
    0xEBB8, // Y
    0xEEA0  // Z
};

static void my_led_on(void){
    led_trigger_event(ledtrig_demo, LED_FULL);
}

static void my_led_off(int duration_ms){
    led_trigger_event(ledtrig_demo, LED_OFF);
    msleep(duration_ms);
}

static void morseCode(unsigned short code, int is_last) {
    int i;
    int hasBegun = 0;
    for (i = 15; i >= 0; i--) {
        int bit = (code >> i) & 1;

        if (hasBegun || bit) {
            hasBegun = 1;

            if (bit) {
                int next_bit = (code >> (i - 1)) & 1;
                int next_next_bit = (code >> (i - 2)) & 1;
                if (next_bit && next_next_bit) {
                    my_led_on();
                    msleep(DASH_TIME_MS);
                    outputBuffer[outputBufferLength++] = '-';
                    my_led_off(GAP_TIME_MS);
                    i -= 2;  // Skip the next two bits, as we've processed a dash (111)
                } else {
                    my_led_on();
                    msleep(DOT_TIME_MS);
                    outputBuffer[outputBufferLength++] = '.';
                    my_led_off(GAP_TIME_MS);
                }
            }
        }
    }

    if (!is_last) {
        msleep(LETTER_GAP_MS - GAP_TIME_MS);
    } else {
        msleep(WORD_GAP_MS - GAP_TIME_MS);
    }
}

static ssize_t my_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos) {
    int i;
    printk(KERN_INFO "Translating input string to morse code...\n");
    outputBufferLength = 0;
    for (i = 0; i < count - 1; i++) {
        char ch;
        int is_last = (i == count - 2);
        if (get_user(ch, buff + i)) {
            return -EFAULT;
        }
        if(ch == ' '){
            outputBuffer[outputBufferLength++] = ' ';
            outputBuffer[outputBufferLength++] = ' ';
            outputBuffer[outputBufferLength++] = ' ';
            msleep(WORD_GAP_MS);
        }
        if (ch >= 'A' && ch <= 'Z' && ch != ' ') {
            morseCode(morsecode_codes[ch - 'A'], is_last);
            outputBuffer[outputBufferLength++] = ' ';
        } else if (ch >= 'a' && ch <= 'z' && ch != ' ') {
            morseCode(morsecode_codes[ch - 'a'], is_last);
            outputBuffer[outputBufferLength++] = ' ';
        }
    }
    outputBuffer[outputBufferLength] = '\0';
    return count;
}

static ssize_t my_read(struct file *file, char __user *buff, size_t count, loff_t *ppos){
return simple_read_from_buffer(buff, count, ppos, outputBuffer, outputBufferLength);
}

struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .write = my_write,
    .read = my_read,
};

static struct miscdevice my_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEVICE_FILE,
    .fops = &my_fops,
};

static int __init my_init(void){
    int ret;
    printk(KERN_INFO "Morse Code Driver Initialized at /dev/%s.\n", MY_DEVICE_FILE);
    ret = misc_register(&my_miscdevice);
    led_trigger_register_simple("morse-code", &ledtrig_demo);
    return ret;
}

static void __exit my_exit(void){
    printk(KERN_INFO "Morse Code Driver Exit.\n");
    misc_deregister(&my_miscdevice);
    led_trigger_unregister_simple(ledtrig_demo);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick Burns");
MODULE_DESCRIPTION("A simple text to Morse Code translator driver");