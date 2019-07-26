



#ifndef _CPR_LINUX_ASSERT_H_
#define _CPR_LINUX_ASSERT_H_

#include "assert.h"















#ifdef FILE_ID
#define cpr_assert(expr) \
    ((expr) ? (void)0 : cpr_assert_msg(FILE_ID, __LINE__, #expr))
#else
#define cpr_assert(expr) \
    ((expr) ? (void)0 : cpr_assert_msg(__FILE__, __LINE__, #expr))
#endif

#define cpr_assert_debug(expr)


















#define cpr_assert_debug_rtn(expr)










typedef enum {
    CPR_ASSERT_MODE_NONE,            
    CPR_ASSERT_MODE_WARNING_LIMITED, 
    CPR_ASSERT_MODE_WARNING_ALL,     
    CPR_ASSERT_MODE_ABORT            
} cpr_assert_mode_e;







extern uint32_t cpr_assert_count;






void
cpr_assert_msg(const char *file, const int line, const char *expression);

#endif
