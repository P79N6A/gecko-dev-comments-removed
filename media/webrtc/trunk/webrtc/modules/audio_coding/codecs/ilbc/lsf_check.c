

















#include "defines.h"
#include "constants.h"





int WebRtcIlbcfix_LsfCheck(
    int16_t *lsf, 
    int dim, 
    int NoAn)  
{
  int k,n,m, Nit=2, change=0,pos;
  const int16_t eps=319;  
  const int16_t eps2=160;  
  const int16_t maxlsf=25723; 
  const int16_t minlsf=82;  

  
  for (n=0;n<Nit;n++) {  
    for (m=0;m<NoAn;m++) { 
      for (k=0;k<(dim-1);k++) {
        pos=m*dim+k;

        
        if ((lsf[pos+1]-lsf[pos])<eps) {

          if (lsf[pos+1]<lsf[pos]) {
            lsf[pos+1]= lsf[pos]+eps2;
            lsf[pos]= lsf[pos+1]-eps2;
          } else {
            lsf[pos]-=eps2;
            lsf[pos+1]+=eps2;
          }
          change=1;
        }

        
        if (lsf[pos]<minlsf) {
          lsf[pos]=minlsf;
          change=1;
        }

        if (lsf[pos]>maxlsf) {
          lsf[pos]=maxlsf;
          change=1;
        }
      }
    }
  }

  return change;
}
