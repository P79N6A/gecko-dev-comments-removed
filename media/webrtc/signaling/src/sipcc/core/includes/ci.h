



#ifndef _CI_INCLUDED_H
#define _CI_INCLUDED_H

#include "plat_api.h"
#include "plat_debug.h"



#define CI_OK             (0)
#define CI_ERROR          (1)
#define CI_INVALID        (2)
#define CI_AMBIGUOUS      (3)


#define CI_PROMPT         (0x0001)




void ci_init();
int ci_process_input(const char *str, char *wkspace, int wklen);
int32_t ci_show_cmds(int32_t argc, const char *argv[]);
ci_callback ci_set_interceptor(ci_callback func);

int ci_err_too_few(void);        
int ci_err_too_many(void);       
int ci_err_inv_arg(void);        
uint32_t ci_streval(const char *str);

#endif 
