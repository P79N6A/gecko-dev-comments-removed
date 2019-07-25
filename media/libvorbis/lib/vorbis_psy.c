
















#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f
static const double stereo_threshholds[]={0.0, .5, 1.0, 1.5, 2.5, 4.5, 8.5, 16.5, 9e10};
static const double stereo_threshholds_limited[]={0.0, .5, 1.0, 1.5, 2.0, 2.5, 4.5, 8.5, 9e10};

vorbis_look_psy_global *_vp_global_look(vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  vorbis_look_psy_global *look=_ogg_calloc(1,sizeof(*look));

  look->channels=vi->channels;

  look->ampmax=-9999.;
  look->gi=gi;
  return(look);
}

void _vp_global_free(vorbis_look_psy_global *look){
  if(look){
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

void _vi_gpsy_free(vorbis_info_psy_global *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

static void min_curve(float *c,
                       float *c2){
  int i;
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
                       float *c2){
  int i;
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static float ***setup_tone_curves(float curveatt_dB[P_BANDS],float binHz,int n,
                                  float center_boost, float center_decay_rate){
  int i,j,k,m;
  float ath[EHMER_MAX];
  float workc[P_BANDS][P_LEVELS][EHMER_MAX];
  float athc[P_LEVELS][EHMER_MAX];
  float *brute_buffer=alloca(n*sizeof(*brute_buffer));

  float ***ret=_ogg_malloc(sizeof(*ret)*P_BANDS);

  memset(workc,0,sizeof(workc));

  for(i=0;i<P_BANDS;i++){
    



    

    int ath_offset=i*4;
    for(j=0;j<EHMER_MAX;j++){
      float min=999.;
      for(k=0;k<4;k++)
        if(j+k+ath_offset<MAX_ATH){
          if(min>ATH[j+k+ath_offset])min=ATH[j+k+ath_offset];
        }else{
          if(min>ATH[MAX_ATH-1])min=ATH[MAX_ATH-1];
        }
      ath[j]=min;
    }

    

    for(j=0;j<6;j++)
      memcpy(workc[i][j+2],tonemasks[i][j],EHMER_MAX*sizeof(*tonemasks[i][j]));
    memcpy(workc[i][0],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));
    memcpy(workc[i][1],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));

    
    for(j=0;j<P_LEVELS;j++){
      for(k=0;k<EHMER_MAX;k++){
        float adj=center_boost+abs(EHMER_OFFSET-k)*center_decay_rate;
        if(adj<0. && center_boost>0)adj=0.;
        if(adj>0. && center_boost<0)adj=0.;
        workc[i][j][k]+=adj;
      }
    }

    
    
    for(j=0;j<P_LEVELS;j++){
      attenuate_curve(workc[i][j],curveatt_dB[i]+100.-(j<2?2:j)*10.-P_LEVEL_0);
      memcpy(athc[j],ath,EHMER_MAX*sizeof(**athc));
      attenuate_curve(athc[j],+100.-j*10.f-P_LEVEL_0);
      max_curve(athc[j],workc[i][j]);
    }

    









    for(j=1;j<P_LEVELS;j++){
      min_curve(athc[j],athc[j-1]);
      min_curve(workc[i][j],athc[j]);
    }
  }

  for(i=0;i<P_BANDS;i++){
    int hi_curve,lo_curve,bin;
    ret[i]=_ogg_malloc(sizeof(**ret)*P_LEVELS);

    








    
    bin=floor(fromOC(i*.5)/binHz);
    lo_curve=  ceil(toOC(bin*binHz+1)*2);
    hi_curve=  floor(toOC((bin+1)*binHz)*2);
    if(lo_curve>i)lo_curve=i;
    if(lo_curve<0)lo_curve=0;
    if(hi_curve>=P_BANDS)hi_curve=P_BANDS-1;

    for(m=0;m<P_LEVELS;m++){
      ret[i][m]=_ogg_malloc(sizeof(***ret)*(EHMER_MAX+2));

      for(j=0;j<n;j++)brute_buffer[j]=999.;

      


      for(k=lo_curve;k<=hi_curve;k++){
        int l=0;

        for(j=0;j<EHMER_MAX;j++){
          int lo_bin= fromOC(j*.125+k*.5-2.0625)/binHz;
          int hi_bin= fromOC(j*.125+k*.5-1.9375)/binHz+1;

          if(lo_bin<0)lo_bin=0;
          if(lo_bin>n)lo_bin=n;
          if(lo_bin<l)l=lo_bin;
          if(hi_bin<0)hi_bin=0;
          if(hi_bin>n)hi_bin=n;

          for(;l<hi_bin && l<n;l++)
            if(brute_buffer[l]>workc[k][m][j])
              brute_buffer[l]=workc[k][m][j];
        }

        for(;l<n;l++)
          if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
            brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }

      
      if(i+1<P_BANDS){
        int l=0;
        k=i+1;
        for(j=0;j<EHMER_MAX;j++){
          int lo_bin= fromOC(j*.125+i*.5-2.0625)/binHz;
          int hi_bin= fromOC(j*.125+i*.5-1.9375)/binHz+1;

          if(lo_bin<0)lo_bin=0;
          if(lo_bin>n)lo_bin=n;
          if(lo_bin<l)l=lo_bin;
          if(hi_bin<0)hi_bin=0;
          if(hi_bin>n)hi_bin=n;

          for(;l<hi_bin && l<n;l++)
            if(brute_buffer[l]>workc[k][m][j])
              brute_buffer[l]=workc[k][m][j];
        }

        for(;l<n;l++)
          if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
            brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }


      for(j=0;j<EHMER_MAX;j++){
        int bin=fromOC(j*.125+i*.5-2.)/binHz;
        if(bin<0){
          ret[i][m][j+2]=-999.;
        }else{
          if(bin>=n){
            ret[i][m][j+2]=-999.;
          }else{
            ret[i][m][j+2]=brute_buffer[bin];
          }
        }
      }

      
      for(j=0;j<EHMER_OFFSET;j++)
        if(ret[i][m][j+2]>-200.f)break;
      ret[i][m][0]=j;

      for(j=EHMER_MAX-1;j>EHMER_OFFSET+1;j--)
        if(ret[i][m][j+2]>-200.f)
          break;
      ret[i][m][1]=j;

    }
  }

  return(ret);
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
                  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,lo=-99,hi=1;
  long maxoc;
  memset(p,0,sizeof(*p));

  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=rint(log(gi->eighth_octave_lines*8.f)/log(2.f))-1;

  p->firstoc=toOC(.25f*rate*.5/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n+.25f)*rate*.5/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;
  p->ath=_ogg_malloc(n*sizeof(*p->ath));

  p->octave=_ogg_malloc(n*sizeof(*p->octave));
  p->bark=_ogg_malloc(n*sizeof(*p->bark));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  
  p->m_val = 1.;
  if(rate < 26000) p->m_val = 0;
  else if(rate < 38000) p->m_val = .94;   
  else if(rate > 46000) p->m_val = 1.275; 

  

  for(i=0,j=0;i<MAX_ATH-1;i++){
    int endpos=rint(fromOC((i+1)*.125-2.)*2*n/rate);
    float base=ATH[i];
    if(j<endpos){
      float delta=(ATH[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
        p->ath[j]=base+100.;
        base+=delta;
      }
    }
  }

  for(;j<n;j++){
    p->ath[j]=p->ath[j-1];
  }

  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i);

    for(;lo+vi->noisewindowlomin<i &&
          toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);

    for(;hi<=n && (hi<i+vi->noisewindowhimin ||
          toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);

    p->bark[i]=((lo-1)<<16)+(hi-1);

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i+.25f)*.5*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=setup_tone_curves(vi->toneatt,rate*.5/n,n,
                                  vi->tone_centerboost,vi->tone_decay);

  
  p->noiseoffset=_ogg_malloc(P_NOISECURVES*sizeof(*p->noiseoffset));
  for(i=0;i<P_NOISECURVES;i++)
    p->noiseoffset[i]=_ogg_malloc(n*sizeof(**p->noiseoffset));

  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.;
    int inthalfoc;
    float del;

    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;

    for(j=0;j<P_NOISECURVES;j++)
      p->noiseoffset[j][i]=
        p->vi->noiseoff[j][inthalfoc]*(1.-del) +
        p->vi->noiseoff[j][inthalfoc+1]*del;

  }
#if 0
  {
    static int ls=0;
    _analysis_output_always("noiseoff0",ls,p->noiseoffset[0],n,1,0,0);
    _analysis_output_always("noiseoff1",ls,p->noiseoffset[1],n,1,0,0);
    _analysis_output_always("noiseoff2",ls++,p->noiseoffset[2],n,1,0,0);
  }
#endif
}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
        for(j=0;j<P_LEVELS;j++){
          _ogg_free(p->tonecurves[i][j]);
        }
        _ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    if(p->noiseoffset){
      for(i=0;i<P_NOISECURVES;i++){
        _ogg_free(p->noiseoffset[i]);
      }
      _ogg_free(p->noiseoffset);
    }
    memset(p,0,sizeof(*p));
  }
}


static void seed_curve(float *seed,
                       const float **curves,
                       float amp,
                       int oc, int n,
                       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset-P_LEVEL_0)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-EHMER_OFFSET)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_loop(vorbis_look_psy *p,
                      const float ***curves,
                      const float *f,
                      const float *flr,
                      float *seed,
                      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }

    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;

      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;

      seed_curve(seed,
                 curves[oc],
                 max,
                 p->octave[i]-p->firstoc,
                 p->total_octave_lines,
                 p->eighth_octave_lines,
                 dBoffset);
    }
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(*posstack));
  float *ampstack=alloca(n*sizeof(*ampstack));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
        if(seeds[i]<ampstack[stack-1]){
          posstack[stack]=i;
          ampstack[stack++]=seeds[i];
          break;
        }else{
          if(i<posstack[stack-1]+linesper){
            if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
               i<posstack[stack-2]+linesper){
              
              stack--;
              continue;
            }
          }
          posstack[stack]=i;
          ampstack[stack++]=seeds[i];
          break;

        }
      }
    }
  }

  


  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1; 

    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }

  


}


#include<stdio.h>
static void max_seeds(vorbis_look_psy *p,
                      float *seed,
                      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n); 

  pos=p->octave[0]-p->firstoc-(linesper>>1);

  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    if(minV>p->vi->tone_abs_limit)minV=p->vi->tone_abs_limit;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
        minV=seed[pos];
    }

    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }

  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }

}

static void bark_noise_hybridmp(int n,const long *b,
                                const float *f,
                                float *noise,
                                const float offset,
                                const int fixed){

  float *N=alloca(n*sizeof(*N));
  float *X=alloca(n*sizeof(*N));
  float *XX=alloca(n*sizeof(*N));
  float *Y=alloca(n*sizeof(*N));
  float *XY=alloca(n*sizeof(*N));

  float tN, tX, tXX, tY, tXY;
  int i;

  int lo, hi;
  float R=0.f;
  float A=0.f;
  float B=0.f;
  float D=1.f;
  float w, x, y;

  tN = tX = tXX = tY = tXY = 0.f;

  y = f[0] + offset;
  if (y < 1.f) y = 1.f;

  w = y * y * .5;

  tN += w;
  tX += w;
  tY += w * y;

  N[0] = tN;
  X[0] = tX;
  XX[0] = tXX;
  Y[0] = tY;
  XY[0] = tXY;

  for (i = 1, x = 1.f; i < n; i++, x += 1.f) {

    y = f[i] + offset;
    if (y < 1.f) y = 1.f;

    w = y * y;

    tN += w;
    tX += w * x;
    tXX += w * x * x;
    tY += w * y;
    tXY += w * x * y;

    N[i] = tN;
    X[i] = tX;
    XX[i] = tXX;
    Y[i] = tY;
    XY[i] = tXY;
  }

  for (i = 0, x = 0.f;; i++, x += 1.f) {

    lo = b[i] >> 16;
    if( lo>=0 ) break;
    hi = b[i] & 0xffff;

    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];
    tXY = XY[hi] - XY[-lo];

    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;
    if (R < 0.f)
      R = 0.f;

    noise[i] = R - offset;
  }

  for ( ;; i++, x += 1.f) {

    lo = b[i] >> 16;
    hi = b[i] & 0xffff;
    if(hi>=n)break;

    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];

    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;
    if (R < 0.f) R = 0.f;

    noise[i] = R - offset;
  }
  for ( ; i < n; i++, x += 1.f) {

    R = (A + x * B) / D;
    if (R < 0.f) R = 0.f;

    noise[i] = R - offset;
  }

  if (fixed <= 0) return;

  for (i = 0, x = 0.f;; i++, x += 1.f) {
    hi = i + fixed / 2;
    lo = hi - fixed;
    if(lo>=0)break;

    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];
    tXY = XY[hi] - XY[-lo];


    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;

    if (R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ;; i++, x += 1.f) {

    hi = i + fixed / 2;
    lo = hi - fixed;
    if(hi>=n)break;

    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];

    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;

    if (R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ; i < n; i++, x += 1.f) {
    R = (A + x * B) / D;
    if (R - offset < noise[i]) noise[i] = R - offset;
  }
}

void _vp_noisemask(vorbis_look_psy *p,
                   float *logmdct,
                   float *logmask){

  int i,n=p->n;
  float *work=alloca(n*sizeof(*work));

  bark_noise_hybridmp(n,p->bark,logmdct,logmask,
                      140.,-1);

  for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

  bark_noise_hybridmp(n,p->bark,work,logmask,0.,
                      p->vi->noisewindowfixed);

  for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];

#if 0
  {
    static int seq=0;

    float work2[n];
    for(i=0;i<n;i++){
      work2[i]=logmask[i]+work[i];
    }

    if(seq&1)
      _analysis_output("median2R",seq/2,work,n,1,0,0);
    else
      _analysis_output("median2L",seq/2,work,n,1,0,0);

    if(seq&1)
      _analysis_output("envelope2R",seq/2,work2,n,1,0,0);
    else
      _analysis_output("envelope2L",seq/2,work2,n,1,0,0);
    seq++;
  }
#endif

  for(i=0;i<n;i++){
    int dB=logmask[i]+.5;
    if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
    if(dB<0)dB=0;
    logmask[i]= work[i]+p->vi->noisecompand[dB];
  }

}

void _vp_tonemask(vorbis_look_psy *p,
                  float *logfft,
                  float *logmask,
                  float global_specmax,
                  float local_specmax){

  int i,n=p->n;

  float *seed=alloca(sizeof(*seed)*p->total_octave_lines);
  float att=local_specmax+p->vi->ath_adjatt;
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;

  

  if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;

  for(i=0;i<n;i++)
    logmask[i]=p->ath[i]+att;

  
  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,seed,logmask);

}

void _vp_offset_and_mix(vorbis_look_psy *p,
                        float *noise,
                        float *tone,
                        int offset_select,
                        float *logmask,
                        float *mdct,
                        float *logmdct){
  int i,n=p->n;
  float de, coeffi, cx;
  float toneatt=p->vi->tone_masteratt[offset_select];

  cx = p->m_val;

  for(i=0;i<n;i++){
    float val= noise[i]+p->noiseoffset[offset_select][i];
    if(val>p->vi->noisemaxsupp)val=p->vi->noisemaxsupp;
    logmask[i]=max(val,tone[i]+toneatt);


    
    







    if(offset_select == 1) {
      coeffi = -17.2;       
      val = val - logmdct[i];  

      if(val > coeffi){
        

        de = 1.0-((val-coeffi)*0.005*cx);
        





        if(de < 0) de = 0.0001;
      }else
        

        de = 1.0-((val-coeffi)*0.0003*cx);
      




      mdct[i] *= de;

    }
  }
}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;

  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*gi->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}

static float FLOOR1_fromdB_LOOKUP[256]={
  1.0649863e-07F, 1.1341951e-07F, 1.2079015e-07F, 1.2863978e-07F,
  1.3699951e-07F, 1.4590251e-07F, 1.5538408e-07F, 1.6548181e-07F,
  1.7623575e-07F, 1.8768855e-07F, 1.9988561e-07F, 2.128753e-07F,
  2.2670913e-07F, 2.4144197e-07F, 2.5713223e-07F, 2.7384213e-07F,
  2.9163793e-07F, 3.1059021e-07F, 3.3077411e-07F, 3.5226968e-07F,
  3.7516214e-07F, 3.9954229e-07F, 4.2550680e-07F, 4.5315863e-07F,
  4.8260743e-07F, 5.1396998e-07F, 5.4737065e-07F, 5.8294187e-07F,
  6.2082472e-07F, 6.6116941e-07F, 7.0413592e-07F, 7.4989464e-07F,
  7.9862701e-07F, 8.5052630e-07F, 9.0579828e-07F, 9.6466216e-07F,
  1.0273513e-06F, 1.0941144e-06F, 1.1652161e-06F, 1.2409384e-06F,
  1.3215816e-06F, 1.4074654e-06F, 1.4989305e-06F, 1.5963394e-06F,
  1.7000785e-06F, 1.8105592e-06F, 1.9282195e-06F, 2.0535261e-06F,
  2.1869758e-06F, 2.3290978e-06F, 2.4804557e-06F, 2.6416497e-06F,
  2.8133190e-06F, 2.9961443e-06F, 3.1908506e-06F, 3.3982101e-06F,
  3.6190449e-06F, 3.8542308e-06F, 4.1047004e-06F, 4.3714470e-06F,
  4.6555282e-06F, 4.9580707e-06F, 5.2802740e-06F, 5.6234160e-06F,
  5.9888572e-06F, 6.3780469e-06F, 6.7925283e-06F, 7.2339451e-06F,
  7.7040476e-06F, 8.2047000e-06F, 8.7378876e-06F, 9.3057248e-06F,
  9.9104632e-06F, 1.0554501e-05F, 1.1240392e-05F, 1.1970856e-05F,
  1.2748789e-05F, 1.3577278e-05F, 1.4459606e-05F, 1.5399272e-05F,
  1.6400004e-05F, 1.7465768e-05F, 1.8600792e-05F, 1.9809576e-05F,
  2.1096914e-05F, 2.2467911e-05F, 2.3928002e-05F, 2.5482978e-05F,
  2.7139006e-05F, 2.8902651e-05F, 3.0780908e-05F, 3.2781225e-05F,
  3.4911534e-05F, 3.7180282e-05F, 3.9596466e-05F, 4.2169667e-05F,
  4.4910090e-05F, 4.7828601e-05F, 5.0936773e-05F, 5.4246931e-05F,
  5.7772202e-05F, 6.1526565e-05F, 6.5524908e-05F, 6.9783085e-05F,
  7.4317983e-05F, 7.9147585e-05F, 8.4291040e-05F, 8.9768747e-05F,
  9.5602426e-05F, 0.00010181521F, 0.00010843174F, 0.00011547824F,
  0.00012298267F, 0.00013097477F, 0.00013948625F, 0.00014855085F,
  0.00015820453F, 0.00016848555F, 0.00017943469F, 0.00019109536F,
  0.00020351382F, 0.00021673929F, 0.00023082423F, 0.00024582449F,
  0.00026179955F, 0.00027881276F, 0.00029693158F, 0.00031622787F,
  0.00033677814F, 0.00035866388F, 0.00038197188F, 0.00040679456F,
  0.00043323036F, 0.00046138411F, 0.00049136745F, 0.00052329927F,
  0.00055730621F, 0.00059352311F, 0.00063209358F, 0.00067317058F,
  0.00071691700F, 0.00076350630F, 0.00081312324F, 0.00086596457F,
  0.00092223983F, 0.00098217216F, 0.0010459992F, 0.0011139742F,
  0.0011863665F, 0.0012634633F, 0.0013455702F, 0.0014330129F,
  0.0015261382F, 0.0016253153F, 0.0017309374F, 0.0018434235F,
  0.0019632195F, 0.0020908006F, 0.0022266726F, 0.0023713743F,
  0.0025254795F, 0.0026895994F, 0.0028643847F, 0.0030505286F,
  0.0032487691F, 0.0034598925F, 0.0036847358F, 0.0039241906F,
  0.0041792066F, 0.0044507950F, 0.0047400328F, 0.0050480668F,
  0.0053761186F, 0.0057254891F, 0.0060975636F, 0.0064938176F,
  0.0069158225F, 0.0073652516F, 0.0078438871F, 0.0083536271F,
  0.0088964928F, 0.009474637F, 0.010090352F, 0.010746080F,
  0.011444421F, 0.012188144F, 0.012980198F, 0.013823725F,
  0.014722068F, 0.015678791F, 0.016697687F, 0.017782797F,
  0.018938423F, 0.020169149F, 0.021479854F, 0.022875735F,
  0.024362330F, 0.025945531F, 0.027631618F, 0.029427276F,
  0.031339626F, 0.033376252F, 0.035545228F, 0.037855157F,
  0.040315199F, 0.042935108F, 0.045725273F, 0.048696758F,
  0.051861348F, 0.055231591F, 0.058820850F, 0.062643361F,
  0.066714279F, 0.071049749F, 0.075666962F, 0.080584227F,
  0.085821044F, 0.091398179F, 0.097337747F, 0.10366330F,
  0.11039993F, 0.11757434F, 0.12521498F, 0.13335215F,
  0.14201813F, 0.15124727F, 0.16107617F, 0.17154380F,
  0.18269168F, 0.19456402F, 0.20720788F, 0.22067342F,
  0.23501402F, 0.25028656F, 0.26655159F, 0.28387361F,
  0.30232132F, 0.32196786F, 0.34289114F, 0.36517414F,
  0.38890521F, 0.41417847F, 0.44109412F, 0.46975890F,
  0.50028648F, 0.53279791F, 0.56742212F, 0.60429640F,
  0.64356699F, 0.68538959F, 0.72993007F, 0.77736504F,
  0.82788260F, 0.88168307F, 0.9389798F, 1.F,
};


static int apsort(const void *a, const void *b){
  float f1=**(float**)a;
  float f2=**(float**)b;
  return (f1<f2)-(f1>f2);
}

static void flag_lossless(int limit, float prepoint, float postpoint, float *mdct,
                         float *floor, int *flag, int i, int jn){
  int j;
  for(j=0;j<jn;j++){
    float point = j>=limit-i ? postpoint : prepoint;
    float r = fabs(mdct[j])/floor[j];
    if(r<point)
      flag[j]=0;
    else
      flag[j]=1;
  }
}





static float noise_normalize(vorbis_look_psy *p, int limit, float *r, float *q, float *f, int *flags, float acc, int i, int n, int *out){

  vorbis_info_psy *vi=p->vi;
  float **sort = alloca(n*sizeof(*sort));
  int j,count=0;
  int start = (vi->normal_p ? vi->normal_start-i : n);
  if(start>n)start=n;

  
  acc=0.f;

  

  for(j=0;j<start;j++){
    if(!flags || !flags[j]){ 


      float ve = q[j]/f[j];
      if(r[j]<0)
        out[j] = -rint(sqrt(ve));
      else
        out[j] = rint(sqrt(ve));
    }
  }

  
  for(;j<n;j++){
    if(!flags || !flags[j]){ 


      float ve = q[j]/f[j];
      



      
      if(ve<.25f && (!flags || j>=limit-i)){
        acc += ve;
        sort[count++]=q+j; 
      }else{
        
        if(r[j]<0)
          out[j] = -rint(sqrt(ve));
        else
          out[j] = rint(sqrt(ve));
        q[j] = out[j]*out[j]*f[j];
      }
    }


  }

  if(count){
    
    qsort(sort,count,sizeof(*sort),apsort);
    for(j=0;j<count;j++){
      int k=sort[j]-q;
      if(acc>=vi->normal_thresh){
        out[k]=unitnorm(r[k]);
        acc-=1.f;
        q[k]=f[k];
      }else{
        out[k]=0;
        q[k]=0.f;
      }
    }
  }

  return acc;
}



void _vp_couple_quantize_normalize(int blobno,
                                   vorbis_info_psy_global *g,
                                   vorbis_look_psy *p,
                                   vorbis_info_mapping0 *vi,
                                   float **mdct,
                                   int   **iwork,
                                   int    *nonzero,
                                   int     sliding_lowpass,
                                   int     ch){

  int i;
  int n = p->n;
  int partition=(p->vi->normal_p ? p->vi->normal_partition : 16);
  int limit = g->coupling_pointlimit[p->vi->blockflag][blobno];
  float prepoint=stereo_threshholds[g->coupling_prepointamp[blobno]];
  float postpoint=stereo_threshholds[g->coupling_postpointamp[blobno]];
  float de=0.1*p->m_val; 

  
  

  
  float **raw = alloca(ch*sizeof(*raw));

  
  float **quant = alloca(ch*sizeof(*quant));

  
  float **floor = alloca(ch*sizeof(*floor));

  
  int   **flag  = alloca(ch*sizeof(*flag));

  
  int    *nz    = alloca(ch*sizeof(*nz));

  
  float  *acc   = alloca((ch+vi->coupling_steps)*sizeof(*acc));

  
  if(n > 1000)
    postpoint=stereo_threshholds_limited[g->coupling_postpointamp[blobno]];

  raw[0]   = alloca(ch*partition*sizeof(**raw));
  quant[0] = alloca(ch*partition*sizeof(**quant));
  floor[0] = alloca(ch*partition*sizeof(**floor));
  flag[0]  = alloca(ch*partition*sizeof(**flag));

  for(i=1;i<ch;i++){
    raw[i]   = &raw[0][partition*i];
    quant[i] = &quant[0][partition*i];
    floor[i] = &floor[0][partition*i];
    flag[i]  = &flag[0][partition*i];
  }
  for(i=0;i<ch+vi->coupling_steps;i++)
    acc[i]=0.f;

  for(i=0;i<n;i+=partition){
    int k,j,jn = partition > n-i ? n-i : partition;
    int step,track = 0;

    memcpy(nz,nonzero,sizeof(*nz)*ch);

    
    memset(flag[0],0,ch*partition*sizeof(**flag));
    for(k=0;k<ch;k++){
      int *iout = &iwork[k][i];
      if(nz[k]){

        for(j=0;j<jn;j++)
          floor[k][j] = FLOOR1_fromdB_LOOKUP[iout[j]];

        flag_lossless(limit,prepoint,postpoint,&mdct[k][i],floor[k],flag[k],i,jn);

        for(j=0;j<jn;j++){
          quant[k][j] = raw[k][j] = mdct[k][i+j]*mdct[k][i+j];
          if(mdct[k][i+j]<0.f) raw[k][j]*=-1.f;
          floor[k][j]*=floor[k][j];
        }

        acc[track]=noise_normalize(p,limit,raw[k],quant[k],floor[k],NULL,acc[track],i,jn,iout);

      }else{
        for(j=0;j<jn;j++){
          floor[k][j] = 1e-10f;
          raw[k][j] = 0.f;
          quant[k][j] = 0.f;
          flag[k][j] = 0;
          iout[j]=0;
        }
        acc[track]=0.f;
      }
      track++;
    }

    
    for(step=0;step<vi->coupling_steps;step++){
      int Mi = vi->coupling_mag[step];
      int Ai = vi->coupling_ang[step];
      int *iM = &iwork[Mi][i];
      int *iA = &iwork[Ai][i];
      float *reM = raw[Mi];
      float *reA = raw[Ai];
      float *qeM = quant[Mi];
      float *qeA = quant[Ai];
      float *floorM = floor[Mi];
      float *floorA = floor[Ai];
      int *fM = flag[Mi];
      int *fA = flag[Ai];

      if(nz[Mi] || nz[Ai]){
        nz[Mi] = nz[Ai] = 1;

        for(j=0;j<jn;j++){

          if(j<sliding_lowpass-i){
            if(fM[j] || fA[j]){
              

              reM[j] = fabs(reM[j])+fabs(reA[j]);
              qeM[j] = qeM[j]+qeA[j];
              fM[j]=fA[j]=1;

              
              {
                int A = iM[j];
                int B = iA[j];

                if(abs(A)>abs(B)){
                  iA[j]=(A>0?A-B:B-A);
                }else{
                  iA[j]=(B>0?A-B:B-A);
                  iM[j]=B;
                }

                
                if(iA[j]>=abs(iM[j])*2){
                  iA[j]= -iA[j];
                  iM[j]= -iM[j];
                }

              }

            }else{
              
              if(j<limit-i){
                
                reM[j] += reA[j];
                qeM[j] = fabs(reM[j]);
              }else{
#if 0
                
                




                float derate = (1.0 - de*((float)(j-limit+i) / (float)(n-limit)));
                
                if(reM[j]+reA[j]<0){
                  reM[j] = - (qeM[j] = (fabs(reM[j])+fabs(reA[j]))*derate*derate);
                }else{
                  reM[j] =   (qeM[j] = (fabs(reM[j])+fabs(reA[j]))*derate*derate);
                }
#else
                
                if(reM[j]+reA[j]<0){
                  reM[j] = - (qeM[j] = fabs(reM[j])+fabs(reA[j]));
                }else{
                  reM[j] =   (qeM[j] = fabs(reM[j])+fabs(reA[j]));
                }
#endif

              }
              reA[j]=qeA[j]=0.f;
              fA[j]=1;
              iA[j]=0;
            }
          }
          floorM[j]=floorA[j]=floorM[j]+floorA[j];
        }
        
        acc[track]=noise_normalize(p,limit,raw[Mi],quant[Mi],floor[Mi],flag[Mi],acc[track],i,jn,iM);
        track++;
      }
    }
  }

  for(i=0;i<vi->coupling_steps;i++){
    

    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){
      nonzero[vi->coupling_mag[i]]=1;
      nonzero[vi->coupling_ang[i]]=1;
    }
  }
}
