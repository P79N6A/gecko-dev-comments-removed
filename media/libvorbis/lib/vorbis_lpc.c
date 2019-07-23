












































#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "os.h"
#include "smallft.h"
#include "lpc.h"
#include "scales.h"
#include "misc.h"







float vorbis_lpc_from_data(float *data,float *lpci,int n,int m){
  double *aut=alloca(sizeof(*aut)*(m+1));
  double *lpc=alloca(sizeof(*lpc)*(m));
  double error;
  int i,j;

  
  j=m+1;
  while(j--){
    double d=0; 
    for(i=j;i<n;i++)d+=(double)data[i]*data[i-j];
    aut[j]=d;
  }
  
  

  error=aut[0];
  
  for(i=0;i<m;i++){
    double r= -aut[i+1];

    if(error==0){
      memset(lpci,0,m*sizeof(*lpci));
      return 0;
    }

    




    for(j=0;j<i;j++)r-=lpc[j]*aut[i-j];
    r/=error; 

    
    
    lpc[i]=r;
    for(j=0;j<i/2;j++){
      double tmp=lpc[j];

      lpc[j]+=r*lpc[i-1-j];
      lpc[i-1-j]+=r*tmp;
    }
    if(i%2)lpc[j]+=lpc[j]*r;

    error*=1.f-r*r;
  }

  for(j=0;j<m;j++)lpci[j]=(float)lpc[j];

  

  
  return error;
}

void vorbis_lpc_predict(float *coeff,float *prime,int m,
                     float *data,long n){

  



  long i,j,o,p;
  float y;
  float *work=alloca(sizeof(*work)*(m+n));

  if(!prime)
    for(i=0;i<m;i++)
      work[i]=0.f;
  else
    for(i=0;i<m;i++)
      work[i]=prime[i];

  for(i=0;i<n;i++){
    y=0;
    o=i;
    p=m;
    for(j=0;j<m;j++)
      y-=work[o++]*coeff[--p];
    
    data[i]=work[o]=y;
  }
}





