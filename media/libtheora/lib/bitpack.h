















#if !defined(_bitpack_H)
# define _bitpack_H (1)
# include <limits.h>



typedef unsigned long      oc_pb_window;
typedef struct oc_pack_buf oc_pack_buf;



# define OC_PB_WINDOW_SIZE ((int)sizeof(oc_pb_window)*CHAR_BIT)



# define OC_LOTS_OF_BITS (0x40000000)



struct oc_pack_buf{
  oc_pb_window         window;
  const unsigned char *ptr;
  const unsigned char *stop;
  int                  bits;
  int                  eof;
};

void oc_pack_readinit(oc_pack_buf *_b,unsigned char *_buf,long _bytes);
int oc_pack_look1(oc_pack_buf *_b);
void oc_pack_adv1(oc_pack_buf *_b);

long oc_pack_read(oc_pack_buf *_b,int _bits);
int oc_pack_read1(oc_pack_buf *_b);

long oc_pack_bytes_left(oc_pack_buf *_b);







#endif
