



































#if !defined(_entdec_H)
# define _entdec_H (1)
# include <limits.h>
# include "entcode.h"




void ec_dec_init(ec_dec *_this,unsigned char *_buf,opus_uint32 _storage);














unsigned ec_decode(ec_dec *_this,unsigned _ft);


unsigned ec_decode_bin(ec_dec *_this,unsigned _bits);














void ec_dec_update(ec_dec *_this,unsigned _fl,unsigned _fh,unsigned _ft);


int ec_dec_bit_logp(ec_dec *_this,unsigned _logp);









int ec_dec_icdf(ec_dec *_this,const unsigned char *_icdf,unsigned _ftb);







opus_uint32 ec_dec_uint(ec_dec *_this,opus_uint32 _ft);







opus_uint32 ec_dec_bits(ec_dec *_this,unsigned _ftb);

#endif
