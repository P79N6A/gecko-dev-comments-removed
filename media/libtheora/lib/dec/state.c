
















#include <stdlib.h>
#include <string.h>
#include "../internal.h"
#include "idct.h"
#if defined(USE_ASM)
#if defined(_MSC_VER)
# include "x86_vc/x86int.h"
#else
# include "x86/x86int.h"
#endif
#endif
#if defined(OC_DUMP_IMAGES)
# include <stdio.h>
# include "png.h"
#endif

void oc_restore_fpu(const oc_theora_state *_state){
  _state->opt_vtable.restore_fpu();
}

void oc_restore_fpu_c(void){}







static int oc_sb_quad_top_left_frag(const oc_sb *_sb,int _quadi){
  


  return _sb->map[_quadi][_quadi&_quadi<<1];
}









static void oc_sb_create_plane_mapping(oc_sb _sbs[],int _frag0,int _hfrags,
 int _vfrags){
  







  static const int SB_MAP[4][4][2]={
    {{0,0},{0,1},{3,2},{3,3}},
    {{0,3},{0,2},{3,1},{3,0}},
    {{1,0},{1,3},{2,0},{2,3}},
    {{1,1},{1,2},{2,1},{2,2}}
  };
  oc_sb *sb;
  int    yfrag;
  int    y;
  sb=_sbs;
  yfrag=_frag0;
  for(y=0;;y+=4){
    int imax;
    int x;
    

    imax=_vfrags-y;
    if(imax>4)imax=4;
    else if(imax<=0)break;
    for(x=0;;x+=4,sb++){
      int    xfrag;
      int    jmax;
      int    quadi;
      int    i;
      

      jmax=_hfrags-x;
      if(jmax>4)jmax=4;
      else if(jmax<=0)break;
      
      memset(sb->map[0],0xFF,sizeof(sb->map));
      
      xfrag=yfrag+x;
      for(i=0;i<imax;i++){
        int j;
        for(j=0;j<jmax;j++){
          sb->map[SB_MAP[i][j][0]][SB_MAP[i][j][1]]=xfrag+j;
        }
        xfrag+=_hfrags;
      }
      
      for(quadi=0;quadi<4;quadi++){
        sb->quad_valid|=(oc_sb_quad_top_left_frag(sb,quadi)>=0)<<quadi;
      }
    }
    yfrag+=_hfrags<<2;
  }
}







static void oc_mb_fill_ymapping(oc_mb *_mb,const oc_fragment_plane *_fplane,
 int _x,int _y){
  int i;
  for(i=0;i<2;i++){
    int j;
    if(_y+i>=_fplane->nvfrags)break;
    for(j=0;j<2;j++){
      if(_x+j>=_fplane->nhfrags)break;
      _mb->map[0][i<<1|j]=(_y+i)*_fplane->nhfrags+_x+j;
    }
  }
}







static void oc_mb_fill_cmapping00(oc_mb *_mb,
 const oc_fragment_plane _fplanes[3],int _x,int _y){
  int fragi;
  _x>>=1;
  _y>>=1;
  fragi=_y*_fplanes[1].nhfrags+_x;
  _mb->map[1][0]=fragi+_fplanes[1].froffset;
  _mb->map[2][0]=fragi+_fplanes[2].froffset;
}







static void oc_mb_fill_cmapping01(oc_mb *_mb,
 const oc_fragment_plane _fplanes[3],int _x,int _y){
  int fragi;
  int j;
  _y>>=1;
  fragi=_y*_fplanes[1].nhfrags+_x;
  for(j=0;j<2;j++){
    if(_x+j>=_fplanes[1].nhfrags)break;
    _mb->map[1][j]=fragi+_fplanes[1].froffset;
    _mb->map[2][j]=fragi+_fplanes[2].froffset;
    fragi++;
  }
}







static void oc_mb_fill_cmapping10(oc_mb *_mb,
 const oc_fragment_plane _fplanes[3],int _x,int _y){
  int fragi;
  int i;
  _x>>=1;
  fragi=_y*_fplanes[1].nhfrags+_x;
  for(i=0;i<2;i++){
    if(_y+i>=_fplanes[1].nvfrags)break;
    _mb->map[1][i<<1]=fragi+_fplanes[1].froffset;
    _mb->map[2][i<<1]=fragi+_fplanes[2].froffset;
    fragi+=_fplanes[1].nhfrags;
  }
}






static void oc_mb_fill_cmapping11(oc_mb *_mb,
 const oc_fragment_plane _fplanes[3]){
  int k;
  for(k=0;k<4;k++){
    if(_mb->map[0][k]>=0){
      _mb->map[1][k]=_mb->map[0][k]+_fplanes[1].froffset;
      _mb->map[2][k]=_mb->map[0][k]+_fplanes[2].froffset;
    }
  }
}







typedef void (*oc_mb_fill_cmapping_func)(oc_mb *_mb,
 const oc_fragment_plane _fplanes[3],int _xfrag0,int _yfrag0);



static const oc_mb_fill_cmapping_func OC_MB_FILL_CMAPPING_TABLE[4]={
  oc_mb_fill_cmapping00,
  oc_mb_fill_cmapping01,
  oc_mb_fill_cmapping10,
  (oc_mb_fill_cmapping_func)oc_mb_fill_cmapping11
};






static void oc_mb_create_mapping(oc_mb _mbs[],
 const oc_fragment_plane _fplanes[3],int _ctype){
  oc_mb_fill_cmapping_func  mb_fill_cmapping;
  oc_mb                    *mb0;
  int                       y;
  mb0=_mbs;
  mb_fill_cmapping=OC_MB_FILL_CMAPPING_TABLE[_ctype];
  
  for(y=0;y<_fplanes[0].nvfrags;y+=4){
    int x;
    for(x=0;x<_fplanes[0].nhfrags;x+=4,mb0+=4){
      int ymb;
      
      for(ymb=0;ymb<2;ymb++){
        int xmb;
        for(xmb=0;xmb<2;xmb++){
          oc_mb *mb;
          int    mbx;
          int    mby;
          mb=mb0+OC_MB_MAP[ymb][xmb];
          mbx=x|xmb<<1;
          mby=y|ymb<<1;
          mb->x=mbx<<3;
          mb->y=mby<<3;
          
          memset(mb->map,0xFF,sizeof(mb->map));
          
          if(mbx>=_fplanes[0].nhfrags||mby>=_fplanes[0].nvfrags){
            mb->mode=OC_MODE_INVALID;
            continue;
          }
          
          oc_mb_fill_ymapping(mb,_fplanes,mbx,mby);
          
          (*mb_fill_cmapping)(mb,_fplanes,mbx,mby);
        }
      }
    }
  }
}




static void oc_state_border_init(oc_theora_state *_state){
  typedef struct{
    int x0;
    int y0;
    int xf;
    int yf;
  }oc_crop_rect;
  oc_fragment       *frag;
  oc_fragment       *yfrag_end;
  oc_fragment       *xfrag_end;
  oc_fragment_plane *fplane;
  oc_crop_rect      *crop;
  oc_crop_rect       crop_rects[3];
  int                pli;
  int                y;
  int                x;
  


  


  _state->nborders=0;
  yfrag_end=frag=_state->frags;
  for(pli=0;pli<3;pli++){
    fplane=_state->fplanes+pli;
    crop=crop_rects+pli;
    
    crop->x0=_state->info.pic_x;
    crop->xf=_state->info.pic_x+_state->info.pic_width;
    crop->y0=_state->info.pic_y;
    crop->yf=_state->info.pic_y+_state->info.pic_height;
    if(pli>0){
      if(!(_state->info.pixel_fmt&1)){
        crop->x0=crop->x0>>1;
        crop->xf=crop->xf+1>>1;
      }
      if(!(_state->info.pixel_fmt&2)){
        crop->y0=crop->y0>>1;
        crop->yf=crop->yf+1>>1;
      }
    }
    y=0;
    for(yfrag_end+=fplane->nfrags;frag<yfrag_end;y+=8){
      x=0;
      for(xfrag_end=frag+fplane->nhfrags;frag<xfrag_end;frag++,x+=8){
        

        



        if(x+8<=crop->x0||crop->xf<=x||y+8<=crop->y0||crop->yf<=y||
         crop->x0>=crop->xf||crop->y0>=crop->yf){
          frag->invalid=1;
        }
        
        else if(x<crop->x0&&crop->x0<x+8||x<crop->xf&&crop->xf<x+8||
         y<crop->y0&&crop->y0<y+8||y<crop->yf&&crop->yf<y+8){
          ogg_int64_t mask;
          int         npixels;
          int         i;
          mask=npixels=0;
          for(i=0;i<8;i++){
            int j;
            for(j=0;j<8;j++){
              if(x+j>=crop->x0&&x+j<crop->xf&&y+i>=crop->y0&&y+i<crop->yf){
                mask|=(ogg_int64_t)1<<(i<<3|j);
                npixels++;
              }
            }
          }
          


          for(i=0;;i++){
            if(i>=_state->nborders){
              _state->nborders++;
              _state->borders[i].mask=mask;
              _state->borders[i].npixels=npixels;
            }
            else if(_state->borders[i].mask!=mask)continue;
            frag->border=_state->borders+i;
            break;
          }
        }
      }
    }
  }
}

static void oc_state_frarray_init(oc_theora_state *_state){
  int yhfrags;
  int yvfrags;
  int chfrags;
  int cvfrags;
  int yfrags;
  int cfrags;
  int nfrags;
  int yhsbs;
  int yvsbs;
  int chsbs;
  int cvsbs;
  int ysbs;
  int csbs;
  int nsbs;
  int nmbs;
  int hdec;
  int vdec;
  int pli;
  
  
  yhfrags=_state->info.frame_width>>3;
  yvfrags=_state->info.frame_height>>3;
  hdec=!(_state->info.pixel_fmt&1);
  vdec=!(_state->info.pixel_fmt&2);
  chfrags=yhfrags+hdec>>hdec;
  cvfrags=yvfrags+vdec>>vdec;
  yfrags=yhfrags*yvfrags;
  cfrags=chfrags*cvfrags;
  nfrags=yfrags+2*cfrags;
  
  yhsbs=yhfrags+3>>2;
  yvsbs=yvfrags+3>>2;
  chsbs=chfrags+3>>2;
  cvsbs=cvfrags+3>>2;
  ysbs=yhsbs*yvsbs;
  csbs=chsbs*cvsbs;
  nsbs=ysbs+2*csbs;
  nmbs=ysbs<<2;
  
  _state->fplanes[0].nhfrags=yhfrags;
  _state->fplanes[0].nvfrags=yvfrags;
  _state->fplanes[0].froffset=0;
  _state->fplanes[0].nfrags=yfrags;
  _state->fplanes[0].nhsbs=yhsbs;
  _state->fplanes[0].nvsbs=yvsbs;
  _state->fplanes[0].sboffset=0;
  _state->fplanes[0].nsbs=ysbs;
  _state->fplanes[1].nhfrags=_state->fplanes[2].nhfrags=chfrags;
  _state->fplanes[1].nvfrags=_state->fplanes[2].nvfrags=cvfrags;
  _state->fplanes[1].froffset=yfrags;
  _state->fplanes[2].froffset=yfrags+cfrags;
  _state->fplanes[1].nfrags=_state->fplanes[2].nfrags=cfrags;
  _state->fplanes[1].nhsbs=_state->fplanes[2].nhsbs=chsbs;
  _state->fplanes[1].nvsbs=_state->fplanes[2].nvsbs=cvsbs;
  _state->fplanes[1].sboffset=ysbs;
  _state->fplanes[2].sboffset=ysbs+csbs;
  _state->fplanes[1].nsbs=_state->fplanes[2].nsbs=csbs;
  _state->nfrags=nfrags;
  _state->frags=_ogg_calloc(nfrags,sizeof(oc_fragment));
  _state->nsbs=nsbs;
  _state->sbs=_ogg_calloc(nsbs,sizeof(oc_sb));
  _state->nhmbs=yhsbs<<1;
  _state->nvmbs=yvsbs<<1;
  _state->nmbs=nmbs;
  _state->mbs=_ogg_calloc(nmbs,sizeof(oc_mb));
  _state->coded_fragis=_ogg_malloc(nfrags*sizeof(_state->coded_fragis[0]));
  _state->uncoded_fragis=_state->coded_fragis+nfrags;
  _state->coded_mbis=_ogg_malloc(nmbs*sizeof(_state->coded_mbis[0]));
  
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    fplane=_state->fplanes+pli;
    oc_sb_create_plane_mapping(_state->sbs+fplane->sboffset,
     fplane->froffset,fplane->nhfrags,fplane->nvfrags);
  }
  
  oc_mb_create_mapping(_state->mbs,_state->fplanes,_state->info.pixel_fmt);
  
  oc_state_border_init(_state);
}

static void oc_state_frarray_clear(oc_theora_state *_state){
  _ogg_free(_state->coded_mbis);
  _ogg_free(_state->coded_fragis);
  _ogg_free(_state->mbs);
  _ogg_free(_state->sbs);
  _ogg_free(_state->frags);
}








static void oc_state_ref_bufs_init(oc_theora_state *_state){
  th_info   *info;
  unsigned char *ref_frame_data;
  size_t         yplane_sz;
  size_t         cplane_sz;
  int            yhstride;
  int            yvstride;
  int            chstride;
  int            cvstride;
  int            yoffset;
  int            coffset;
  int            rfi;
  info=&_state->info;
  
  yhstride=info->frame_width+2*OC_UMV_PADDING;
  yvstride=info->frame_height+2*OC_UMV_PADDING;
  chstride=yhstride>>!(info->pixel_fmt&1);
  cvstride=yvstride>>!(info->pixel_fmt&2);
  yplane_sz=(size_t)yhstride*yvstride;
  cplane_sz=(size_t)chstride*cvstride;
  yoffset=OC_UMV_PADDING+OC_UMV_PADDING*yhstride;
  coffset=(OC_UMV_PADDING>>!(info->pixel_fmt&1))+
   (OC_UMV_PADDING>>!(info->pixel_fmt&2))*chstride;
  _state->ref_frame_data=ref_frame_data=_ogg_malloc(3*(yplane_sz+2*cplane_sz));
  
  _state->ref_frame_bufs[0][0].width=info->frame_width;
  _state->ref_frame_bufs[0][0].height=info->frame_height;
  _state->ref_frame_bufs[0][0].stride=yhstride;
  _state->ref_frame_bufs[0][1].width=_state->ref_frame_bufs[0][2].width=
   info->frame_width>>!(info->pixel_fmt&1);
  _state->ref_frame_bufs[0][1].height=_state->ref_frame_bufs[0][2].height=
   info->frame_height>>!(info->pixel_fmt&2);
  _state->ref_frame_bufs[0][1].stride=_state->ref_frame_bufs[0][2].stride=
   chstride;
  memcpy(_state->ref_frame_bufs[1],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  memcpy(_state->ref_frame_bufs[2],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  
  for(rfi=0;rfi<3;rfi++){
    _state->ref_frame_bufs[rfi][0].data=ref_frame_data+yoffset;
    ref_frame_data+=yplane_sz;
    _state->ref_frame_bufs[rfi][1].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    _state->ref_frame_bufs[rfi][2].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    
    oc_ycbcr_buffer_flip(_state->ref_frame_bufs[rfi],
     _state->ref_frame_bufs[rfi]);
    
    oc_state_fill_buffer_ptrs(_state,rfi,_state->ref_frame_bufs[rfi]);
  }
  
  _state->ref_frame_idx[OC_FRAME_GOLD]=
   _state->ref_frame_idx[OC_FRAME_PREV]=
   _state->ref_frame_idx[OC_FRAME_SELF]=-1;
}

static void oc_state_ref_bufs_clear(oc_theora_state *_state){
  _ogg_free(_state->ref_frame_data);
}


void oc_state_vtable_init_c(oc_theora_state *_state){
  _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_c;
  _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_c;
  _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_c;
  _state->opt_vtable.state_frag_copy=oc_state_frag_copy_c;
  _state->opt_vtable.state_frag_recon=oc_state_frag_recon_c;
  _state->opt_vtable.state_loop_filter_frag_rows=
   oc_state_loop_filter_frag_rows_c;
  _state->opt_vtable.restore_fpu=oc_restore_fpu_c;
}


void oc_state_vtable_init(oc_theora_state *_state){
#if defined(USE_ASM)
  oc_state_vtable_init_x86(_state);
#else
  oc_state_vtable_init_c(_state);
#endif
}


int oc_state_init(oc_theora_state *_state,const th_info *_info){
  int old_granpos;
  
  if(_info==NULL)return TH_EFAULT;
  








  if((_info->frame_width&0xF)||(_info->frame_height&0xF)||
   _info->frame_width>=0x100000||_info->frame_height>=0x100000||
   _info->pic_x+_info->pic_width>_info->frame_width||
   _info->pic_y+_info->pic_height>_info->frame_height||
   _info->pic_x>255||
   _info->frame_height-_info->pic_height-_info->pic_y>255||
   _info->colorspace<0||_info->colorspace>=TH_CS_NSPACES||
   _info->pixel_fmt<0||_info->pixel_fmt>=TH_PF_NFORMATS){
    return TH_EINVAL;
  }
  memset(_state,0,sizeof(*_state));
  memcpy(&_state->info,_info,sizeof(*_info));
  

  _state->info.pic_y=_info->frame_height-_info->pic_height-_info->pic_y;
  _state->frame_type=OC_UNKWN_FRAME;
  oc_state_vtable_init(_state);
  oc_state_frarray_init(_state);
  oc_state_ref_bufs_init(_state);
  

  if(_info->keyframe_granule_shift<0||_info->keyframe_granule_shift>31){
    _state->info.keyframe_granule_shift=31;
  }
  _state->keyframe_num=1;
  _state->curframe_num=0;
  



  old_granpos=!TH_VERSION_CHECK(_info,3,2,1);
  _state->curframe_num-=old_granpos;
  _state->keyframe_num-=old_granpos;
  return 0;
}

void oc_state_clear(oc_theora_state *_state){
  oc_state_ref_bufs_clear(_state);
  oc_state_frarray_clear(_state);
}










void oc_state_borders_fill_rows(oc_theora_state *_state,int _refi,int _pli,
 int _y0,int _yend){
  th_img_plane *iplane;
  unsigned char    *apix;
  unsigned char    *bpix;
  unsigned char    *epix;
  int               hpadding;
  hpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&1));
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  apix=iplane->data+_y0*iplane->stride;
  bpix=apix+iplane->width-1;
  epix=iplane->data+_yend*iplane->stride;
  
  while(apix!=epix){
    memset(apix-hpadding,apix[0],hpadding);
    memset(bpix+1,bpix[0],hpadding);
    apix+=iplane->stride;
    bpix+=iplane->stride;
  }
}







void oc_state_borders_fill_caps(oc_theora_state *_state,int _refi,int _pli){
  th_img_plane *iplane;
  unsigned char    *apix;
  unsigned char    *bpix;
  unsigned char    *epix;
  int               hpadding;
  int               vpadding;
  int               fullw;
  hpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&1));
  vpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&2));
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  fullw=iplane->width+(hpadding<<1);
  apix=iplane->data-hpadding;
  bpix=iplane->data+(iplane->height-1)*iplane->stride-hpadding;
  epix=apix-iplane->stride*vpadding;
  while(apix!=epix){
    memcpy(apix-iplane->stride,apix,fullw);
    memcpy(bpix+iplane->stride,bpix,fullw);
    apix-=iplane->stride;
    bpix+=iplane->stride;
  }
}





void oc_state_borders_fill(oc_theora_state *_state,int _refi){
  int pli;
  for(pli=0;pli<3;pli++){
    oc_state_borders_fill_rows(_state,_refi,pli,0,
     _state->ref_frame_bufs[_refi][pli].height);
    oc_state_borders_fill_caps(_state,_refi,pli);
  }
}








void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 th_ycbcr_buffer _img){
  int pli;
  


  if(_buf_idx==OC_FRAME_IO){
     if(memcmp(_state->input,_img,sizeof(th_ycbcr_buffer))==0)return;
     memcpy(_state->input,_img,sizeof(th_ycbcr_buffer));
  }
  for(pli=0;pli<3;pli++){
    th_img_plane  *iplane;
    oc_fragment_plane *fplane;
    oc_fragment       *frag;
    oc_fragment       *vfrag_end;
    unsigned char     *vpix;
    iplane=&_img[pli];
    fplane=&_state->fplanes[pli];
    vpix=iplane->data;
    frag=_state->frags+fplane->froffset;
    vfrag_end=frag+fplane->nfrags;
    while(frag<vfrag_end){
      oc_fragment   *hfrag_end;
      unsigned char *hpix;
      hpix=vpix;
      for(hfrag_end=frag+fplane->nhfrags;frag<hfrag_end;frag++){
        frag->buffer[_buf_idx]=hpix;
        hpix+=8;
      }
      vpix+=iplane->stride<<3;
    }
  }
}






int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby){
  return ((_mbx&~1)<<1)+(_mby&~1)*_state->nhmbs+OC_MB_MAP[_mby&1][_mbx&1];
}












int oc_state_get_mv_offsets(oc_theora_state *_state,int _offsets[2],
 int _dx,int _dy,int _ystride,int _pli){
  int xprec;
  int yprec;
  int xfrac;
  int yfrac;
  













  

  xprec=1+(!(_state->info.pixel_fmt&1)&&_pli);
  yprec=1+(!(_state->info.pixel_fmt&2)&&_pli);
  

  xfrac=!!(_dx&(1<<xprec)-1);
  yfrac=!!(_dy&(1<<yprec)-1);
  _offsets[0]=(_dx>>xprec)+(_dy>>yprec)*_ystride;
  if(xfrac||yfrac){
    













    _offsets[1]=_offsets[0];
    _offsets[_dx>=0]+=xfrac;
    _offsets[_dy>=0]+=_ystride&-yfrac;
    return 2;
  }
  else return 1;
}

void oc_state_frag_recon(oc_theora_state *_state,oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_iquant,const ogg_uint16_t _ac_iquant[64]){
  _state->opt_vtable.state_frag_recon(_state,_frag,_pli,_dct_coeffs,
   _last_zzi,_ncoefs,_dc_iquant,_ac_iquant);
}

void oc_state_frag_recon_c(oc_theora_state *_state,oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_iquant, const ogg_uint16_t _ac_iquant[64]){
  ogg_int16_t dct_buf[64];
  ogg_int16_t res_buf[64];
  int dst_framei;
  int dst_ystride;
  int zzi;
  int ci;
  























  
  if(_last_zzi<2){
    ogg_int16_t p;
    

    p=(ogg_int16_t)((ogg_int32_t)_frag->dc*_dc_iquant+15>>5);
    
    for(ci=0;ci<64;ci++)res_buf[ci]=p;
  }
  else{
    
    dct_buf[0]=(ogg_int16_t)((ogg_int32_t)_frag->dc*_dc_iquant);
    for(zzi=1;zzi<_ncoefs;zzi++){
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      dct_buf[ci]=(ogg_int16_t)((ogg_int32_t)_dct_coeffs[zzi]*_ac_iquant[ci]);
    }
    

    if(_last_zzi<10){
      for(;zzi<10;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8_10_c(res_buf,dct_buf);
    }
    else{
      for(;zzi<64;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8_c(res_buf,dct_buf);
    }
  }
  
  dst_framei=_state->ref_frame_idx[OC_FRAME_SELF];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  
  if(_frag->mbmode==OC_MODE_INTRA){
    oc_frag_recon_intra(_state,_frag->buffer[dst_framei],dst_ystride,res_buf);
  }
  else{
    int ref_framei;
    int ref_ystride;
    int mvoffsets[2];
    ref_framei=_state->ref_frame_idx[OC_FRAME_FOR_MODE[_frag->mbmode]];
    ref_ystride=_state->ref_frame_bufs[ref_framei][_pli].stride;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_frag->mv[0],_frag->mv[1],
     ref_ystride,_pli)>1){
      oc_frag_recon_inter2(_state,_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffsets[0],ref_ystride,
       _frag->buffer[ref_framei]+mvoffsets[1],ref_ystride,res_buf);
    }
    else{
      oc_frag_recon_inter(_state,_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffsets[0],ref_ystride,res_buf);
    }
  }
  oc_restore_fpu(_state);
}








void oc_state_frag_copy(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli){
  _state->opt_vtable.state_frag_copy(_state,_fragis,_nfragis,_dst_frame,
   _src_frame,_pli);
}

void oc_state_frag_copy_c(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli){
  const int *fragi;
  const int *fragi_end;
  int        dst_framei;
  int        dst_ystride;
  int        src_framei;
  int        src_ystride;
  dst_framei=_state->ref_frame_idx[_dst_frame];
  src_framei=_state->ref_frame_idx[_src_frame];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  src_ystride=_state->ref_frame_bufs[src_framei][_pli].stride;
  fragi_end=_fragis+_nfragis;
  for(fragi=_fragis;fragi<fragi_end;fragi++){
    oc_fragment   *frag;
    unsigned char *dst;
    unsigned char *src;
    int            j;
    frag=_state->frags+*fragi;
    dst=frag->buffer[dst_framei];
    src=frag->buffer[src_framei];
    for(j=0;j<8;j++){
      memcpy(dst,src,sizeof(dst[0])*8);
      dst+=dst_ystride;
      src+=src_ystride;
    }
  }
}

static void loop_filter_h(unsigned char *_pix,int _ystride,int *_bv){
  int y;
  _pix-=2;
  for(y=0;y<8;y++){
    int f;
    f=_pix[0]-_pix[3]+3*(_pix[2]-_pix[1]);
    


    f=*(_bv+(f+4>>3));
    _pix[1]=OC_CLAMP255(_pix[1]+f);
    _pix[2]=OC_CLAMP255(_pix[2]-f);
    _pix+=_ystride;
  }
}

static void loop_filter_v(unsigned char *_pix,int _ystride,int *_bv){
  int y;
  _pix-=_ystride*2;
  for(y=0;y<8;y++){
    int f;
    f=_pix[0]-_pix[_ystride*3]+3*(_pix[_ystride*2]-_pix[_ystride]);
    


    f=*(_bv+(f+4>>3));
    _pix[_ystride]=OC_CLAMP255(_pix[_ystride]+f);
    _pix[_ystride*2]=OC_CLAMP255(_pix[_ystride*2]-f);
    _pix++;
  }
}




int oc_state_loop_filter_init(oc_theora_state *_state,int *_bv){
  int flimit;
  int i;
  flimit=_state->loop_filter_limits[_state->qis[0]];
  if(flimit==0)return 1;
  memset(_bv,0,sizeof(_bv[0])*256);
  for(i=0;i<flimit;i++){
    if(127-i-flimit>=0)_bv[127-i-flimit]=i-flimit;
    _bv[127-i]=-i;
    _bv[127+i]=i;
    if(127+i+flimit<256)_bv[127+i+flimit]=flimit-i;
  }
  return 0;
}









void oc_state_loop_filter_frag_rows(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end){
  _state->opt_vtable.state_loop_filter_frag_rows(_state,_bv,_refi,_pli,
   _fragy0,_fragy_end);
}

void oc_state_loop_filter_frag_rows_c(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end){
  th_img_plane      *iplane;
  oc_fragment_plane *fplane;
  oc_fragment       *frag_top;
  oc_fragment       *frag0;
  oc_fragment       *frag;
  oc_fragment       *frag_end;
  oc_fragment       *frag0_end;
  oc_fragment       *frag_bot;
  _bv+=127;
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  fplane=_state->fplanes+_pli;
  




  frag_top=_state->frags+fplane->froffset;
  frag0=frag_top+_fragy0*fplane->nhfrags;
  frag0_end=frag0+(_fragy_end-_fragy0)*fplane->nhfrags;
  frag_bot=_state->frags+fplane->froffset+fplane->nfrags;
  while(frag0<frag0_end){
    frag=frag0;
    frag_end=frag+fplane->nhfrags;
    while(frag<frag_end){
      if(frag->coded){
        if(frag>frag0){
          loop_filter_h(frag->buffer[_refi],iplane->stride,_bv);
        }
        if(frag0>frag_top){
          loop_filter_v(frag->buffer[_refi],iplane->stride,_bv);
        }
        if(frag+1<frag_end&&!(frag+1)->coded){
          loop_filter_h(frag->buffer[_refi]+8,iplane->stride,_bv);
        }
        if(frag+fplane->nhfrags<frag_bot&&!(frag+fplane->nhfrags)->coded){
          loop_filter_v((frag+fplane->nhfrags)->buffer[_refi],
           iplane->stride,_bv);
        }
      }
      frag++;
    }
    frag0+=fplane->nhfrags;
  }
}

#if defined(OC_DUMP_IMAGES)
int oc_state_dump_frame(const oc_theora_state *_state,int _frame,
 const char *_suf){
  
  png_structp    png;
  png_infop      info;
  png_bytep     *image;
  FILE          *fp;
  char           fname[16];
  unsigned char *y_row;
  unsigned char *u_row;
  unsigned char *v_row;
  unsigned char *y;
  unsigned char *u;
  unsigned char *v;
  ogg_int64_t    iframe;
  ogg_int64_t    pframe;
  int            y_stride;
  int            u_stride;
  int            v_stride;
  int            framei;
  int            width;
  int            height;
  int            imgi;
  int            imgj;
  width=_state->info.frame_width;
  height=_state->info.frame_height;
  iframe=_state->granpos>>_state->info.keyframe_granule_shift;
  pframe=_state->granpos-(iframe<<_state->info.keyframe_granule_shift);
  sprintf(fname,"%08i%s.png",(int)(iframe+pframe),_suf);
  fp=fopen(fname,"wb");
  if(fp==NULL)return TH_EFAULT;
  image=(png_bytep *)oc_malloc_2d(height,6*width,sizeof(image[0][0]));
  png=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(png==NULL){
    oc_free_2d(image);
    fclose(fp);
    return TH_EFAULT;
  }
  info=png_create_info_struct(png);
  if(info==NULL){
    png_destroy_write_struct(&png,NULL);
    oc_free_2d(image);
    fclose(fp);
    return TH_EFAULT;
  }
  if(setjmp(png_jmpbuf(png))){
    png_destroy_write_struct(&png,&info);
    oc_free_2d(image);
    fclose(fp);
    return TH_EFAULT;
  }
  framei=_state->ref_frame_idx[_frame];
  y_row=_state->ref_frame_bufs[framei][0].data;
  u_row=_state->ref_frame_bufs[framei][1].data;
  v_row=_state->ref_frame_bufs[framei][2].data;
  y_stride=_state->ref_frame_bufs[framei][0].stride;
  u_stride=_state->ref_frame_bufs[framei][1].stride;
  v_stride=_state->ref_frame_bufs[framei][2].stride;
  




  for(imgi=height;imgi-->0;){
    int dc;
    y=y_row;
    u=u_row;
    v=v_row;
    for(imgj=0;imgj<6*width;){
      float    yval;
      float    uval;
      float    vval;
      unsigned rval;
      unsigned gval;
      unsigned bval;
      
      yval=(*y-16)*(1.0F/219);
      uval=(*u-128)*(2*(1-0.114F)/224);
      vval=(*v-128)*(2*(1-0.299F)/224);
      rval=OC_CLAMPI(0,(int)(65535*(yval+vval)+0.5F),65535);
      gval=OC_CLAMPI(0,(int)(65535*(
       yval-uval*(0.114F/0.587F)-vval*(0.299F/0.587F))+0.5F),65535);
      bval=OC_CLAMPI(0,(int)(65535*(yval+uval)+0.5F),65535);
      image[imgi][imgj++]=(unsigned char)(rval>>8);
      image[imgi][imgj++]=(unsigned char)(rval&0xFF);
      image[imgi][imgj++]=(unsigned char)(gval>>8);
      image[imgi][imgj++]=(unsigned char)(gval&0xFF);
      image[imgi][imgj++]=(unsigned char)(bval>>8);
      image[imgi][imgj++]=(unsigned char)(bval&0xFF);
      dc=(y-y_row&1)|(_state->info.pixel_fmt&1);
      y++;
      u+=dc;
      v+=dc;
    }
    dc=-((height-1-imgi&1)|_state->info.pixel_fmt>>1);
    y_row+=y_stride;
    u_row+=dc&u_stride;
    v_row+=dc&v_stride;
  }
  png_init_io(png,fp);
  png_set_compression_level(png,Z_BEST_COMPRESSION);
  png_set_IHDR(png,info,width,height,16,PNG_COLOR_TYPE_RGB,
   PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
  switch(_state->info.colorspace){
    case TH_CS_ITU_REC_470M:{
      png_set_gAMA(png,info,2.2);
      png_set_cHRM_fixed(png,info,31006,31616,
       67000,32000,21000,71000,14000,8000);
    }break;
    case TH_CS_ITU_REC_470BG:{
      png_set_gAMA(png,info,2.67);
      png_set_cHRM_fixed(png,info,31271,32902,
       64000,33000,29000,60000,15000,6000);
    }break;
  }
  png_set_pHYs(png,info,_state->info.aspect_numerator,
   _state->info.aspect_denominator,0);
  png_set_rows(png,info,image);
  png_write_png(png,info,PNG_TRANSFORM_IDENTITY,NULL);
  png_write_end(png,info);
  png_destroy_write_struct(&png,&info);
  oc_free_2d(image);
  fclose(fp);
  return 0;
}
#endif



ogg_int64_t th_granule_frame(void *_encdec,ogg_int64_t _granpos){
  oc_theora_state *state;
  state=(oc_theora_state *)_encdec;
  if(_granpos>=0){
    ogg_int64_t iframe;
    ogg_int64_t pframe;
    iframe=_granpos>>state->info.keyframe_granule_shift;
    pframe=_granpos-(iframe<<state->info.keyframe_granule_shift);
    



    return iframe+pframe-TH_VERSION_CHECK(&state->info,3,2,1);
  }
  return -1;
}

double th_granule_time(void *_encdec,ogg_int64_t _granpos){
  oc_theora_state *state;
  state=(oc_theora_state *)_encdec;
  if(_granpos>=0){
    return (th_granule_frame(_encdec, _granpos)+1)*(
     (double)state->info.fps_denominator/state->info.fps_numerator);
  }
  return -1;
}
