





































#ifndef __KWS_DETECTIONS_H__
#define __KWS_DETECTIONS_H__


#include <sphinxbase/glist.h>


#include "pocketsphinx_internal.h"
#include "hmm.h"

typedef struct kws_detection_s {
    const char* keyphrase;
    frame_idx_t sf;
    frame_idx_t ef;
    int32 prob;
    int32 ascr;
} kws_detection_t;

typedef struct kws_detections_s {
    glist_t detect_list;
    gnode_t *insert_ptr;
} kws_detections_t;




void kws_detections_reset(kws_detections_t *detections);




void kws_detections_add(kws_detections_t *detections, const char* keyphrase, int sf, int ef, int prob, int ascr);




void kws_detections_hyp_str(kws_detections_t *detections, char** hyp_str);

#endif                          