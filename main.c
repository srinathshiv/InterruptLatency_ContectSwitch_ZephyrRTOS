/*Interrupt latency */
#include <zephyr.h>
#include <shell/shell.h>
#include <misc/printk.h>
#include <device.h>
#include <pwm.h>
#include <gpio.h>
#include <pinmux.h>
#include "board.h"
#include <kernel.h>

#define GPIOPIN	4		// IO1 - GPIO4 
#define	PWMPIN 1		// IO3 - PWM1
#define IO1 1
#define IO2 2
#define IO3 3
#define IO6 6
#define STACK_SIZE 256
#define PRIORITY0	7
#define PRIORITY1	8

#define MY_SHELL_MODULE "cse522"

uint64_t istart, iend; //, cstart0, cend0, cstart1, cend1;
uint64_t buf0[500],buf1[500],buf2[500];
int global_police=0;
int global_alt=0;
int global_flag=0;

/* Callback (ISR) */
static struct gpio_callback gpio_cb;
int flag = 0;
void gpio_isr(struct device *gpiob, struct gpio_callback *cb, u32_t pins){
iend = _tsc_read();

if(global_flag==0) {
buf0[global_police]=(iend-istart);
//printk("%llu\n",buf0[global_police]);
}

else if(global_flag==1 && global_alt<500){
buf1[global_alt]=(iend-istart);
global_alt++;
//printk("\nInterrupt latency:%llu",iend-istart);
//printk("%llu\n",buf1[global_police]);
}
flag = 1;
}

K_MUTEX_DEFINE(my_mutex);

/* Message queue data item  and definition */
struct data_item{
uint32_t data1;
uint32_t data2;
}data;
K_MSGQ_DEFINE(my_msgq, sizeof(struct data_item),10,4);


void sender(void *a,void *b,void *c){
	while(1){	
		data.data1++;
		data.data2++;
		
		k_mutex_lock(&my_mutex, K_FOREVER);
		k_msgq_put(&my_msgq, &data, K_NO_WAIT);
		k_mutex_unlock(&my_mutex);					
		k_sleep(1);
	
// reentry point

	}
}

void receiver(void *a,void *b,void *c){
	while(1){

			k_mutex_lock(&my_mutex, K_FOREVER);
			k_msgq_get(&my_msgq, &data, K_NO_WAIT);
			k_mutex_unlock(&my_mutex);
			k_sleep(1);	
			
		}
}

// Static definition of threads 

K_THREAD_STACK_DEFINE(stack_area0, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_area1, STACK_SIZE);

K_THREAD_STACK_DEFINE(stack_area2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_area3, STACK_SIZE);


uint64_t csstart, csend;

K_MUTEX_DEFINE(mutex2);


int shared_variable=1;

void lower(void *a, void *b, void *c){
	
	while(1){

		k_mutex_lock(&mutex2,K_FOREVER);
		//shared_variable++;
		shared_variable = 1;
		csstart= _tsc_read();
		
		k_mutex_unlock(&mutex2);
		k_yield();
	}
}

void higher(void *a, void *b, void *c){
static int loop=0;
		while(1){
		
			k_mutex_lock(&mutex2, K_FOREVER);
			while(shared_variable==0) 
					printk("");
			csend = _tsc_read();
	//		printk("\nCS time:%llu", csend-csstart);
			if(loop < 500){
			buf2[loop]= csend-csstart;	
//			printk("%llu\n",buf2[loop]);
			}

			loop++;
			shared_variable = 0;
			k_mutex_unlock(&mutex2);
			k_sleep(1);
		}
}

static int shell_cmd_qsn1(int argc,char *argv[])
{
	int i=0;
	printk("\n");
	for(i=0 ; i<500;i++)
		printk("%llu\t",buf0[i]);

	return 0;
}
 
static int shell_cmd_qsn2(int argc,char *argv[])
{
	int i=0;
	printk("\n");
	for(i=0 ; i<500;i++)
		printk("%llu\t",buf1[i]);

	return 0;
}
 
static int shell_cmd_qsn3(int argc,char *argv[])
{
	int i=0;
	printk("\n");
	for(i=0 ; i<500;i++)
		printk("%llu\t",buf2[i]);
	return 0;
}

static struct shell_cmd commands[]={
		{ "qsn1", shell_cmd_qsn1, "Interrupt Latency-no background"},
		{ "qsn2", shell_cmd_qsn2, "Interrupt Latency- with bkground"},
		{ "qsn3", shell_cmd_qsn3, "Context Switch"},
		{NULL, NULL, NULL}
	};


 
void main(void)
{
	int i,ret;
	struct device *pwm;
	struct device *gpio;
	struct device *pinmux;
	u32_t period = 4095;
	
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
	
	u32_t pulse  = 3096;

	k_tid_t thread_sender,thread_receiver, th_lo, th_hi;
	struct k_thread thread_sender_data, thread_receiver_data;
	struct k_thread thread_lower_data, thread_higher_data;
//	k_sem_reset(&sem0);
//	k_sem_reset(&sem1);
	data.data1 = 1;
	data.data2 = 2;

	pinmux = device_get_binding(CONFIG_PINMUX_NAME);
	gpio   = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	pwm    = device_get_binding(PINMUX_GALILEO_PWM0_NAME);

	pinmux_pin_set(pinmux,IO1, PINMUX_FUNC_B);
//	pinmux_pin_set(pinmux,IO6, PINMUX_FUNC_C);

	pinmux_pin_set(pinmux,IO3, PINMUX_FUNC_A); //Output IO3 - GPIO6
	
	gpio_pin_configure(gpio,5, GPIO_DIR_OUT);
	gpio_pin_configure(gpio,GPIOPIN ,GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH );
	
	gpio_init_callback(&gpio_cb, gpio_isr, BIT(4)); 
	gpio_add_callback(gpio, &gpio_cb);
	gpio_pin_enable_callback(gpio, 4);

	
	for(i=0; i<500; i++){
	istart = _tsc_read();
	gpio_pin_write(gpio,6,1);
	while(flag==0)
		printk("");
	flag = 0;
	gpio_pin_write(gpio,6,0);

	global_police++;
	
	}
	
	global_police=0; // for buf2[]
		

	thread_sender = k_thread_create(&thread_sender_data ,stack_area0, K_THREAD_STACK_SIZEOF(stack_area0), 
					sender, NULL,NULL,NULL,
					PRIORITY0,0,K_NO_WAIT);

	thread_receiver = k_thread_create( &thread_receiver_data,stack_area1, K_THREAD_STACK_SIZEOF(stack_area1), 
					receiver, NULL,NULL,NULL,
					PRIORITY1,0,K_NO_WAIT);

	ret=pinmux_pin_set(pinmux,IO3, PINMUX_FUNC_D);

	
	global_flag=1;
	for(i=0;i<500; i++){
		istart = _tsc_read();	
		ret = pwm_pin_set_cycles(pwm, 1, period, pulse);
		while(flag==0)
			printk(" ");
		flag=0;
		k_sleep(1);
	}
	gpio_pin_disable_callback(gpio,4);
	k_thread_abort(thread_sender);
	k_thread_abort(thread_receiver);

	th_lo = k_thread_create(&thread_lower_data ,stack_area2, K_THREAD_STACK_SIZEOF(stack_area2),
                     lower, NULL,NULL,NULL,
                     8,0,K_NO_WAIT); 
    th_hi = k_thread_create( &thread_higher_data,stack_area3, K_THREAD_STACK_SIZEOF(stack_area3),
                     higher, NULL,NULL,NULL,
                     6,0,K_NO_WAIT);

	for(i=0;i<500;i++){
//	printk("\nlol");
	k_sleep(1);
	}

	gpio_pin_disable_callback(gpio,4);
	k_thread_abort(th_lo);
	k_thread_abort(th_hi);

}
