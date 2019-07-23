















#if !defined(_bitpack_H)
# define _bitpack_H (1)
# include <ogg/ogg.h>

void theorapackB_readinit(oggpack_buffer *_b,unsigned char *_buf,int _bytes);
int theorapackB_look1(oggpack_buffer *_b,long *_ret);
void theorapackB_adv1(oggpack_buffer *_b);

int theorapackB_read(oggpack_buffer *_b,int _bits,long *_ret);
int theorapackB_read1(oggpack_buffer *_b,long *_ret);
long theorapackB_bytes(oggpack_buffer *_b);
long theorapackB_bits(oggpack_buffer *_b);
unsigned char *theorapackB_get_buffer(oggpack_buffer *_b);








#endif
