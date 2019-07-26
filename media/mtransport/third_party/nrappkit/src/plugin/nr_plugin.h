





































#ifndef _nr_plugin_h
#define _nr_plugin_h

typedef int (NR_plugin_hook)(void);

typedef struct NR_plugin_hook_def_ {
     char *type;
     NR_plugin_hook *func;
} NR_plugin_hook_def;

typedef struct NR_plugin_def_ {
     int api_version; 
     char *name;
     char *version;
     NR_plugin_hook_def *hooks;
} NR_plugin_def;

#endif

