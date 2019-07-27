
















#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "os.h"
#include "misc.h"
#include "vorbis/codec.h"
#include "codebook.h"
#include "scales.h"



int ov_ilog(ogg_uint32_t v){
  int ret;
  for(ret=0;v;ret++)v>>=1;
  return ret;
}





#define VQ_FEXP 10
#define VQ_FMAN 21
#define VQ_FEXP_BIAS 768 /* bias toward values smaller than 1. */


long _float32_pack(float val){
  int sign=0;
  long exp;
  long mant;
  if(val<0){
    sign=0x80000000;
    val= -val;
  }
  exp= floor(log(val)/log(2.f)+.001); 
  mant=rint(ldexp(val,(VQ_FMAN-1)-exp));
  exp=(exp+VQ_FEXP_BIAS)<<VQ_FMAN;

  return(sign|exp|mant);
}

float _float32_unpack(long val){
  double mant=val&0x1fffff;
  int    sign=val&0x80000000;
  long   exp =(val&0x7fe00000L)>>VQ_FMAN;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-(VQ_FMAN-1)-VQ_FEXP_BIAS));
}




ogg_uint32_t *_make_words(char *l,long n,long sparsecount){
  long i,j,count=0;
  ogg_uint32_t marker[33];
  ogg_uint32_t *r=_ogg_malloc((sparsecount?sparsecount:n)*sizeof(*r));
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    if(length>0){
      ogg_uint32_t entry=marker[length];

      




      
      if(length<32 && (entry>>length)){
        
        _ogg_free(r);
        return(NULL);
      }
      r[count++]=entry;

      

      {
        for(j=length;j>0;j--){

          if(marker[j]&1){
            
            if(j==1)
              marker[1]++;
            else
              marker[j]=marker[j-1]<<1;
            break; 

          }
          marker[j]++;
        }
      }

      


      for(j=length+1;j<33;j++)
        if((marker[j]>>1) == entry){
          entry=marker[j];
          marker[j]=marker[j-1]<<1;
        }else
          break;
    }else
      if(sparsecount==0)count++;
  }

  
  


  if(!(count==1 && marker[2]==2)){
    for(i=1;i<33;i++)
      if(marker[i] & (0xffffffffUL>>(32-i))){
        _ogg_free(r);
        return(NULL);
      }
  }

  

  for(i=0,count=0;i<n;i++){
    ogg_uint32_t temp=0;
    for(j=0;j<l[i];j++){
      temp<<=1;
      temp|=(r[count]>>j)&1;
    }

    if(sparsecount){
      if(l[i])
        r[count++]=temp;
    }else
      r[count++]=temp;
  }

  return(r);
}




long _book_maptype1_quantvals(const static_codebook *b){
  long vals=floor(pow((float)b->entries,1.f/b->dim));

  



  
  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}






float *_book_unquantize(const static_codebook *b,int n,int *sparsemap){
  long j,k,count=0;
  if(b->maptype==1 || b->maptype==2){
    int quantvals;
    float mindel=_float32_unpack(b->q_min);
    float delta=_float32_unpack(b->q_delta);
    float *r=_ogg_calloc(n*b->dim,sizeof(*r));

    

    switch(b->maptype){
    case 1:
      






      quantvals=_book_maptype1_quantvals(b);
      for(j=0;j<b->entries;j++){
        if((sparsemap && b->lengthlist[j]) || !sparsemap){
          float last=0.f;
          int indexdiv=1;
          for(k=0;k<b->dim;k++){
            int index= (j/indexdiv)%quantvals;
            float val=b->quantlist[index];
            val=fabs(val)*delta+mindel+last;
            if(b->q_sequencep)last=val;
            if(sparsemap)
              r[sparsemap[count]*b->dim+k]=val;
            else
              r[count*b->dim+k]=val;
            indexdiv*=quantvals;
          }
          count++;
        }

      }
      break;
    case 2:
      for(j=0;j<b->entries;j++){
        if((sparsemap && b->lengthlist[j]) || !sparsemap){
          float last=0.f;

          for(k=0;k<b->dim;k++){
            float val=b->quantlist[j*b->dim+k];
            val=fabs(val)*delta+mindel+last;
            if(b->q_sequencep)last=val;
            if(sparsemap)
              r[sparsemap[count]*b->dim+k]=val;
            else
              r[count*b->dim+k]=val;
          }
          count++;
        }
      }
      break;
    }

    return(r);
  }
  return(NULL);
}

void vorbis_staticbook_destroy(static_codebook *b){
  if(b->allocedp){
    if(b->quantlist)_ogg_free(b->quantlist);
    if(b->lengthlist)_ogg_free(b->lengthlist);
    memset(b,0,sizeof(*b));
    _ogg_free(b);
  } 
}

void vorbis_book_clear(codebook *b){
  

  if(b->valuelist)_ogg_free(b->valuelist);
  if(b->codelist)_ogg_free(b->codelist);

  if(b->dec_index)_ogg_free(b->dec_index);
  if(b->dec_codelengths)_ogg_free(b->dec_codelengths);
  if(b->dec_firsttable)_ogg_free(b->dec_firsttable);

  memset(b,0,sizeof(*b));
}

int vorbis_book_init_encode(codebook *c,const static_codebook *s){

  memset(c,0,sizeof(*c));
  c->c=s;
  c->entries=s->entries;
  c->used_entries=s->entries;
  c->dim=s->dim;
  c->codelist=_make_words(s->lengthlist,s->entries,0);
  
  c->quantvals=_book_maptype1_quantvals(s);
  c->minval=(int)rint(_float32_unpack(s->q_min));
  c->delta=(int)rint(_float32_unpack(s->q_delta));

  return(0);
}

static ogg_uint32_t bitreverse(ogg_uint32_t x){
  x=    ((x>>16)&0x0000ffffUL) | ((x<<16)&0xffff0000UL);
  x=    ((x>> 8)&0x00ff00ffUL) | ((x<< 8)&0xff00ff00UL);
  x=    ((x>> 4)&0x0f0f0f0fUL) | ((x<< 4)&0xf0f0f0f0UL);
  x=    ((x>> 2)&0x33333333UL) | ((x<< 2)&0xccccccccUL);
  return((x>> 1)&0x55555555UL) | ((x<< 1)&0xaaaaaaaaUL);
}

static int sort32a(const void *a,const void *b){
  return ( **(ogg_uint32_t **)a>**(ogg_uint32_t **)b)-
    ( **(ogg_uint32_t **)a<**(ogg_uint32_t **)b);
}


int vorbis_book_init_decode(codebook *c,const static_codebook *s){
  int i,j,n=0,tabn;
  int *sortindex;

  memset(c,0,sizeof(*c));

  
  for(i=0;i<s->entries;i++)
    if(s->lengthlist[i]>0)
      n++;

  c->entries=s->entries;
  c->used_entries=n;
  c->dim=s->dim;

  if(n>0){
    









    
    ogg_uint32_t *codes=_make_words(s->lengthlist,s->entries,c->used_entries);
    ogg_uint32_t **codep=alloca(sizeof(*codep)*n);

    if(codes==NULL)goto err_out;

    for(i=0;i<n;i++){
      codes[i]=bitreverse(codes[i]);
      codep[i]=codes+i;
    }

    qsort(codep,n,sizeof(*codep),sort32a);

    sortindex=alloca(n*sizeof(*sortindex));
    c->codelist=_ogg_malloc(n*sizeof(*c->codelist));
    
    for(i=0;i<n;i++){
      int position=codep[i]-codes;
      sortindex[position]=i;
    }

    for(i=0;i<n;i++)
      c->codelist[sortindex[i]]=codes[i];
    _ogg_free(codes);

    c->valuelist=_book_unquantize(s,n,sortindex);
    c->dec_index=_ogg_malloc(n*sizeof(*c->dec_index));

    for(n=0,i=0;i<s->entries;i++)
      if(s->lengthlist[i]>0)
        c->dec_index[sortindex[n++]]=i;

    c->dec_codelengths=_ogg_malloc(n*sizeof(*c->dec_codelengths));
    c->dec_maxlength=0;
    for(n=0,i=0;i<s->entries;i++)
      if(s->lengthlist[i]>0){
        c->dec_codelengths[sortindex[n++]]=s->lengthlist[i];
        if(s->lengthlist[i]>c->dec_maxlength)
          c->dec_maxlength=s->lengthlist[i];
      }

    if(n==1 && c->dec_maxlength==1){
      


      c->dec_firsttablen=1;
      c->dec_firsttable=_ogg_calloc(2,sizeof(*c->dec_firsttable));
      c->dec_firsttable[0]=c->dec_firsttable[1]=1;

    }else{
      c->dec_firsttablen=ov_ilog(c->used_entries)-4; 
      if(c->dec_firsttablen<5)c->dec_firsttablen=5;
      if(c->dec_firsttablen>8)c->dec_firsttablen=8;

      tabn=1<<c->dec_firsttablen;
      c->dec_firsttable=_ogg_calloc(tabn,sizeof(*c->dec_firsttable));

      for(i=0;i<n;i++){
        if(c->dec_codelengths[i]<=c->dec_firsttablen){
          ogg_uint32_t orig=bitreverse(c->codelist[i]);
          for(j=0;j<(1<<(c->dec_firsttablen-c->dec_codelengths[i]));j++)
            c->dec_firsttable[orig|(j<<c->dec_codelengths[i])]=i+1;
        }
      }

      

      {
        ogg_uint32_t mask=0xfffffffeUL<<(31-c->dec_firsttablen);
        long lo=0,hi=0;

        for(i=0;i<tabn;i++){
          ogg_uint32_t word=i<<(32-c->dec_firsttablen);
          if(c->dec_firsttable[bitreverse(word)]==0){
            while((lo+1)<n && c->codelist[lo+1]<=word)lo++;
            while(    hi<n && word>=(c->codelist[hi]&mask))hi++;

            


            {
              unsigned long loval=lo;
              unsigned long hival=n-hi;

              if(loval>0x7fff)loval=0x7fff;
              if(hival>0x7fff)hival=0x7fff;
              c->dec_firsttable[bitreverse(word)]=
                0x80000000UL | (loval<<15) | hival;
            }
          }
        }
      }
    }
  }

  return(0);
 err_out:
  vorbis_book_clear(c);
  return(-1);
}

long vorbis_book_codeword(codebook *book,int entry){
  if(book->c) 

    return book->codelist[entry];
  return -1;
}

long vorbis_book_codelen(codebook *book,int entry){
  if(book->c) 

    return book->c->lengthlist[entry];
  return -1;
}

#ifdef _V_SELFTEST





#include <stdio.h>











static long full_quantlist1[]={0,1,2,3,    4,5,6,7, 8,3,6,1};
static long partial_quantlist1[]={0,7,2};


static_codebook test1={
  4,16,
  NULL,
  0,
  0,0,0,0,
  NULL,
  0
};
static float *test1_result=NULL;


static_codebook test2={
  4,3,
  NULL,
  2,
  -533200896,1611661312,4,0,
  full_quantlist1,
  0
};
static float test2_result[]={-3,-2,-1,0, 1,2,3,4, 5,0,3,-2};


static_codebook test3={
  4,3,
  NULL,
  2,
  -533200896,1611661312,4,1,
  full_quantlist1,
  0
};
static float test3_result[]={-3,-5,-6,-6, 1,3,6,10, 5,5,8,6};


static_codebook test4={
  3,27,
  NULL,
  1,
  -533200896,1611661312,4,0,
  partial_quantlist1,
  0
};
static float test4_result[]={-3,-3,-3, 4,-3,-3, -1,-3,-3,
                              -3, 4,-3, 4, 4,-3, -1, 4,-3,
                              -3,-1,-3, 4,-1,-3, -1,-1,-3,
                              -3,-3, 4, 4,-3, 4, -1,-3, 4,
                              -3, 4, 4, 4, 4, 4, -1, 4, 4,
                              -3,-1, 4, 4,-1, 4, -1,-1, 4,
                              -3,-3,-1, 4,-3,-1, -1,-3,-1,
                              -3, 4,-1, 4, 4,-1, -1, 4,-1,
                              -3,-1,-1, 4,-1,-1, -1,-1,-1};


static_codebook test5={
  3,27,
  NULL,
  1,
  -533200896,1611661312,4,1,
  partial_quantlist1,
  0
};
static float test5_result[]={-3,-6,-9, 4, 1,-2, -1,-4,-7,
                              -3, 1,-2, 4, 8, 5, -1, 3, 0,
                              -3,-4,-7, 4, 3, 0, -1,-2,-5,
                              -3,-6,-2, 4, 1, 5, -1,-4, 0,
                              -3, 1, 5, 4, 8,12, -1, 3, 7,
                              -3,-4, 0, 4, 3, 7, -1,-2, 2,
                              -3,-6,-7, 4, 1, 0, -1,-4,-5,
                              -3, 1, 0, 4, 8, 7, -1, 3, 2,
                              -3,-4,-5, 4, 3, 2, -1,-2,-3};

void run_test(static_codebook *b,float *comp){
  float *out=_book_unquantize(b,b->entries,NULL);
  int i;

  if(comp){
    if(!out){
      fprintf(stderr,"_book_unquantize incorrectly returned NULL\n");
      exit(1);
    }

    for(i=0;i<b->entries*b->dim;i++)
      if(fabs(out[i]-comp[i])>.0001){
        fprintf(stderr,"disagreement in unquantized and reference data:\n"
                "position %d, %g != %g\n",i,out[i],comp[i]);
        exit(1);
      }

  }else{
    if(out){
      fprintf(stderr,"_book_unquantize returned a value array: \n"
              " correct result should have been NULL\n");
      exit(1);
    }
  }
}

int main(){
  
  fprintf(stderr,"Dequant test 1... ");
  run_test(&test1,test1_result);
  fprintf(stderr,"OK\nDequant test 2... ");
  run_test(&test2,test2_result);
  fprintf(stderr,"OK\nDequant test 3... ");
  run_test(&test3,test3_result);
  fprintf(stderr,"OK\nDequant test 4... ");
  run_test(&test4,test4_result);
  fprintf(stderr,"OK\nDequant test 5... ");
  run_test(&test5,test5_result);
  fprintf(stderr,"OK\n\n");

  return(0);
}

#endif
