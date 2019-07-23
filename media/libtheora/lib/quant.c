
















#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "quant.h"
#include "decint.h"

static const unsigned OC_DC_QUANT_MIN[2]={4<<2,8<<2};
static const unsigned OC_AC_QUANT_MIN[2]={2<<2,4<<2};














void oc_dequant_tables_init(ogg_uint16_t *_dequant[64][3][2],
 int _pp_dc_scale[64],const th_quant_info *_qinfo){
  
  int          qti;
  
  int          pli;
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    
    int qi;
    
    int qri;
    for(qi=0,qri=0;qri<=_qinfo->qi_ranges[qti][pli].nranges;qri++){
      th_quant_base base;
      ogg_uint32_t  q;
      int           qi_start;
      int           qi_end;
      memcpy(base,_qinfo->qi_ranges[qti][pli].base_matrices[qri],
       sizeof(base));
      qi_start=qi;
      if(qri==_qinfo->qi_ranges[qti][pli].nranges)qi_end=qi+1;
      else qi_end=qi+_qinfo->qi_ranges[qti][pli].sizes[qri];
      
      for(;;){
        ogg_uint32_t qfac;
        int          zzi;
        int          ci;
        








        qfac=(ogg_uint32_t)_qinfo->dc_scale[qi]*base[0];
        
        if(_pp_dc_scale!=NULL)_pp_dc_scale[qi]=(int)(qfac/160);
        
        q=(qfac/100)<<2;
        q=OC_CLAMPI(OC_DC_QUANT_MIN[qti],q,OC_QUANT_MAX);
        _dequant[qi][pli][qti][0]=(ogg_uint16_t)q;
        
        for(zzi=1;zzi<64;zzi++){
          q=((ogg_uint32_t)_qinfo->ac_scale[qi]*base[OC_FZIG_ZAG[zzi]]/100)<<2;
          q=OC_CLAMPI(OC_AC_QUANT_MIN[qti],q,OC_QUANT_MAX);
          _dequant[qi][pli][qti][zzi]=(ogg_uint16_t)q;
        }
        

        {
          int dupe;
          int qtj;
          int plj;
          dupe=0;
          for(qtj=0;qtj<=qti;qtj++){
            for(plj=0;plj<(qtj<qti?3:pli);plj++){
              if(!memcmp(_dequant[qi][pli][qti],_dequant[qi][plj][qtj],
               sizeof(oc_quant_table))){
                dupe=1;
                break;
              }
            }
            if(dupe)break;
          }
          if(dupe)_dequant[qi][pli][qti]=_dequant[qi][plj][qtj];
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
  }
}
