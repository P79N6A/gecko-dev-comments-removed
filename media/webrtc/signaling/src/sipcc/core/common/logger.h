



#ifndef _LOGGER_INCLUDED_H
#define _LOGGER_INCLUDED_H

#define LOG_MAX_LEN 64




void log_msg(int log, ...);
void log_clear(int msg);
char *get_device_name();

#endif 
