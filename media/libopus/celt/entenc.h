



































#if !defined(_entenc_H)
# define _entenc_H (1)
# include <stddef.h>
# include "entcode.h"




void ec_enc_init(ec_enc *_this,unsigned char *_buf,opus_uint32 _size);













void ec_encode(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _ft);


void ec_encode_bin(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _bits);


void ec_enc_bit_logp(ec_enc *_this,int _val,unsigned _logp);








void ec_enc_icdf(ec_enc *_this,int _s,const unsigned char *_icdf,unsigned _ftb);





void ec_enc_uint(ec_enc *_this,opus_uint32 _fl,opus_uint32 _ft);





void ec_enc_bits(ec_enc *_this,opus_uint32 _fl,unsigned _ftb);















void ec_enc_patch_initial_bits(ec_enc *_this,unsigned _val,unsigned _nbits);









void ec_enc_shrink(ec_enc *_this,opus_uint32 _size);




void ec_enc_done(ec_enc *_this);

#endif
