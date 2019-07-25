


























#ifndef _SYS_TLS_H
#define _SYS_TLS_H

#include <sys/cdefs.h>

__BEGIN_DECLS













#define BIONIC_TLS_SLOTS            64








#define TLS_SLOT_SELF               0
#define TLS_SLOT_THREAD_ID          1
#define TLS_SLOT_ERRNO              2

#define TLS_SLOT_OPENGL_API         3
#define TLS_SLOT_OPENGL             4






#define  TLS_SLOT_BIONIC_PREINIT    (TLS_SLOT_ERRNO+1)










#define TLS_SLOT_MAX_WELL_KNOWN     TLS_SLOT_ERRNO

#define TLS_DEFAULT_ALLOC_MAP       0x0000001F


extern void __init_tls(void**  tls, void*  thread_info);


extern int __set_tls(void *ptr);


#ifdef __arm__





#  ifdef HAVE_ARM_TLS_REGISTER
#    define __get_tls() \
    ({ register unsigned int __val asm("r0"); \
       asm ("mrc p15, 0, r0, c13, c0, 3" : "=r"(__val) ); \
       (volatile void*)__val; })
#  else 
#    define __get_tls() ( *((volatile void **) 0xffff0ff0) )
#  endif
#else
extern void*  __get_tls( void );
#endif


extern void*  __get_stack_base(int  *p_stack_size);

__END_DECLS

#endif 
