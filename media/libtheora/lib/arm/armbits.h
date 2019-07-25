















#if !defined(_arm_armbits_H)
# define _arm_armbits_H (1)
# include "../bitpack.h"
# include "armcpu.h"

# if defined(OC_ARM_ASM)
#  define oc_pack_read oc_pack_read_arm
#  define oc_pack_read1 oc_pack_read1_arm
#  define oc_huff_token_decode oc_huff_token_decode_arm
# endif

long oc_pack_read_arm(oc_pack_buf *_b,int _bits);
int oc_pack_read1_arm(oc_pack_buf *_b);
int oc_huff_token_decode_arm(oc_pack_buf *_b,const ogg_int16_t *_tree);

#endif
