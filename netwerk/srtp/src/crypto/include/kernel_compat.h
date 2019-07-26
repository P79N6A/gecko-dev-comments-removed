












































#ifndef KERNEL_COMPAT_H
#define KERNEL_COMPAT_H

#ifdef SRTP_KERNEL_LINUX

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/byteorder/generic.h>


#define err_report(priority, ...) \
  do {\
    if (priority <= err_level) {\
       printk(__VA_ARGS__);\
    }\
  }while(0)

#define clock()	(jiffies)
#define time(x)	(jiffies)


#define RAND_MAX	32767

static inline int rand(void)
{
	uint32_t temp;
	get_random_bytes(&temp, sizeof(temp));
	return temp % (RAND_MAX+1);
}


#define printf(...)	printk(__VA_ARGS__)
#define exit(n)	panic("%s:%d: exit(%d)\n", __FILE__, __LINE__, (n))

#endif 

#endif 
