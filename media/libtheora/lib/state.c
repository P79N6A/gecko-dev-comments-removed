
















#include <stdlib.h>
#include <string.h>
#include "internal.h"
#if defined(OC_X86_ASM)
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







static ptrdiff_t oc_sb_quad_top_left_frag(oc_sb_map_quad _sb_map[4],int _quadi){
  


  return _sb_map[_quadi][_quadi&_quadi<<1];
}










static void oc_sb_create_plane_mapping(oc_sb_map _sb_maps[],
 oc_sb_flags _sb_flags[],ptrdiff_t _frag0,int _hfrags,int _vfrags){
  







  static const int SB_MAP[4][4][2]={
    {{0,0},{0,1},{3,2},{3,3}},
    {{0,3},{0,2},{3,1},{3,0}},
    {{1,0},{1,3},{2,0},{2,3}},
    {{1,1},{1,2},{2,1},{2,2}}
  };
  ptrdiff_t  yfrag;
  unsigned   sbi;
  int        y;
  sbi=0;
  yfrag=_frag0;
  for(y=0;;y+=4){
    int imax;
    int x;
    

    imax=_vfrags-y;
    if(imax>4)imax=4;
    else if(imax<=0)break;
    for(x=0;;x+=4,sbi++){
      ptrdiff_t xfrag;
      int       jmax;
      int       quadi;
      int       i;
      

      jmax=_hfrags-x;
      if(jmax>4)jmax=4;
      else if(jmax<=0)break;
      
      memset(_sb_maps[sbi][0],0xFF,sizeof(_sb_maps[sbi]));
      
      xfrag=yfrag+x;
      for(i=0;i<imax;i++){
        int j;
        for(j=0;j<jmax;j++){
          _sb_maps[sbi][SB_MAP[i][j][0]][SB_MAP[i][j][1]]=xfrag+j;
        }
        xfrag+=_hfrags;
      }
      
      for(quadi=0;quadi<4;quadi++){
        _sb_flags[sbi].quad_valid|=
         (oc_sb_quad_top_left_frag(_sb_maps[sbi],quadi)>=0)<<quadi;
      }
    }
    yfrag+=_hfrags<<2;
  }
}







static void oc_mb_fill_ymapping(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane *_fplane,int _xfrag0,int _yfrag0){
  int i;
  int j;
  for(i=0;i<2;i++)for(j=0;j<2;j++){
    _mb_map[0][i<<1|j]=(_yfrag0+i)*(ptrdiff_t)_fplane->nhfrags+_xfrag0+j;
  }
}








static void oc_mb_fill_cmapping00(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane _fplanes[3],int _xfrag0,int _yfrag0){
  ptrdiff_t fragi;
  _xfrag0>>=1;
  _yfrag0>>=1;
  fragi=_yfrag0*(ptrdiff_t)_fplanes[1].nhfrags+_xfrag0;
  _mb_map[1][0]=fragi+_fplanes[1].froffset;
  _mb_map[2][0]=fragi+_fplanes[2].froffset;
}







static void oc_mb_fill_cmapping01(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane _fplanes[3],int _xfrag0,int _yfrag0){
  ptrdiff_t fragi;
  int       j;
  _yfrag0>>=1;
  fragi=_yfrag0*(ptrdiff_t)_fplanes[1].nhfrags+_xfrag0;
  for(j=0;j<2;j++){
    _mb_map[1][j]=fragi+_fplanes[1].froffset;
    _mb_map[2][j]=fragi+_fplanes[2].froffset;
    fragi++;
  }
}







static void oc_mb_fill_cmapping10(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane _fplanes[3],int _xfrag0,int _yfrag0){
  ptrdiff_t fragi;
  int       i;
  _xfrag0>>=1;
  fragi=_yfrag0*(ptrdiff_t)_fplanes[1].nhfrags+_xfrag0;
  for(i=0;i<2;i++){
    _mb_map[1][i<<1]=fragi+_fplanes[1].froffset;
    _mb_map[2][i<<1]=fragi+_fplanes[2].froffset;
    fragi+=_fplanes[1].nhfrags;
  }
}






static void oc_mb_fill_cmapping11(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane _fplanes[3]){
  int k;
  for(k=0;k<4;k++){
    _mb_map[1][k]=_mb_map[0][k]+_fplanes[1].froffset;
    _mb_map[2][k]=_mb_map[0][k]+_fplanes[2].froffset;
  }
}







typedef void (*oc_mb_fill_cmapping_func)(oc_mb_map_plane _mb_map[3],
 const oc_fragment_plane _fplanes[3],int _xfrag0,int _yfrag0);



static const oc_mb_fill_cmapping_func OC_MB_FILL_CMAPPING_TABLE[4]={
  oc_mb_fill_cmapping00,
  oc_mb_fill_cmapping01,
  oc_mb_fill_cmapping10,
  (oc_mb_fill_cmapping_func)oc_mb_fill_cmapping11
};








static void oc_mb_create_mapping(oc_mb_map _mb_maps[],
 signed char _mb_modes[],const oc_fragment_plane _fplanes[3],int _pixel_fmt){
  oc_mb_fill_cmapping_func  mb_fill_cmapping;
  unsigned                  sbi;
  int                       y;
  mb_fill_cmapping=OC_MB_FILL_CMAPPING_TABLE[_pixel_fmt];
  
  for(sbi=y=0;y<_fplanes[0].nvfrags;y+=4){
    int x;
    for(x=0;x<_fplanes[0].nhfrags;x+=4,sbi++){
      int ymb;
      
      for(ymb=0;ymb<2;ymb++){
        int xmb;
        for(xmb=0;xmb<2;xmb++){
          unsigned mbi;
          int      mbx;
          int      mby;
          mbi=sbi<<2|OC_MB_MAP[ymb][xmb];
          mbx=x|xmb<<1;
          mby=y|ymb<<1;
          
          memset(_mb_maps[mbi],0xFF,sizeof(_mb_maps[mbi]));
          
          if(mbx>=_fplanes[0].nhfrags||mby>=_fplanes[0].nvfrags){
            _mb_modes[mbi]=OC_MODE_INVALID;
            continue;
          }
          
          oc_mb_fill_ymapping(_mb_maps[mbi],_fplanes,mbx,mby);
          
          (*mb_fill_cmapping)(_mb_maps[mbi],_fplanes,mbx,mby);
        }
      }
    }
  }
}




static void oc_state_border_init(oc_theora_state *_state){
  oc_fragment       *frag;
  oc_fragment       *yfrag_end;
  oc_fragment       *xfrag_end;
  oc_fragment_plane *fplane;
  int                crop_x0;
  int                crop_y0;
  int                crop_xf;
  int                crop_yf;
  int                pli;
  int                y;
  int                x;
  


  


  _state->nborders=0;
  yfrag_end=frag=_state->frags;
  for(pli=0;pli<3;pli++){
    fplane=_state->fplanes+pli;
    
    crop_x0=_state->info.pic_x;
    crop_xf=_state->info.pic_x+_state->info.pic_width;
    crop_y0=_state->info.pic_y;
    crop_yf=_state->info.pic_y+_state->info.pic_height;
    if(pli>0){
      if(!(_state->info.pixel_fmt&1)){
        crop_x0=crop_x0>>1;
        crop_xf=crop_xf+1>>1;
      }
      if(!(_state->info.pixel_fmt&2)){
        crop_y0=crop_y0>>1;
        crop_yf=crop_yf+1>>1;
      }
    }
    y=0;
    for(yfrag_end+=fplane->nfrags;frag<yfrag_end;y+=8){
      x=0;
      for(xfrag_end=frag+fplane->nhfrags;frag<xfrag_end;frag++,x+=8){
        

        



        if(x+8<=crop_x0||crop_xf<=x||y+8<=crop_y0||crop_yf<=y||
         crop_x0>=crop_xf||crop_y0>=crop_yf){
          frag->invalid=1;
        }
        
        else if(x<crop_x0&&crop_x0<x+8||x<crop_xf&&crop_xf<x+8||
         y<crop_y0&&crop_y0<y+8||y<crop_yf&&crop_yf<y+8){
          ogg_int64_t mask;
          int         npixels;
          int         i;
          mask=npixels=0;
          for(i=0;i<8;i++){
            int j;
            for(j=0;j<8;j++){
              if(x+j>=crop_x0&&x+j<crop_xf&&y+i>=crop_y0&&y+i<crop_yf){
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
            frag->borderi=i;
            break;
          }
        }
        else frag->borderi=-1;
      }
    }
  }
}

static int oc_state_frarray_init(oc_theora_state *_state){
  int       yhfrags;
  int       yvfrags;
  int       chfrags;
  int       cvfrags;
  ptrdiff_t yfrags;
  ptrdiff_t cfrags;
  ptrdiff_t nfrags;
  unsigned  yhsbs;
  unsigned  yvsbs;
  unsigned  chsbs;
  unsigned  cvsbs;
  unsigned  ysbs;
  unsigned  csbs;
  unsigned  nsbs;
  size_t    nmbs;
  int       hdec;
  int       vdec;
  int       pli;
  
  
  yhfrags=_state->info.frame_width>>3;
  yvfrags=_state->info.frame_height>>3;
  hdec=!(_state->info.pixel_fmt&1);
  vdec=!(_state->info.pixel_fmt&2);
  chfrags=yhfrags+hdec>>hdec;
  cvfrags=yvfrags+vdec>>vdec;
  yfrags=yhfrags*(ptrdiff_t)yvfrags;
  cfrags=chfrags*(ptrdiff_t)cvfrags;
  nfrags=yfrags+2*cfrags;
  
  yhsbs=yhfrags+3>>2;
  yvsbs=yvfrags+3>>2;
  chsbs=chfrags+3>>2;
  cvsbs=cvfrags+3>>2;
  ysbs=yhsbs*yvsbs;
  csbs=chsbs*cvsbs;
  nsbs=ysbs+2*csbs;
  nmbs=(size_t)ysbs<<2;
  








  if(yfrags/yhfrags!=yvfrags||2*cfrags<cfrags||nfrags<yfrags||
   ysbs/yhsbs!=yvsbs||2*csbs<csbs||nsbs<ysbs||nmbs>>2!=ysbs){
    return TH_EIMPL;
  }
  
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
  _state->frags=_ogg_calloc(nfrags,sizeof(*_state->frags));
  _state->frag_mvs=_ogg_malloc(nfrags*sizeof(*_state->frag_mvs));
  _state->nsbs=nsbs;
  _state->sb_maps=_ogg_malloc(nsbs*sizeof(*_state->sb_maps));
  _state->sb_flags=_ogg_calloc(nsbs,sizeof(*_state->sb_flags));
  _state->nhmbs=yhsbs<<1;
  _state->nvmbs=yvsbs<<1;
  _state->nmbs=nmbs;
  _state->mb_maps=_ogg_calloc(nmbs,sizeof(*_state->mb_maps));
  _state->mb_modes=_ogg_calloc(nmbs,sizeof(*_state->mb_modes));
  _state->coded_fragis=_ogg_malloc(nfrags*sizeof(*_state->coded_fragis));
  if(_state->frags==NULL||_state->frag_mvs==NULL||_state->sb_maps==NULL||
   _state->sb_flags==NULL||_state->mb_maps==NULL||_state->mb_modes==NULL||
   _state->coded_fragis==NULL){
    return TH_EFAULT;
  }
  
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    fplane=_state->fplanes+pli;
    oc_sb_create_plane_mapping(_state->sb_maps+fplane->sboffset,
     _state->sb_flags+fplane->sboffset,fplane->froffset,
     fplane->nhfrags,fplane->nvfrags);
  }
  
  oc_mb_create_mapping(_state->mb_maps,_state->mb_modes,
   _state->fplanes,_state->info.pixel_fmt);
  
  oc_state_border_init(_state);
  return 0;
}

static void oc_state_frarray_clear(oc_theora_state *_state){
  _ogg_free(_state->coded_fragis);
  _ogg_free(_state->mb_modes);
  _ogg_free(_state->mb_maps);
  _ogg_free(_state->sb_flags);
  _ogg_free(_state->sb_maps);
  _ogg_free(_state->frag_mvs);
  _ogg_free(_state->frags);
}








static int oc_state_ref_bufs_init(oc_theora_state *_state,int _nrefs){
  th_info       *info;
  unsigned char *ref_frame_data;
  size_t         ref_frame_data_sz;
  size_t         ref_frame_sz;
  size_t         yplane_sz;
  size_t         cplane_sz;
  int            yhstride;
  int            yheight;
  int            chstride;
  int            cheight;
  ptrdiff_t      yoffset;
  ptrdiff_t      coffset;
  ptrdiff_t     *frag_buf_offs;
  ptrdiff_t      fragi;
  int            hdec;
  int            vdec;
  int            rfi;
  int            pli;
  if(_nrefs<3||_nrefs>4)return TH_EINVAL;
  info=&_state->info;
  
  hdec=!(info->pixel_fmt&1);
  vdec=!(info->pixel_fmt&2);
  yhstride=info->frame_width+2*OC_UMV_PADDING;
  yheight=info->frame_height+2*OC_UMV_PADDING;
  chstride=yhstride>>hdec;
  cheight=yheight>>vdec;
  yplane_sz=yhstride*(size_t)yheight;
  cplane_sz=chstride*(size_t)cheight;
  yoffset=OC_UMV_PADDING+OC_UMV_PADDING*(ptrdiff_t)yhstride;
  coffset=(OC_UMV_PADDING>>hdec)+(OC_UMV_PADDING>>vdec)*(ptrdiff_t)chstride;
  ref_frame_sz=yplane_sz+2*cplane_sz;
  ref_frame_data_sz=_nrefs*ref_frame_sz;
  

  if(yplane_sz/yhstride!=yheight||2*cplane_sz<cplane_sz||
   ref_frame_sz<yplane_sz||ref_frame_data_sz/_nrefs!=ref_frame_sz){
    return TH_EIMPL;
  }
  ref_frame_data=_ogg_malloc(ref_frame_data_sz);
  frag_buf_offs=_state->frag_buf_offs=
   _ogg_malloc(_state->nfrags*sizeof(*frag_buf_offs));
  if(ref_frame_data==NULL||frag_buf_offs==NULL){
    _ogg_free(frag_buf_offs);
    _ogg_free(ref_frame_data);
    return TH_EFAULT;
  }
  
  _state->ref_frame_bufs[0][0].width=info->frame_width;
  _state->ref_frame_bufs[0][0].height=info->frame_height;
  _state->ref_frame_bufs[0][0].stride=yhstride;
  _state->ref_frame_bufs[0][1].width=_state->ref_frame_bufs[0][2].width=
   info->frame_width>>hdec;
  _state->ref_frame_bufs[0][1].height=_state->ref_frame_bufs[0][2].height=
   info->frame_height>>vdec;
  _state->ref_frame_bufs[0][1].stride=_state->ref_frame_bufs[0][2].stride=
   chstride;
  for(rfi=1;rfi<_nrefs;rfi++){
    memcpy(_state->ref_frame_bufs[rfi],_state->ref_frame_bufs[0],
     sizeof(_state->ref_frame_bufs[0]));
  }
  
  for(rfi=0;rfi<_nrefs;rfi++){
    _state->ref_frame_data[rfi]=ref_frame_data;
    _state->ref_frame_bufs[rfi][0].data=ref_frame_data+yoffset;
    ref_frame_data+=yplane_sz;
    _state->ref_frame_bufs[rfi][1].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    _state->ref_frame_bufs[rfi][2].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    


    oc_ycbcr_buffer_flip(_state->ref_frame_bufs[rfi],
     _state->ref_frame_bufs[rfi]);
  }
  _state->ref_ystride[0]=-yhstride;
  _state->ref_ystride[1]=_state->ref_ystride[2]=-chstride;
  
  ref_frame_data=_state->ref_frame_data[0];
  fragi=0;
  for(pli=0;pli<3;pli++){
    th_img_plane      *iplane;
    oc_fragment_plane *fplane;
    unsigned char     *vpix;
    ptrdiff_t          stride;
    ptrdiff_t          vfragi_end;
    int                nhfrags;
    iplane=_state->ref_frame_bufs[0]+pli;
    fplane=_state->fplanes+pli;
    vpix=iplane->data;
    vfragi_end=fplane->froffset+fplane->nfrags;
    nhfrags=fplane->nhfrags;
    stride=iplane->stride;
    while(fragi<vfragi_end){
      ptrdiff_t      hfragi_end;
      unsigned char *hpix;
      hpix=vpix;
      for(hfragi_end=fragi+nhfrags;fragi<hfragi_end;fragi++){
        frag_buf_offs[fragi]=hpix-ref_frame_data;
        hpix+=8;
      }
      vpix+=stride<<3;
    }
  }
  
  _state->ref_frame_idx[OC_FRAME_GOLD]=
   _state->ref_frame_idx[OC_FRAME_PREV]=
   _state->ref_frame_idx[OC_FRAME_SELF]=-1;
  _state->ref_frame_idx[OC_FRAME_IO]=_nrefs>3?3:-1;
  return 0;
}

static void oc_state_ref_bufs_clear(oc_theora_state *_state){
  _ogg_free(_state->frag_buf_offs);
  _ogg_free(_state->ref_frame_data[0]);
}


void oc_state_vtable_init_c(oc_theora_state *_state){
  _state->opt_vtable.frag_copy=oc_frag_copy_c;
  _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_c;
  _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_c;
  _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_c;
  _state->opt_vtable.idct8x8=oc_idct8x8_c;
  _state->opt_vtable.state_frag_recon=oc_state_frag_recon_c;
  _state->opt_vtable.state_frag_copy_list=oc_state_frag_copy_list_c;
  _state->opt_vtable.state_loop_filter_frag_rows=
   oc_state_loop_filter_frag_rows_c;
  _state->opt_vtable.restore_fpu=oc_restore_fpu_c;
  _state->opt_data.dct_fzig_zag=OC_FZIG_ZAG;
}


void oc_state_vtable_init(oc_theora_state *_state){
#if defined(OC_X86_ASM)
  oc_state_vtable_init_x86(_state);
#else
  oc_state_vtable_init_c(_state);
#endif
}


int oc_state_init(oc_theora_state *_state,const th_info *_info,int _nrefs){
  int ret;
  
  if(_info==NULL)return TH_EFAULT;
  








  if((_info->frame_width&0xF)||(_info->frame_height&0xF)||
   _info->frame_width<=0||_info->frame_width>=0x100000||
   _info->frame_height<=0||_info->frame_height>=0x100000||
   _info->pic_x+_info->pic_width>_info->frame_width||
   _info->pic_y+_info->pic_height>_info->frame_height||
   _info->pic_x>255||_info->frame_height-_info->pic_height-_info->pic_y>255||
   




   _info->colorspace<0||_info->colorspace>=TH_CS_NSPACES||
   _info->pixel_fmt<0||_info->pixel_fmt>=TH_PF_NFORMATS){
    return TH_EINVAL;
  }
  memset(_state,0,sizeof(*_state));
  memcpy(&_state->info,_info,sizeof(*_info));
  

  _state->info.pic_y=_info->frame_height-_info->pic_height-_info->pic_y;
  _state->frame_type=OC_UNKWN_FRAME;
  oc_state_vtable_init(_state);
  ret=oc_state_frarray_init(_state);
  if(ret>=0)ret=oc_state_ref_bufs_init(_state,_nrefs);
  if(ret<0){
    oc_state_frarray_clear(_state);
    return ret;
  }
  

  if(_info->keyframe_granule_shift<0||_info->keyframe_granule_shift>31){
    _state->info.keyframe_granule_shift=31;
  }
  _state->keyframe_num=0;
  _state->curframe_num=-1;
  



  _state->granpos_bias=TH_VERSION_CHECK(_info,3,2,1);
  return 0;
}

void oc_state_clear(oc_theora_state *_state){
  oc_state_ref_bufs_clear(_state);
  oc_state_frarray_clear(_state);
}










void oc_state_borders_fill_rows(oc_theora_state *_state,int _refi,int _pli,
 int _y0,int _yend){
  th_img_plane  *iplane;
  unsigned char *apix;
  unsigned char *bpix;
  unsigned char *epix;
  int            stride;
  int            hpadding;
  hpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&1));
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  stride=iplane->stride;
  apix=iplane->data+_y0*(ptrdiff_t)stride;
  bpix=apix+iplane->width-1;
  epix=iplane->data+_yend*(ptrdiff_t)stride;
  
  while(apix!=epix){
    memset(apix-hpadding,apix[0],hpadding);
    memset(bpix+1,bpix[0],hpadding);
    apix+=stride;
    bpix+=stride;
  }
}







void oc_state_borders_fill_caps(oc_theora_state *_state,int _refi,int _pli){
  th_img_plane  *iplane;
  unsigned char *apix;
  unsigned char *bpix;
  unsigned char *epix;
  int            stride;
  int            hpadding;
  int            vpadding;
  int            fullw;
  hpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&1));
  vpadding=OC_UMV_PADDING>>(_pli!=0&&!(_state->info.pixel_fmt&2));
  iplane=_state->ref_frame_bufs[_refi]+_pli;
  stride=iplane->stride;
  fullw=iplane->width+(hpadding<<1);
  apix=iplane->data-hpadding;
  bpix=iplane->data+(iplane->height-1)*(ptrdiff_t)stride-hpadding;
  epix=apix-stride*(ptrdiff_t)vpadding;
  while(apix!=epix){
    memcpy(apix-stride,apix,fullw);
    memcpy(bpix+stride,bpix,fullw);
    apix-=stride;
    bpix+=stride;
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











int oc_state_get_mv_offsets(const oc_theora_state *_state,int _offsets[2],
 int _pli,int _dx,int _dy){
  













#if 0
  
  int ystride;
  int xprec;
  int yprec;
  int xfrac;
  int yfrac;
  int offs;
  ystride=_state->ref_ystride[_pli];
  

  xprec=1+(_pli!=0&&!(_state->info.pixel_fmt&1));
  yprec=1+(_pli!=0&&!(_state->info.pixel_fmt&2));
  

  xfrac=OC_SIGNMASK(-(_dx&(xprec|1)));
  yfrac=OC_SIGNMASK(-(_dy&(yprec|1)));
  offs=(_dx>>xprec)+(_dy>>yprec)*ystride;
  if(xfrac||yfrac){
    int xmask;
    int ymask;
    xmask=OC_SIGNMASK(_dx);
    ymask=OC_SIGNMASK(_dy);
    yfrac&=ystride;
    _offsets[0]=offs-(xfrac&xmask)+(yfrac&ymask);
    _offsets[1]=offs-(xfrac&~xmask)+(yfrac&~ymask);
    return 2;
  }
  else{
    _offsets[0]=offs;
    return 1;
  }
#else
  

  static const signed char OC_MVMAP[2][64]={
    {
          -15,-15,-14,-14,-13,-13,-12,-12,-11,-11,-10,-10, -9, -9, -8,
       -8, -7, -7, -6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1,  0,
        0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
        8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15
    },
    {
           -7, -7, -7, -7, -6, -6, -6, -6, -5, -5, -5, -5, -4, -4, -4,
       -4, -3, -3, -3, -3, -2, -2, -2, -2, -1, -1, -1, -1,  0,  0,  0,
        0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
        4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7
    }
  };
  static const signed char OC_MVMAP2[2][64]={
    {
        -1, 0,-1,  0,-1, 0,-1,  0,-1, 0,-1,  0,-1, 0,-1,
      0,-1, 0,-1,  0,-1, 0,-1,  0,-1, 0,-1,  0,-1, 0,-1,
      0, 1, 0, 1,  0, 1, 0, 1,  0, 1, 0, 1,  0, 1, 0, 1,
      0, 1, 0, 1,  0, 1, 0, 1,  0, 1, 0, 1,  0, 1, 0, 1
    },
    {
        -1,-1,-1,  0,-1,-1,-1,  0,-1,-1,-1,  0,-1,-1,-1,
      0,-1,-1,-1,  0,-1,-1,-1,  0,-1,-1,-1,  0,-1,-1,-1,
      0, 1, 1, 1,  0, 1, 1, 1,  0, 1, 1, 1,  0, 1, 1, 1,
      0, 1, 1, 1,  0, 1, 1, 1,  0, 1, 1, 1,  0, 1, 1, 1
    }
  };
  int ystride;
  int qpx;
  int qpy;
  int mx;
  int my;
  int mx2;
  int my2;
  int offs;
  ystride=_state->ref_ystride[_pli];
  qpy=_pli!=0&&!(_state->info.pixel_fmt&2);
  my=OC_MVMAP[qpy][_dy+31];
  my2=OC_MVMAP2[qpy][_dy+31];
  qpx=_pli!=0&&!(_state->info.pixel_fmt&1);
  mx=OC_MVMAP[qpx][_dx+31];
  mx2=OC_MVMAP2[qpx][_dx+31];
  offs=my*ystride+mx;
  if(mx2||my2){
    _offsets[1]=offs+my2*ystride+mx2;
    _offsets[0]=offs;
    return 2;
  }
  _offsets[0]=offs;
  return 1;
#endif
}

void oc_state_frag_recon(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant){
  _state->opt_vtable.state_frag_recon(_state,_fragi,_pli,_dct_coeffs,
   _last_zzi,_dc_quant);
}

void oc_state_frag_recon_c(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant){
  unsigned char *dst;
  ptrdiff_t      frag_buf_off;
  int            ystride;
  int            mb_mode;
  
  
  if(_last_zzi<2){
    ogg_int16_t p;
    int         ci;
    

    p=(ogg_int16_t)(_dct_coeffs[0]*(ogg_int32_t)_dc_quant+15>>5);
    
    for(ci=0;ci<64;ci++)_dct_coeffs[ci]=p;
  }
  else{
    
    _dct_coeffs[0]=(ogg_int16_t)(_dct_coeffs[0]*(int)_dc_quant);
    oc_idct8x8(_state,_dct_coeffs,_last_zzi);
  }
  
  frag_buf_off=_state->frag_buf_offs[_fragi];
  mb_mode=_state->frags[_fragi].mb_mode;
  ystride=_state->ref_ystride[_pli];
  dst=_state->ref_frame_data[_state->ref_frame_idx[OC_FRAME_SELF]]+frag_buf_off;
  if(mb_mode==OC_MODE_INTRA)oc_frag_recon_intra(_state,dst,ystride,_dct_coeffs);
  else{
    const unsigned char *ref;
    int                  mvoffsets[2];
    ref=
     _state->ref_frame_data[_state->ref_frame_idx[OC_FRAME_FOR_MODE(mb_mode)]]
     +frag_buf_off;
    if(oc_state_get_mv_offsets(_state,mvoffsets,_pli,
     _state->frag_mvs[_fragi][0],_state->frag_mvs[_fragi][1])>1){
      oc_frag_recon_inter2(_state,
       dst,ref+mvoffsets[0],ref+mvoffsets[1],ystride,_dct_coeffs);
    }
    else oc_frag_recon_inter(_state,dst,ref+mvoffsets[0],ystride,_dct_coeffs);
  }
}








void oc_state_frag_copy_list(const oc_theora_state *_state,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
 int _dst_frame,int _src_frame,int _pli){
  _state->opt_vtable.state_frag_copy_list(_state,_fragis,_nfragis,_dst_frame,
   _src_frame,_pli);
}

void oc_state_frag_copy_list_c(const oc_theora_state *_state,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
 int _dst_frame,int _src_frame,int _pli){
  const ptrdiff_t     *frag_buf_offs;
  const unsigned char *src_frame_data;
  unsigned char       *dst_frame_data;
  ptrdiff_t            fragii;
  int                  ystride;
  dst_frame_data=_state->ref_frame_data[_state->ref_frame_idx[_dst_frame]];
  src_frame_data=_state->ref_frame_data[_state->ref_frame_idx[_src_frame]];
  ystride=_state->ref_ystride[_pli];
  frag_buf_offs=_state->frag_buf_offs;
  for(fragii=0;fragii<_nfragis;fragii++){
    ptrdiff_t frag_buf_off;
    frag_buf_off=frag_buf_offs[_fragis[fragii]];
    oc_frag_copy(_state,dst_frame_data+frag_buf_off,
     src_frame_data+frag_buf_off,ystride);
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
  int x;
  _pix-=_ystride*2;
  for(x=0;x<8;x++){
    int f;
    f=_pix[x]-_pix[_ystride*3+x]+3*(_pix[_ystride*2+x]-_pix[_ystride+x]);
    


    f=*(_bv+(f+4>>3));
    _pix[_ystride+x]=OC_CLAMP255(_pix[_ystride+x]+f);
    _pix[_ystride*2+x]=OC_CLAMP255(_pix[_ystride*2+x]-f);
  }
}




int oc_state_loop_filter_init(oc_theora_state *_state,int _bv[256]){
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









void oc_state_loop_filter_frag_rows(const oc_theora_state *_state,int _bv[256],
 int _refi,int _pli,int _fragy0,int _fragy_end){
  _state->opt_vtable.state_loop_filter_frag_rows(_state,_bv,_refi,_pli,
   _fragy0,_fragy_end);
}

void oc_state_loop_filter_frag_rows_c(const oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end){
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  const ptrdiff_t         *frag_buf_offs;
  unsigned char           *ref_frame_data;
  ptrdiff_t                fragi_top;
  ptrdiff_t                fragi_bot;
  ptrdiff_t                fragi0;
  ptrdiff_t                fragi0_end;
  int                      ystride;
  int                      nhfrags;
  _bv+=127;
  fplane=_state->fplanes+_pli;
  nhfrags=fplane->nhfrags;
  fragi_top=fplane->froffset;
  fragi_bot=fragi_top+fplane->nfrags;
  fragi0=fragi_top+_fragy0*(ptrdiff_t)nhfrags;
  fragi0_end=fragi0+(_fragy_end-_fragy0)*(ptrdiff_t)nhfrags;
  ystride=_state->ref_ystride[_pli];
  frags=_state->frags;
  frag_buf_offs=_state->frag_buf_offs;
  ref_frame_data=_state->ref_frame_data[_refi];
  




  while(fragi0<fragi0_end){
    ptrdiff_t fragi;
    ptrdiff_t fragi_end;
    fragi=fragi0;
    fragi_end=fragi+nhfrags;
    while(fragi<fragi_end){
      if(frags[fragi].coded){
        unsigned char *ref;
        ref=ref_frame_data+frag_buf_offs[fragi];
        if(fragi>fragi0)loop_filter_h(ref,ystride,_bv);
        if(fragi0>fragi_top)loop_filter_v(ref,ystride,_bv);
        if(fragi+1<fragi_end&&!frags[fragi+1].coded){
          loop_filter_h(ref+8,ystride,_bv);
        }
        if(fragi+nhfrags<fragi_bot&&!frags[fragi+nhfrags].coded){
          loop_filter_v(ref+(ystride<<3),ystride,_bv);
        }
      }
      fragi++;
    }
    fragi0+=nhfrags;
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
  image=(png_bytep *)oc_malloc_2d(height,6*width,sizeof(**image));
  if(image==NULL){
    fclose(fp);
    return TH_EFAULT;
  }
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
    default:break;
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
