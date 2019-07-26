











































#ifndef __NRSTATS_H__
#define __NRSTATS_H__

#include <sys/types.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <r_types.h>

#ifndef CAPTURE_USER
#define CAPTURE_USER "pcecap"
#endif

#define NR_MAX_STATS_TYPES         256   /* max number of stats objects */
#define NR_MAX_STATS_TYPE_NAME     26

typedef struct NR_stats_type_ {
      char name[NR_MAX_STATS_TYPE_NAME];
      int (*reset)(void *stats);
      int (*print)(void *stats, char *stat_namespace, void (*output)(void *handle, const char *fmt, ...), void *handle);
      int  (*get_lib_name)(char **libname);
      unsigned int size;
} NR_stats_type;

typedef struct NR_stats_app_ {
        time_t               last_counter_reset;
        time_t               last_restart;
        UINT8                total_restarts;
        char                 version[64];
} NR_stats_app;

extern NR_stats_type *NR_stats_type_app;


typedef struct NR_stats_memory_ {
        UINT8                current_size;
        UINT8                max_size;
        UINT8                in_use;
        UINT8                in_use_max;
} NR_stats_memory;

extern NR_stats_type *NR_stats_type_memory;





extern int NR_stats_startup(char *app_name, char *user_name, void (*errprintf)(void *handle, const char *fmt, ...), void *errhandle);
extern int NR_stats_shutdown(void);
#define NR_STATS_CREATE  (1<<0)
extern int NR_stats_get(char *module_name, NR_stats_type *type, int flag, void **stats);
extern int NR_stats_clear(void *stats);   
extern int NR_stats_reset(void *stats);   
extern int NR_stats_register(NR_stats_type *type);
extern int NR_stats_acquire_mutex(void *stats);
extern int NR_stats_release_mutex(void *stats);

extern int NR_stats_get_names(unsigned int *nnames, char ***names);
extern int NR_stats_get_by_name(char *name, NR_stats_type **type, void **stats);
extern int NR_stats_get_lib_name(void *stats, char **lib_name);
extern int NR_stats_rmids(void);

extern char *NR_prefix_to_stats_module(char *prefix);

#define NR_INCREMENT_STAT(stat) do { \
       stat++; if(stat>stat##_max) stat##_max=stat; \
     } while (0)
#define NR_UPDATE_STAT(stat,newval) do { \
       stat=newval; if(stat>stat##_max) stat##_max=stat; \
     } while (0)

#endif
