









































#ifndef __SBTHREAD_H__
#define __SBTHREAD_H__

#include <sphinx_config.h>

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/cmd_ln.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




typedef struct sbthread_s sbthread_t;




typedef struct sbmsgq_s sbmsgq_t;




typedef struct sbmtx_s sbmtx_t;




typedef struct sbevent_s sbevent_t;




typedef int (*sbthread_main)(sbthread_t *th);




SPHINXBASE_EXPORT
sbthread_t *sbthread_start(cmd_ln_t *config, sbthread_main func, void *arg);




SPHINXBASE_EXPORT
int sbthread_wait(sbthread_t *th);




SPHINXBASE_EXPORT
void sbthread_free(sbthread_t *th);




SPHINXBASE_EXPORT
cmd_ln_t *sbthread_config(sbthread_t *th);




SPHINXBASE_EXPORT
void *sbthread_arg(sbthread_t *th);




SPHINXBASE_EXPORT
sbmsgq_t *sbthread_msgq(sbthread_t *th);




SPHINXBASE_EXPORT
int sbthread_wait(sbthread_t *th);







SPHINXBASE_EXPORT
int sbthread_send(sbthread_t *th, size_t len, void const *data);






SPHINXBASE_EXPORT
sbmsgq_t *sbmsgq_init(size_t depth);




SPHINXBASE_EXPORT
void sbmsgq_free(sbmsgq_t *q);




SPHINXBASE_EXPORT
int sbmsgq_send(sbmsgq_t *q, size_t len, void const *data);




SPHINXBASE_EXPORT
void *sbmsgq_wait(sbmsgq_t *q, size_t *out_len, int sec, int nsec);




SPHINXBASE_EXPORT
sbmtx_t *sbmtx_init(void);




SPHINXBASE_EXPORT
int sbmtx_trylock(sbmtx_t *mtx);




SPHINXBASE_EXPORT
int sbmtx_lock(sbmtx_t *mtx);




SPHINXBASE_EXPORT
int sbmtx_unlock(sbmtx_t *mtx);




SPHINXBASE_EXPORT
void sbmtx_free(sbmtx_t *mtx);




SPHINXBASE_EXPORT
sbevent_t *sbevent_init(void);




SPHINXBASE_EXPORT
void sbevent_free(sbevent_t *evt);




SPHINXBASE_EXPORT
int sbevent_signal(sbevent_t *evt);




SPHINXBASE_EXPORT
int sbevent_wait(sbevent_t *evt, int sec, int nsec);


#ifdef __cplusplus
}
#endif


#endif
