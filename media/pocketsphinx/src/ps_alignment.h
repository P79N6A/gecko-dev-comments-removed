








































#ifndef __PS_ALIGNMENT_H__
#define __PS_ALIGNMENT_H__




#include <sphinxbase/prim_type.h>


#include "dict2pid.h"
#include "hmm.h"

#define PS_ALIGNMENT_NONE ((uint16)0xffff)

struct ps_alignment_entry_s {
    union {
        int32 wid;
        struct {
            uint16 ssid;
            uint16 cipid;
            uint16 tmatid;
        } pid;
        uint16 senid;
    } id;
    int16 start;
    int16 duration;
    uint16 parent;
    uint16 child;
};
typedef struct ps_alignment_entry_s ps_alignment_entry_t;

struct ps_alignment_vector_s {
    ps_alignment_entry_t *seq;
    uint16 n_ent, n_alloc;
};
typedef struct ps_alignment_vector_s ps_alignment_vector_t;

struct ps_alignment_s {
    dict2pid_t *d2p;
    ps_alignment_vector_t word;
    ps_alignment_vector_t sseq;
    ps_alignment_vector_t state;
};
typedef struct ps_alignment_s ps_alignment_t;

struct ps_alignment_iter_s {
    ps_alignment_t *al;
    ps_alignment_vector_t *vec;
    int pos;
};
typedef struct ps_alignment_iter_s ps_alignment_iter_t;




ps_alignment_t *ps_alignment_init(dict2pid_t *d2p);




int ps_alignment_free(ps_alignment_t *al);




int ps_alignment_add_word(ps_alignment_t *al,
                          int32 wid, int duration);




int ps_alignment_populate(ps_alignment_t *al);




int ps_alignment_populate_ci(ps_alignment_t *al);




int ps_alignment_propagate(ps_alignment_t *al);




int ps_alignment_n_words(ps_alignment_t *al);




int ps_alignment_n_phones(ps_alignment_t *al);




int ps_alignment_n_states(ps_alignment_t *al);




ps_alignment_iter_t *ps_alignment_words(ps_alignment_t *al);




ps_alignment_iter_t *ps_alignment_phones(ps_alignment_t *al);




ps_alignment_iter_t *ps_alignment_states(ps_alignment_t *al);




ps_alignment_entry_t *ps_alignment_iter_get(ps_alignment_iter_t *itor);




ps_alignment_iter_t *ps_alignment_iter_goto(ps_alignment_iter_t *itor, int pos);




ps_alignment_iter_t *ps_alignment_iter_next(ps_alignment_iter_t *itor);




ps_alignment_iter_t *ps_alignment_iter_prev(ps_alignment_iter_t *itor);




ps_alignment_iter_t *ps_alignment_iter_up(ps_alignment_iter_t *itor);



ps_alignment_iter_t *ps_alignment_iter_down(ps_alignment_iter_t *itor);




int ps_alignment_iter_free(ps_alignment_iter_t *itor);

#endif 
