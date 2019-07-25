
















#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "misc.h"
#include "ivorbiscodec.h"
#include "codebook.h"


int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}





#define VQ_FEXP 10
#define VQ_FMAN 21
#define VQ_FEXP_BIAS 768 /* bias toward values smaller than 1. */

static ogg_int32_t _float32_unpack(long val,int *point){
  long   mant=val&0x1fffff;
  int    sign=val&0x80000000;
  long   exp =(val&0x7fe00000L)>>VQ_FMAN;

  exp-=(VQ_FMAN-1)+VQ_FEXP_BIAS;

  if(mant){
    while(!(mant&0x40000000)){
      mant<<=1;
      exp-=1;
    }

    if(sign)mant= -mant;
  }else{
    sign=0;
    exp=-9999;
  }

  *point=exp;
  return mant;
}




ogg_uint32_t *_make_words(long *l,long n,long sparsecount){
  long i,j,count=0;
  ogg_uint32_t marker[33];
  ogg_uint32_t *r=(ogg_uint32_t *)_ogg_malloc((sparsecount?sparsecount:n)*sizeof(*r));
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

  




  if(sparsecount != 1){
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
  
  int bits=_ilog(b->entries);
  int vals=b->entries>>((bits-1)*(b->dim-1)/b->dim);

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










ogg_int32_t *_book_unquantize(const static_codebook *b,int n,int *sparsemap,
			      int *maxpoint){
  long j,k,count=0;
  if(b->maptype==1 || b->maptype==2){
    int quantvals;
    int minpoint,delpoint;
    ogg_int32_t mindel=_float32_unpack(b->q_min,&minpoint);
    ogg_int32_t delta=_float32_unpack(b->q_delta,&delpoint);
    ogg_int32_t *r=(ogg_int32_t *)_ogg_calloc(n*b->dim,sizeof(*r));
    int *rp=(int *)_ogg_calloc(n*b->dim,sizeof(*rp));

    *maxpoint=minpoint;

    

    switch(b->maptype){
    case 1:
      






      quantvals=_book_maptype1_quantvals(b);
      for(j=0;j<b->entries;j++){
	if((sparsemap && b->lengthlist[j]) || !sparsemap){
	  ogg_int32_t last=0;
	  int lastpoint=0;
	  int indexdiv=1;
	  for(k=0;k<b->dim;k++){
	    int index= (j/indexdiv)%quantvals;
	    int point=0;
	    int val=VFLOAT_MULTI(delta,delpoint,
				 abs(b->quantlist[index]),&point);

	    val=VFLOAT_ADD(mindel,minpoint,val,point,&point);
	    val=VFLOAT_ADD(last,lastpoint,val,point,&point);
	    
	    if(b->q_sequencep){
	      last=val;	  
	      lastpoint=point;
	    }
	    
	    if(sparsemap){
	      r[sparsemap[count]*b->dim+k]=val;
	      rp[sparsemap[count]*b->dim+k]=point;
	    }else{
	      r[count*b->dim+k]=val;
	      rp[count*b->dim+k]=point;
	    }
	    if(*maxpoint<point)*maxpoint=point;
	    indexdiv*=quantvals;
	  }
	  count++;
	}

      }
      break;
    case 2:
      for(j=0;j<b->entries;j++){
	if((sparsemap && b->lengthlist[j]) || !sparsemap){
	  ogg_int32_t last=0;
	  int         lastpoint=0;

	  for(k=0;k<b->dim;k++){
	    int point=0;
	    int val=VFLOAT_MULTI(delta,delpoint,
				 abs(b->quantlist[j*b->dim+k]),&point);

	    val=VFLOAT_ADD(mindel,minpoint,val,point,&point);
	    val=VFLOAT_ADD(last,lastpoint,val,point,&point);
	    
	    if(b->q_sequencep){
	      last=val;	  
	      lastpoint=point;
	    }

	    if(sparsemap){
	      r[sparsemap[count]*b->dim+k]=val;
	      rp[sparsemap[count]*b->dim+k]=point;
	    }else{
	      r[count*b->dim+k]=val;
	      rp[count*b->dim+k]=point;
	    }
	    if(*maxpoint<point)*maxpoint=point;
	  }
	  count++;
	}
      }
      break;
    }

    for(j=0;j<n*b->dim;j++)
      if(rp[j]<*maxpoint)
	r[j]>>=*maxpoint-rp[j];
	    
    _ogg_free(rp);
    return(r);
  }
  return(NULL);
}

void vorbis_staticbook_destroy(static_codebook *b){
  if(b->quantlist)_ogg_free(b->quantlist);
  if(b->lengthlist)_ogg_free(b->lengthlist);
  memset(b,0,sizeof(*b));
  _ogg_free(b);
}

void vorbis_book_clear(codebook *b){
  

  if(b->valuelist)_ogg_free(b->valuelist);
  if(b->codelist)_ogg_free(b->codelist);

  if(b->dec_index)_ogg_free(b->dec_index);
  if(b->dec_codelengths)_ogg_free(b->dec_codelengths);
  if(b->dec_firsttable)_ogg_free(b->dec_firsttable);

  memset(b,0,sizeof(*b));
}

static ogg_uint32_t bitreverse(ogg_uint32_t x){
  x=    ((x>>16)&0x0000ffffUL) | ((x<<16)&0xffff0000UL);
  x=    ((x>> 8)&0x00ff00ffUL) | ((x<< 8)&0xff00ff00UL);
  x=    ((x>> 4)&0x0f0f0f0fUL) | ((x<< 4)&0xf0f0f0f0UL);
  x=    ((x>> 2)&0x33333333UL) | ((x<< 2)&0xccccccccUL);
  return((x>> 1)&0x55555555UL) | ((x<< 1)&0xaaaaaaaaUL);
}

static int sort32a(const void *a,const void *b){
  return (**(ogg_uint32_t **)a>**(ogg_uint32_t **)b)-
    (**(ogg_uint32_t **)a<**(ogg_uint32_t **)b);
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
    ogg_uint32_t **codep=(ogg_uint32_t **)alloca(sizeof(*codep)*n);
    
    if(codes==NULL)goto err_out;

    for(i=0;i<n;i++){
      codes[i]=bitreverse(codes[i]);
      codep[i]=codes+i;
    }

    qsort(codep,n,sizeof(*codep),sort32a);

    sortindex=(int *)alloca(n*sizeof(*sortindex));
    c->codelist=(ogg_uint32_t *)_ogg_malloc(n*sizeof(*c->codelist));
    
    for(i=0;i<n;i++){
      int position=codep[i]-codes;
      sortindex[position]=i;
    }

    for(i=0;i<n;i++)
      c->codelist[sortindex[i]]=codes[i];
    _ogg_free(codes);
    
    
    
    c->valuelist=_book_unquantize(s,n,sortindex,&c->binarypoint);
    c->dec_index=(int *)_ogg_malloc(n*sizeof(*c->dec_index));
    
    for(n=0,i=0;i<s->entries;i++)
      if(s->lengthlist[i]>0)
	c->dec_index[sortindex[n++]]=i;
    
    c->dec_codelengths=(char *)_ogg_malloc(n*sizeof(*c->dec_codelengths));
    for(n=0,i=0;i<s->entries;i++)
      if(s->lengthlist[i]>0)
	c->dec_codelengths[sortindex[n++]]=s->lengthlist[i];
    
    c->dec_firsttablen=_ilog(c->used_entries)-4; 
    if(c->dec_firsttablen<5)c->dec_firsttablen=5;
    if(c->dec_firsttablen>8)c->dec_firsttablen=8;
    
    tabn=1<<c->dec_firsttablen;
    c->dec_firsttable=(ogg_uint32_t *)_ogg_calloc(tabn,sizeof(*c->dec_firsttable));
    c->dec_maxlength=0;
    
    for(i=0;i<n;i++){
      if(c->dec_maxlength<c->dec_codelengths[i])
	c->dec_maxlength=c->dec_codelengths[i];
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

  return(0);
 err_out:
  vorbis_book_clear(c);
  return(-1);
}

