
















#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "quant.h"
#include "decint.h"

unsigned OC_DC_QUANT_MIN[2]={4<<2,8<<2};
unsigned OC_AC_QUANT_MIN[2]={2<<2,4<<2};














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
	
	ogg_uint32_t      q;
	int               qi_start;
	int               qi_end;
	int               ci;
	memcpy(base,_qinfo->qi_ranges[qti][pli].base_matrices[qri],
	       sizeof(base));

	qi_start=qi;
	if(qri==_qinfo->qi_ranges[qti][pli].nranges)
	  qi_end=qi+1;
	else 
	  qi_end=qi+_qinfo->qi_ranges[qti][pli].sizes[qri];
	
	
	for(;;){
	  
	  









	  
	  if(_pp_dc_scale!=NULL)
	    _pp_dc_scale[qi]=(int)((ogg_uint32_t)_qinfo->dc_scale[qi]*base[0]/160);

	  
	  q=((ogg_uint32_t)_qinfo->dc_scale[qi]*base[0]/100)<<2;
	  q=OC_CLAMPI(OC_DC_QUANT_MIN[qti],q,OC_QUANT_MAX);
	  stage[qi][0]=(ogg_uint16_t)q;
	  
	  
	  for(ci=1;ci<64;ci++){
	    q=((ogg_uint32_t)_qinfo->ac_scale[qi]*base[ci]/100)<<2;
	    q=OC_CLAMPI(OC_AC_QUANT_MIN[qti],q,OC_QUANT_MAX);
	    stage[qi][ci]=(ogg_uint16_t)q;
	  }
	  
	  if(++qi>=qi_end)break;
	  
	  
	  for(ci=0;ci<64;ci++){
	    base[ci]=(unsigned char)
	      ((2*((qi_end-qi)*_qinfo->qi_ranges[qti][pli].base_matrices[qri][ci]+
		   (qi-qi_start)*_qinfo->qi_ranges[qti][pli].base_matrices[qri+1][ci])
		+_qinfo->qi_ranges[qti][pli].sizes[qri])/
	       (2*_qinfo->qi_ranges[qti][pli].sizes[qri]));
	  }
	}
      }

      


      {
	int dupe = 0;
	int i,j;
	for(i=0;i<=qti;i++){
	  for(j=0;j<(i<qti?3:pli);j++){
	    if(!memcmp(stage,_dequant[i][j],sizeof(stage))){
	      dupe = 1;
	      break;
	    }
	  }
	  if(dupe)break;
	}
	if(dupe){
	  _dequant[qti][pli]=_dequant[i][j];
	}else{
	  memcpy(_dequant[qti][pli],stage,sizeof(stage));
	}
      }
    }
  }

#ifdef _TH_DEBUG_
  int i, j, k, l;
  
  for(i=0;i<2;i++){
    for(j=0;j<3;j++){
      for(k=0;k<64;k++){
	TH_DEBUG("quantizer table [%s][%s][Q%d] = {",
		 (i==0?"intra":"inter"),(j==0?"Y":(j==1?"U":"V")),k);
	for(l=0;l<64;l++){
	  if((l&7)==0)
	    TH_DEBUG("\n   ");
	  TH_DEBUG("%4d ",_dequant[i][j][k][l]);
	}
	TH_DEBUG("}\n");
      }
    }
  }
#endif

}
