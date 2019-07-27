




































#ifndef _LIBUTIL_ERR_H_
#define _LIBUTIL_ERR_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <sphinxbase/sphinxbase_export.h>


















#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define E_SYSCALL(stmt, ...)  if (stmt) E_FATAL_SYSTEM(__VA_ARGS__);

#define FILELINE  __FILE__ , __LINE__




#define E_FATAL(...)                               \
    do {                                           \
        err_msg(ERR_FATAL, FILELINE, __VA_ARGS__); \
        exit(EXIT_FAILURE);                        \
    } while (0)




#define E_FATAL_SYSTEM(...)                                                  \
    do {                                                                     \
        err_msg_system(ERR_FATAL, FILELINE, __VA_ARGS__);		     \
        exit(EXIT_FAILURE);                                                  \
    } while (0)




#define E_ERROR_SYSTEM(...)     err_msg_system(ERR_ERROR, FILELINE, __VA_ARGS__)




#define E_ERROR(...)     err_msg(ERR_ERROR, FILELINE, __VA_ARGS__)




#define E_WARN(...)      err_msg(ERR_WARN, FILELINE, __VA_ARGS__)




#define E_INFO(...)      err_msg(ERR_INFO, FILELINE, __VA_ARGS__)




#define E_INFOCONT(...)  err_msg(ERR_INFOCONT, NULL, 0, __VA_ARGS__)




#define E_INFO_NOFN(...)  err_msg(ERR_INFO, NULL, 0, __VA_ARGS__)











#ifdef SPHINX_DEBUG
#define E_DEBUG(level, ...) \
    if (err_get_debug_level() >= level) \
        err_msg(ERR_DEBUG, FILELINE, __VA_ARGS__)
#define E_DEBUGCONT(level, ...) \
    if (err_get_debug_level() >= level) \
        err_msg(ERR_DEBUG, NULL, 0, __VA_ARGS__)
#else
#define E_DEBUG(level,x)
#define E_DEBUGCONT(level,x)
#endif

typedef enum err_e {
    ERR_DEBUG,
    ERR_INFO,
    ERR_INFOCONT,
    ERR_WARN,
    ERR_ERROR,
    ERR_FATAL,
    ERR_MAX
} err_lvl_t;

SPHINXBASE_EXPORT void
err_msg(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...);

SPHINXBASE_EXPORT void
err_msg_system(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...);

SPHINXBASE_EXPORT void
err_logfp_cb(void * user_data, err_lvl_t level, const char *fmt, ...);

typedef void (*err_cb_f)(void* user_data, err_lvl_t, const char *, ...);








SPHINXBASE_EXPORT void
err_set_callback(err_cb_f callback, void *user_data);






SPHINXBASE_EXPORT void
err_set_logfp(FILE *stream);







SPHINXBASE_EXPORT FILE *
err_get_logfp(void);









SPHINXBASE_EXPORT int
err_set_logfile(const char *path);








SPHINXBASE_EXPORT
int err_set_debug_level(int level);






SPHINXBASE_EXPORT
int err_get_debug_level(void);

#ifdef __cplusplus
}
#endif

#endif
