







































#ifndef _AD_H_
#define _AD_H_

#include <sphinx_config.h>

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define DEFAULT_SAMPLES_PER_SEC	16000


#define AD_OK		0
#define AD_EOF		-1
#define AD_ERR_GEN	-1
#define AD_ERR_NOT_OPEN	-2
#define AD_ERR_WAVE	-3

typedef struct ad_rec_s ad_rec_t;










SPHINXBASE_EXPORT
ad_rec_t *ad_open_dev (
	const char *dev, 
	int32 samples_per_sec 
	);




SPHINXBASE_EXPORT
ad_rec_t *ad_open_sps (
		       int32 samples_per_sec 
		       );





SPHINXBASE_EXPORT
ad_rec_t *ad_open ( void );



SPHINXBASE_EXPORT
int32 ad_start_rec (ad_rec_t *);



SPHINXBASE_EXPORT
int32 ad_stop_rec (ad_rec_t *);



SPHINXBASE_EXPORT
int32 ad_close (ad_rec_t *);






SPHINXBASE_EXPORT
int32 ad_read (ad_rec_t *, int16 *buf, int32 max);


#ifdef __cplusplus
}
#endif

#endif
