






































#ifndef _r_log_h
#define _r_log_h

#ifndef WIN32
#include <syslog.h>
#endif
#include <stdarg.h>
#include <r_common.h>

int r_log(int facility,int level,const char *fmt,...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 3, 4)))
#endif
;

int r_vlog(int facility,int level,const char *fmt,va_list ap);
int r_dump(int facility,int level,char *name,char *data,int len);

int r_log_e(int facility,int level,const char *fmt,...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 3, 4)))
#endif
;

int r_vlog_e(int facility,int level,const char *fmt,va_list ap);
int r_log_nr(int facility,int level,int r,const char *fmt,...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 4, 5)))
#endif
;

int r_vlog_nr(int facility,int level,int r,const char *fmt,va_list ap);

int r_log_register(char *tipename,int *facility);
int r_log_facility(int facility,char **tipename);
int r_logging(int facility, int level);
int r_log_init(void);

#define LOG_GENERIC 0
#define LOG_COMMON 0

typedef int r_dest_vlog(int facility,int level,const char *format,va_list ap);
int r_log_set_extra_destination(int default_level, r_dest_vlog *dest_vlog);

#endif

