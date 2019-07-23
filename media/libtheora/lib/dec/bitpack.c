



















#include <string.h>
#include <stdlib.h>
#include "bitpack.h"

void theorapackB_readinit(oggpack_buffer *_b,unsigned char *_buf,int _bytes){
  memset(_b,0,sizeof(*_b));
  _b->buffer=_b->ptr=_buf;
  _b->storage=_bytes;
}

int theorapackB_look1(oggpack_buffer *_b,long *_ret){
  if(_b->endbyte>=_b->storage){
    *_ret=0L;
    return -1;
  }
  *_ret=(_b->ptr[0]>>7-_b->endbit)&1;
  return 0;
}

void theorapackB_adv1(oggpack_buffer *_b){
  if(++(_b->endbit)>7){
    _b->endbit=0;
    _b->ptr++;
    _b->endbyte++;
  }
}


int theorapackB_read(oggpack_buffer *_b,int _bits,long *_ret){
  long ret;
  long m;
  long d;
  int fail;
  m=32-_bits;
  _bits+=_b->endbit;
  d=_b->storage-_b->endbyte;
  if(d<=4){
    
    if(d*8<_bits){
      *_ret=0L;
      fail=-1;
      goto overflow;
    }
    

    else if(!_bits){
      *_ret=0L;
      return 0;
    }
  }
  ret=_b->ptr[0]<<24+_b->endbit;
  if(_bits>8){
    ret|=_b->ptr[1]<<16+_b->endbit;
    if(_bits>16){
      ret|=_b->ptr[2]<<8+_b->endbit;
      if(_bits>24){
        ret|=_b->ptr[3]<<_b->endbit;
        if(_bits>32)ret|=_b->ptr[4]>>8-_b->endbit;
      }
    }
  }
  *_ret=((ret&0xFFFFFFFFUL)>>(m>>1))>>(m+1>>1);
  fail=0;
overflow:
  _b->ptr+=_bits>>3;
  _b->endbyte+=_bits>>3;
  _b->endbit=_bits&7;
  return fail;
}

int theorapackB_read1(oggpack_buffer *_b,long *_ret){
  int fail;
  if(_b->endbyte>=_b->storage){
    
    *_ret=0L;
    fail=-1;
  }
  else{
    *_ret=(_b->ptr[0]>>7-_b->endbit)&1;
    fail=0;
  }
  _b->endbit++;
  if(_b->endbit>7){
    _b->endbit=0;
    _b->ptr++;
    _b->endbyte++;
  }
  return fail;
}

long theorapackB_bytes(oggpack_buffer *_b){
  return _b->endbyte+(_b->endbit+7>>3);
}

long theorapackB_bits(oggpack_buffer *_b){
  return _b->endbyte*8+_b->endbit;
}

unsigned char *theorapackB_get_buffer(oggpack_buffer *_b){
  return _b->buffer;
}
