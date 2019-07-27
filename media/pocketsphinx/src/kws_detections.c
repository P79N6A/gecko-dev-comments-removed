





































#include "kws_detections.h"

void
kws_detections_reset(kws_detections_t *detections)
{
    gnode_t *gn;

    if (!detections->detect_list)
        return;

    for (gn = detections->detect_list; gn; gn = gnode_next(gn))
        ckd_free(gnode_ptr(gn));
    detections->detect_list = NULL;
}

void
kws_detections_add(kws_detections_t *detections, const char* keyphrase, int sf, int ef, int prob, int ascr)
{
    kws_detection_t* detection;

    detection = (kws_detection_t *)ckd_calloc(1, sizeof(*detection));
    detection->sf = sf;
    detection->ef = ef;
    detection->keyphrase = keyphrase;
    detection->prob = prob;
    detection->ascr = ascr;
    if (!detections->detect_list) {
        detections->detect_list = glist_add_ptr(detections->detect_list, (void *)detection);
        detections->insert_ptr = detections->detect_list;
    } else {
        detections->insert_ptr = glist_insert_ptr(detections->insert_ptr, (void *)detection);
    }
}

void
kws_detections_hyp_str(kws_detections_t *detections, char** hyp_str)
{
    gnode_t *gn;
    char *c;
    int len;

    len = 0;
    for (gn = detections->detect_list; gn; gn = gnode_next(gn))
        len += strlen(((kws_detection_t *)gnode_ptr(gn))->keyphrase) + 2;

    if (len == 0) {
        hyp_str = NULL;
        return;
    }

    *hyp_str = (char *)ckd_calloc(len, sizeof(char));
    c = *hyp_str;
    for (gn = detections->detect_list; gn; gn = gnode_next(gn)) {
        const char *word = ((kws_detection_t *)gnode_ptr(gn))->keyphrase;
        memcpy(c, word, strlen(word));
        c += strlen(word);
        *c = ' ';
        c++;
    }
    c--;
    *c = '\0';
}

