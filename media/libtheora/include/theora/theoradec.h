



















#if !defined(_O_THEORA_THEORADEC_H_)
# define _O_THEORA_THEORADEC_H_ (1)
# include <stddef.h>
# include <ogg/ogg.h>
# include "codec.h"

#if defined(__cplusplus)
extern "C" {
#endif




















#define TH_DECCTL_GET_PPLEVEL_MAX (1)
















#define TH_DECCTL_SET_PPLEVEL (3)











#define TH_DECCTL_SET_GRANPOS (5)













#define TH_DECCTL_SET_STRIPE_CB (7)


#define TH_DECCTL_SET_TELEMETRY_MBMODE (9)

#define TH_DECCTL_SET_TELEMETRY_MV (11)

#define TH_DECCTL_SET_TELEMETRY_QI (13)

#define TH_DECCTL_SET_TELEMETRY_BITS (15)



































typedef void (*th_stripe_decoded_func)(void *_ctx,th_ycbcr_buffer _buf,
 int _yfrag0,int _yfrag_end);


typedef struct{
  

  void                   *ctx;
  
  th_stripe_decoded_func  stripe_decoded;
}th_stripe_callback;










typedef struct th_dec_ctx    th_dec_ctx;






typedef struct th_setup_info th_setup_info;
































































extern int th_decode_headerin(th_info *_info,th_comment *_tc,
 th_setup_info **_setup,ogg_packet *_op);






















extern th_dec_ctx *th_decode_alloc(const th_info *_info,
 const th_setup_info *_setup);





extern void th_setup_free(th_setup_info *_setup);








extern int th_decode_ctl(th_dec_ctx *_dec,int _req,void *_buf,
 size_t _buf_sz);





















extern int th_decode_packetin(th_dec_ctx *_dec,const ogg_packet *_op,
 ogg_int64_t *_granpos);















extern int th_decode_ycbcr_out(th_dec_ctx *_dec,
 th_ycbcr_buffer _ycbcr);


extern void th_decode_free(th_dec_ctx *_dec);





#if defined(__cplusplus)
}
#endif

#endif
