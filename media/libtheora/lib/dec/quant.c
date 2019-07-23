
















#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "quant.h"
#include "decint.h"

static const unsigned OC_DC_QUANT_MIN[2]={4<<2,8<<2};
static const unsigned OC_AC_QUANT_MIN[2]={2<<2,4<<2};














void oc_dequant_tables_init(oc_quant_table *_dequant[2][3],
 int _pp_dc_scale[64],const th_quant_info *_qinfo){
  
  int          qti;
  
  int          pli;
  for(qti=0;qti<2;qti++){
    for(pli=0;pli<3;pli++){
      oc_quant_tables stage;
      
      int qi;
      
      int qri;
      for(qi=0,qri=0; qri<=_qinfo->qi_ranges[qti][pli].nranges; qri++){
        th_quant_base base;
        ogg_uint32_t  q;
        int           qi_start;
        int           qi_end;
        int           ci;
        memcpy(base,_qinfo->qi_ranges[qti][pli].base_matrices[qri],
         sizeof(base));
        qi_start=qi;
        if(qri==_qinfo->qi_ranges[qti][pli].nranges)qi_end=qi+1;
        else qi_end=qi+_qinfo->qi_ranges[qti][pli].sizes[qri];
        
        for(;;){
          ogg_uint32_t qfac;
          








          qfac=(ogg_uint32_t)_qinfo->dc_scale[qi]*base[0];
          
          if(_pp_dc_scale!=NULL)_pp_dc_scale[qi]=(int)(qfac/160);
          
          q=(qfac/100)<<2;
          q=OC_CLAMPI(OC_DC_QUANT_MIN[qti],q,OC_QUANT_MAX);
          stage[qi][0]=(ogg_uint16_t)q;
          
          for(ci=1;ci<64;ci++){
            q=((ogg_uint32_t)_qinfo->ac_scale[qi]*base[ci]/100)<<2;
            q=OC_CLAMPI(OC_AC_QUANT_MIN[qti],q,OC_QUANT_MAX);
            stage[qi][ci]=(ogg_uint16_t)q;
          }
          if(++qi>=qi_end)break;
          
          for(ci=0;ci<64;ci++){
            base[ci]=(unsigned char)(
             (2*((qi_end-qi)*_qinfo->qi_ranges[qti][pli].base_matrices[qri][ci]+
             (qi-qi_start)*_qinfo->qi_ranges[qti][pli].base_matrices[qri+1][ci])
             +_qinfo->qi_ranges[qti][pli].sizes[qri])/
             (2*_qinfo->qi_ranges[qti][pli].sizes[qri]));
          }
        }
      }
      


      {
        int dupe;
        int qtj;
        int plj;
        dupe=0;
        for(qtj=0;qtj<=qti;qtj++){
          for(plj=0;plj<(qtj<qti?3:pli);plj++){
            if(!memcmp(stage,_dequant[qtj][plj],sizeof(stage))){
              dupe=1;
              break;
            }
          }
          if(dupe)break;
        }
        if(dupe)_dequant[qti][pli]=_dequant[qtj][plj];
        else memcpy(_dequant[qti][pli],stage,sizeof(stage));
      }
    }
  }
}
