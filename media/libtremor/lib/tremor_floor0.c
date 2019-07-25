
















#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "block.h"

#define LSP_FRACBITS 14

typedef struct {
  long n;
  int ln;
  int  m;
  int *linearmap;

  vorbis_info_floor0 *vi;
  ogg_int32_t *lsp_look;

} vorbis_look_floor0;



#include "lsp_lookup.h"





static long ADJUST_SQRT2[2]={8192,5792};
STIN ogg_int32_t vorbis_invsqlook_i(long a,long e){
  long i=(a&0x7fff)>>(INVSQ_LOOKUP_I_SHIFT-1); 
  long d=a&INVSQ_LOOKUP_I_MASK;                              
  long val=INVSQ_LOOKUP_I[i]-                                
    ((INVSQ_LOOKUP_IDel[i]*d)>>INVSQ_LOOKUP_I_SHIFT);        
  val*=ADJUST_SQRT2[e&1];
  e=(e>>1)+21;
  return(val>>e);
}



STIN ogg_int32_t vorbis_fromdBlook_i(long a){
  int i=(-a)>>(12-FROMdB2_SHIFT);
  if(i<0) return 0x7fffffff;
  if(i>=(FROMdB_LOOKUP_SZ<<FROMdB_SHIFT))return 0;
  
  return FROMdB_LOOKUP[i>>FROMdB_SHIFT] * FROMdB2_LOOKUP[i&FROMdB2_MASK];
}



STIN ogg_int32_t vorbis_coslook_i(long a){
  int i=a>>COS_LOOKUP_I_SHIFT;
  int d=a&COS_LOOKUP_I_MASK;
  return COS_LOOKUP_I[i]- ((d*(COS_LOOKUP_I[i]-COS_LOOKUP_I[i+1]))>>
			   COS_LOOKUP_I_SHIFT);
}



STIN ogg_int32_t vorbis_coslook2_i(long a){
  a=a&0x1ffff;

  if(a>0x10000)a=0x20000-a;
  {               
    int i=a>>COS_LOOKUP_I_SHIFT;
    int d=a&COS_LOOKUP_I_MASK;
    a=((COS_LOOKUP_I[i]<<COS_LOOKUP_I_SHIFT)-
       d*(COS_LOOKUP_I[i]-COS_LOOKUP_I[i+1]))>>
      (COS_LOOKUP_I_SHIFT-LSP_FRACBITS+14);
  }
  
  return(a);
}

static const int barklook[28]={
  0,100,200,301,          405,516,635,766,
  912,1077,1263,1476,     1720,2003,2333,2721,
  3184,3742,4428,5285,    6376,7791,9662,12181,
  15624,20397,27087,36554
};


STIN ogg_int32_t toBARK(int n){
  int i;
  for(i=0;i<27;i++) 
    if(n>=barklook[i] && n<barklook[i+1])break;
  
  if(i==27){
    return 27<<15;
  }else{
    int gap=barklook[i+1]-barklook[i];
    int del=n-barklook[i];

    return((i<<15)+((del<<15)/gap));
  }
}

static const unsigned char MLOOP_1[64]={
   0,10,11,11, 12,12,12,12, 13,13,13,13, 13,13,13,13,
  14,14,14,14, 14,14,14,14, 14,14,14,14, 14,14,14,14,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
};

static const unsigned char MLOOP_2[64]={
  0,4,5,5, 6,6,6,6, 7,7,7,7, 7,7,7,7,
  8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
};

static const unsigned char MLOOP_3[8]={0,1,2,2,3,3,3,3};

void vorbis_lsp_to_curve(ogg_int32_t *curve,int *map,int n,int ln,
			 ogg_int32_t *lsp,int m,
			 ogg_int32_t amp,
			 ogg_int32_t ampoffset,
			 ogg_int32_t *icos){

  

  
  int i;
  int ampoffseti=ampoffset*4096;
  int ampi=amp;
  ogg_int32_t *ilsp=(ogg_int32_t *)alloca(m*sizeof(*ilsp));
  
  for(i=0;i<m;i++){
#ifndef _LOW_ACCURACY_
    ogg_int32_t val=MULT32(lsp[i],0x517cc2);
#else
    ogg_int32_t val=((lsp[i]>>10)*0x517d)>>14;
#endif

    
    if(val<0 || (val>>COS_LOOKUP_I_SHIFT)>=COS_LOOKUP_I_SZ){
      memset(curve,0,sizeof(*curve)*n);
      return;
    }

    ilsp[i]=vorbis_coslook_i(val);
  }

  i=0;
  while(i<n){
    int j,k=map[i];
    ogg_uint32_t pi=46341; 
    ogg_uint32_t qi=46341;
    ogg_int32_t qexp=0,shift;
    ogg_int32_t wi=icos[k];

#ifdef _V_LSP_MATH_ASM
    lsp_loop_asm(&qi,&pi,&qexp,ilsp,wi,m);

    pi=((pi*pi)>>16);
    qi=((qi*qi)>>16);
    
    if(m&1){
      qexp= qexp*2-28*((m+1)>>1)+m;	     
      pi*=(1<<14)-((wi*wi)>>14);
      qi+=pi>>14;     
    }else{
      qexp= qexp*2-13*m;
      
      pi*=(1<<14)-wi;
      qi*=(1<<14)+wi;
      
      qi=(qi+pi)>>14;
    }
    
    if(qi&0xffff0000){ 
      qi>>=1; qexp++; 
    }else
      lsp_norm_asm(&qi,&qexp);

#else

    j=1;
    if(m>1){
      qi*=labs(ilsp[0]-wi);
      pi*=labs(ilsp[1]-wi);

      for(j+=2;j<m;j+=2){
        if(!(shift=MLOOP_1[(pi|qi)>>25]))
          if(!(shift=MLOOP_2[(pi|qi)>>19]))
            shift=MLOOP_3[(pi|qi)>>16];
        qi=(qi>>shift)*labs(ilsp[j-1]-wi);
        pi=(pi>>shift)*labs(ilsp[j]-wi);
        qexp+=shift;
      }
    }
    if(!(shift=MLOOP_1[(pi|qi)>>25]))
      if(!(shift=MLOOP_2[(pi|qi)>>19]))
	shift=MLOOP_3[(pi|qi)>>16];

    

    if(m&1){
      
      
      qi=(qi>>shift)*labs(ilsp[j-1]-wi);
      pi=(pi>>shift)<<14;
      qexp+=shift;

      if(!(shift=MLOOP_1[(pi|qi)>>25]))
	if(!(shift=MLOOP_2[(pi|qi)>>19]))
	  shift=MLOOP_3[(pi|qi)>>16];
      
      pi>>=shift;
      qi>>=shift;
      qexp+=shift-14*((m+1)>>1);

      pi=((pi*pi)>>16);
      qi=((qi*qi)>>16);
      qexp=qexp*2+m;

      pi*=(1<<14)-((wi*wi)>>14);
      qi+=pi>>14;

    }else{
      

      

      
      pi>>=shift;
      qi>>=shift;
      qexp+=shift-7*m;

      pi=((pi*pi)>>16);
      qi=((qi*qi)>>16);
      qexp=qexp*2+m;
      
      pi*=(1<<14)-wi;
      qi*=(1<<14)+wi;
      qi=(qi+pi)>>14;
      
    }
    

    



    if(qi&0xffff0000){ 
      qi>>=1; qexp++; 
    }else
      while(qi && !(qi&0x8000)){ 
	qi<<=1; qexp--; 
      }

#endif

    amp=vorbis_fromdBlook_i(ampi*                     
			    vorbis_invsqlook_i(qi,qexp)- 
			                              
			    ampoffseti);              
    
#ifdef _LOW_ACCURACY_
    amp>>=9;
#endif
    curve[i]= MULT31_SHIFT15(curve[i],amp);
    while(map[++i]==k) curve[i]= MULT31_SHIFT15(curve[i],amp);
  }
}



static void floor0_free_info(vorbis_info_floor *i){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

static void floor0_free_look(vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  if(look){

    if(look->linearmap)_ogg_free(look->linearmap);
    if(look->lsp_look)_ogg_free(look->lsp_look);
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

static vorbis_info_floor *floor0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  int j;

  vorbis_info_floor0 *info=(vorbis_info_floor0 *)_ogg_malloc(sizeof(*info));
  info->order=oggpack_read(opb,8);
  info->rate=oggpack_read(opb,16);
  info->barkmap=oggpack_read(opb,16);
  info->ampbits=oggpack_read(opb,6);
  info->ampdB=oggpack_read(opb,8);
  info->numbooks=oggpack_read(opb,4)+1;
  
  if(info->order<1)goto err_out;
  if(info->rate<1)goto err_out;
  if(info->barkmap<1)goto err_out;
  if(info->numbooks<1)goto err_out;
    
  for(j=0;j<info->numbooks;j++){
    info->books[j]=oggpack_read(opb,8);
    if(info->books[j]<0 || info->books[j]>=ci->books)goto err_out;
    if(ci->book_param[info->books[j]]->maptype==0)goto err_out;
  }
  return(info);

 err_out:
  floor0_free_info(info);
  return(NULL);
}









static vorbis_look_floor *floor0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){
  int j;
  vorbis_info        *vi=vd->vi;
  codec_setup_info   *ci=(codec_setup_info *)vi->codec_setup;
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)_ogg_calloc(1,sizeof(*look));
  look->m=info->order;
  look->n=ci->blocksizes[mi->blockflag]/2;
  look->ln=info->barkmap;
  look->vi=info;

  





  look->linearmap=(int *)_ogg_malloc((look->n+1)*sizeof(*look->linearmap));
  for(j=0;j<look->n;j++){

    int val=(look->ln*
	     ((toBARK(info->rate/2*j/look->n)<<11)/toBARK(info->rate/2)))>>11;

    if(val>=look->ln)val=look->ln-1; 
    look->linearmap[j]=val;
  }
  look->linearmap[j]=-1;

  look->lsp_look=(ogg_int32_t *)_ogg_malloc(look->ln*sizeof(*look->lsp_look));
  for(j=0;j<look->ln;j++)
    look->lsp_look[j]=vorbis_coslook2_i(0x10000*j/look->ln);

  return look;
}

static void *floor0_inverse1(vorbis_block *vb,vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  int j,k;
  
  int ampraw=oggpack_read(&vb->opb,info->ampbits);
  if(ampraw>0){ 
    long maxval=(1<<info->ampbits)-1;
    int amp=((ampraw*info->ampdB)<<4)/maxval;
    int booknum=oggpack_read(&vb->opb,_ilog(info->numbooks));
    
    if(booknum!=-1 && booknum<info->numbooks){ 
      codec_setup_info  *ci=(codec_setup_info *)vb->vd->vi->codec_setup;
      codebook *b=ci->fullbooks+info->books[booknum];
      ogg_int32_t last=0;
      ogg_int32_t *lsp=(ogg_int32_t *)_vorbis_block_alloc(vb,sizeof(*lsp)*(look->m+1));
            
      if(vorbis_book_decodev_set(b,lsp,&vb->opb,look->m,-24)==-1)goto eop;
      for(j=0;j<look->m;){
	for(k=0;j<look->m && k<b->dim;k++,j++)lsp[j]+=last;
	last=lsp[j-1];
      }
      
      lsp[look->m]=amp;
      return(lsp);
    }
  }
 eop:
  return(NULL);
}

static int floor0_inverse2(vorbis_block *vb,vorbis_look_floor *i,
			   void *memo,ogg_int32_t *out){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  
  if(memo){
    ogg_int32_t *lsp=(ogg_int32_t *)memo;
    ogg_int32_t amp=lsp[look->m];

    
    vorbis_lsp_to_curve(out,look->linearmap,look->n,look->ln,
			lsp,look->m,amp,info->ampdB,look->lsp_look);
    return(1);
  }
  memset(out,0,sizeof(*out)*look->n);
  return(0);
}


vorbis_func_floor floor0_exportbundle={
  &floor0_unpack,&floor0_look,&floor0_free_info,
  &floor0_free_look,&floor0_inverse1,&floor0_inverse2
};


