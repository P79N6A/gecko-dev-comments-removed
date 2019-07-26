











































#include "err.h"

#ifdef ERR_REPORTING_SYSLOG
# ifdef HAVE_SYSLOG_H
#  include <syslog.h>
# endif
#endif




err_reporting_level_t err_level = err_level_none;

#ifdef SRTP_KERNEL_LINUX
err_status_t
err_reporting_init(char *ident) {

  return err_status_ok;
}

#else 	



static FILE *err_file = NULL;

err_status_t
err_reporting_init(char *ident) {
#ifdef ERR_REPORTING_SYSLOG
  openlog(ident, LOG_PID, LOG_AUTHPRIV);
#endif
  
  




#ifdef ERR_REPORTING_STDOUT
  err_file = stdout;
#elif defined(USE_ERR_REPORTING_FILE)
  
  err_file = fopen(ERR_REPORTING_FILE, "w");
  if (err_file == NULL)
    return err_status_init_fail;
#endif

  return err_status_ok;
}

void
err_report(int priority, char *format, ...) {
  va_list args;

  if (priority <= err_level) {

    va_start(args, format);
    if (err_file != NULL) {
      vfprintf(err_file, format, args);
	  
    }
#ifdef ERR_REPORTING_SYSLOG
    if (1) { 
      int syslogpri;

      switch (priority) {
      case err_level_emergency:
	syslogpri = LOG_EMERG;
	break;
      case err_level_alert:
	syslogpri = LOG_ALERT;
	break;
      case err_level_critical:
	syslogpri = LOG_CRIT;
	break;
      case err_level_error:
	syslogpri = LOG_ERR;
	break;
      case err_level_warning:
	syslogpri = LOG_WARNING;
	break;
      case err_level_notice:
	syslogpri = LOG_NOTICE;
	break;
      case err_level_info:
	syslogpri = LOG_INFO;
	break;
      case err_level_debug:
      case err_level_none:
      default:
	syslogpri = LOG_DEBUG;
	break;
      }

      vsyslog(syslogpri, format, args);
#endif
    va_end(args);
  }
}
#endif 	

void
err_reporting_set_level(err_reporting_level_t lvl) { 
  err_level = lvl;
}
