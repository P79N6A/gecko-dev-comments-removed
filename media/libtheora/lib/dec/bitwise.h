















#if !defined(_bitwise_H)
# define _bitwise_H (1)
# include <ogg/ogg.h>

void theorapackB_reset(oggpack_buffer *b);
void theorapackB_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);

static int theorapackB_look(oggpack_buffer *b,int bits,long *_ret);
int theorapackB_look1(oggpack_buffer *b,long *_ret);
static void theorapackB_adv(oggpack_buffer *b,int bits);
void theorapackB_adv1(oggpack_buffer *b);

int theorapackB_read(oggpack_buffer *b,int bits,long *_ret);
int theorapackB_read1(oggpack_buffer *b,long *_ret);
long theorapackB_bytes(oggpack_buffer *b);
long theorapackB_bits(oggpack_buffer *b);
unsigned char *theorapackB_get_buffer(oggpack_buffer *b);





static int theorapackB_look(oggpack_buffer *b,int bits,long *_ret){
  long ret;
  long m;
  m=32-bits;
  bits+=b->endbit;
  if(b->endbyte+4>=b->storage){
    
    if(b->endbyte>=b->storage){
      *_ret=0L;
      return -1;
    }
    
    if((b->storage-b->endbyte)*8<bits)bits=(b->storage-b->endbyte)*8;
  }
  ret=b->ptr[0]<<(24+b->endbit);
  if(bits>8){
    ret|=b->ptr[1]<<(16+b->endbit);
    if(bits>16){
      ret|=b->ptr[2]<<(8+b->endbit);
      if(bits>24){
        ret|=b->ptr[3]<<(b->endbit);
        if(bits>32&&b->endbit)
          ret|=b->ptr[4]>>(8-b->endbit);
      }
    }
  }
  *_ret=((ret&0xffffffff)>>(m>>1))>>((m+1)>>1);
  return 0;
}

static void theorapackB_adv(oggpack_buffer *b,int bits){
  bits+=b->endbit;
  b->ptr+=bits/8;
  b->endbyte+=bits/8;
  b->endbit=bits&7;
}

#endif
