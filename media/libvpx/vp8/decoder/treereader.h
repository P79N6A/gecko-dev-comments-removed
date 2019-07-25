










#ifndef tree_reader_h
#define tree_reader_h 1

#include "vp8/common/treecoder.h"

#include "dboolhuff.h"

typedef BOOL_DECODER vp8_reader;

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( R, vp8_prob_half)




static int vp8_treed_read(
    vp8_reader *const r,        
    vp8_tree t,
    const vp8_prob *const p
)
{
    register vp8_tree_index i = 0;

    while ((i = t[ i + vp8_read(r, p[i>>1])]) > 0) ;

    return -i;
}

#endif 
