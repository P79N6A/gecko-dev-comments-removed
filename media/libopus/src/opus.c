


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "opus.h"
#include "opus_private.h"

#ifndef DISABLE_FLOAT_API
OPUS_EXPORT void opus_pcm_soft_clip(float *_x, int N, int C, float *declip_mem)
{
   int c;
   int i;
   float *x;

   



   for (i=0;i<N*C;i++)
      _x[i] = MAX16(-2.f, MIN16(2.f, _x[i]));
   for (c=0;c<C;c++)
   {
      float a;
      float x0;
      int curr;

      x = _x+c;
      a = declip_mem[c];
      

      for (i=0;i<N;i++)
      {
         if (x[i*C]*a>=0)
            break;
         x[i*C] = x[i*C]+a*x[i*C]*x[i*C];
      }

      curr=0;
      x0 = x[0];
      while(1)
      {
         int start, end;
         float maxval;
         int special=0;
         int peak_pos;
         for (i=curr;i<N;i++)
         {
            if (x[i*C]>1 || x[i*C]<-1)
               break;
         }
         if (i==N)
         {
            a=0;
            break;
         }
         peak_pos = i;
         start=end=i;
         maxval=ABS16(x[i*C]);
         
         while (start>0 && x[i*C]*x[(start-1)*C]>=0)
            start--;
         
         while (end<N && x[i*C]*x[end*C]>=0)
         {
            
            if (ABS16(x[end*C])>maxval)
            {
               maxval = ABS16(x[end*C]);
               peak_pos = end;
            }
            end++;
         }
         
         special = (start==0 && x[i*C]*x[0]>=0);

         
         a=(maxval-1)/(maxval*maxval);
         if (x[i*C]>0)
            a = -a;
         
         for (i=start;i<end;i++)
            x[i*C] = x[i*C]+a*x[i*C]*x[i*C];

         if (special && peak_pos>=2)
         {
            

            float delta;
            float offset = x0-x[0];
            delta = offset / peak_pos;
            for (i=curr;i<peak_pos;i++)
            {
               offset -= delta;
               x[i*C] += offset;
               x[i*C] = MAX16(-1.f, MIN16(1.f, x[i*C]));
            }
         }
         curr = end;
         if (curr==N)
            break;
      }
      declip_mem[c] = a;
   }
}
#endif

int encode_size(int size, unsigned char *data)
{
   if (size < 252)
   {
      data[0] = size;
      return 1;
   } else {
      data[0] = 252+(size&0x3);
      data[1] = (size-(int)data[0])>>2;
      return 2;
   }
}

