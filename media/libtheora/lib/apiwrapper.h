
















#if !defined(_apiwrapper_H)
# define _apiwrapper_H (1)
# include <ogg/ogg.h>
# include <theora/theora.h>
# include "theora/theoradec.h"
# include "theora/theoraenc.h"
# include "internal.h"

typedef struct th_api_wrapper th_api_wrapper;
typedef struct th_api_info    th_api_info;





typedef void (*oc_setup_clear_func)(void *_ts);





struct th_api_wrapper{
  oc_setup_clear_func  clear;
  th_setup_info       *setup;
  th_dec_ctx          *decode;
  th_enc_ctx          *encode;
};

struct th_api_info{
  th_api_wrapper api;
  theora_info    info;
};


void oc_theora_info2th_info(th_info *_info,const theora_info *_ci);

#endif
