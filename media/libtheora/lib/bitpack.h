















#if !defined(_bitpack_H)
# define _bitpack_H (1)
# include <stddef.h>
# include <limits.h>
# include "internal.h"



typedef size_t             oc_pb_window;
typedef struct oc_pack_buf oc_pack_buf;




# if defined(OC_ARM_ASM)
#  include "arm/armbits.h"
# endif

# if !defined(oc_pack_read)
#  define oc_pack_read oc_pack_read_c
# endif
# if !defined(oc_pack_read1)
#  define oc_pack_read1 oc_pack_read1_c
# endif
# if !defined(oc_huff_token_decode)
#  define oc_huff_token_decode oc_huff_token_decode_c
# endif

# define OC_PB_WINDOW_SIZE ((int)sizeof(oc_pb_window)*CHAR_BIT)



# define OC_LOTS_OF_BITS (0x40000000)



struct oc_pack_buf{
  const unsigned char *stop;
  const unsigned char *ptr;
  oc_pb_window         window;
  int                  bits;
  int                  eof;
};

void oc_pack_readinit(oc_pack_buf *_b,unsigned char *_buf,long _bytes);
int oc_pack_look1(oc_pack_buf *_b);
void oc_pack_adv1(oc_pack_buf *_b);

long oc_pack_read_c(oc_pack_buf *_b,int _bits);
int oc_pack_read1_c(oc_pack_buf *_b);

long oc_pack_bytes_left(oc_pack_buf *_b);







#endif
