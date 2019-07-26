












































#ifndef ERR_H
#define ERR_H

#include "datatypes.h"


















typedef enum {
  err_status_ok           = 0,  
  err_status_fail         = 1,  
  err_status_bad_param    = 2,  
  err_status_alloc_fail   = 3,  
  err_status_dealloc_fail = 4,  
  err_status_init_fail    = 5,  
  err_status_terminus     = 6,  
  err_status_auth_fail    = 7,  
  err_status_cipher_fail  = 8,  
  err_status_replay_fail  = 9,  
  err_status_replay_old   = 10, 
  err_status_algo_fail    = 11, 
  err_status_no_such_op   = 12, 
  err_status_no_ctx       = 13, 
  err_status_cant_check   = 14, 
  err_status_key_expired  = 15, 
  err_status_socket_err   = 16, 
  err_status_signal_err   = 17, 
  err_status_nonce_bad    = 18, 
  err_status_read_fail    = 19, 
  err_status_write_fail   = 20, 
  err_status_parse_err    = 21, 
  err_status_encode_err   = 22, 
  err_status_semaphore_err = 23,
  err_status_pfkey_err    = 24  
} err_status_t;





typedef enum {
  err_level_emergency = 0,
  err_level_alert,
  err_level_critical,
  err_level_error,
  err_level_warning,
  err_level_notice,
  err_level_info,
  err_level_debug,
  err_level_none
} err_reporting_level_t;









err_status_t
err_reporting_init(char *ident);

#ifdef SRTP_KERNEL_LINUX
extern err_reporting_level_t err_level;
#else











void
err_report(int priority, char *format, ...);
#endif 






typedef struct {
  int   on;          
  char *name;        
} debug_module_t;

#ifdef ENABLE_DEBUGGING

#define debug_on(mod)  (mod).on = 1

#define debug_off(mod) (mod).on = 0


#define debug_print(mod, format, arg)                  \
  if (mod.on) err_report(err_level_debug, ("%s: " format "\n"), mod.name, arg)
#define debug_print2(mod, format, arg1,arg2)                  \
  if (mod.on) err_report(err_level_debug, ("%s: " format "\n"), mod.name, arg1,arg2)

#else


#define debug_print(mod, format, arg)

#define debug_on(mod)

#define debug_off(mod)

#endif

#endif 
