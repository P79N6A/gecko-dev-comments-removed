


























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

   if (C<1 || N<1 || !_x || !declip_mem) return;

   



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

static int parse_size(const unsigned char *data, opus_int32 len, opus_int16 *size)
{
   if (len<1)
   {
      *size = -1;
      return -1;
   } else if (data[0]<252)
   {
      *size = data[0];
      return 1;
   } else if (len<2)
   {
      *size = -1;
      return -1;
   } else {
      *size = 4*data[1] + data[0];
      return 2;
   }
}

int opus_packet_parse_impl(const unsigned char *data, opus_int32 len,
      int self_delimited, unsigned char *out_toc,
      const unsigned char *frames[48], opus_int16 size[48],
      int *payload_offset, opus_int32 *packet_offset)
{
   int i, bytes;
   int count;
   int cbr;
   unsigned char ch, toc;
   int framesize;
   opus_int32 last_size;
   opus_int32 pad = 0;
   const unsigned char *data0 = data;

   if (size==NULL)
      return OPUS_BAD_ARG;

   framesize = opus_packet_get_samples_per_frame(data, 48000);

   cbr = 0;
   toc = *data++;
   len--;
   last_size = len;
   switch (toc&0x3)
   {
   
   case 0:
      count=1;
      break;
   
   case 1:
      count=2;
      cbr = 1;
      if (!self_delimited)
      {
         if (len&0x1)
            return OPUS_INVALID_PACKET;
         last_size = len/2;
         
         size[0] = (opus_int16)last_size;
      }
      break;
   
   case 2:
      count = 2;
      bytes = parse_size(data, len, size);
      len -= bytes;
      if (size[0]<0 || size[0] > len)
         return OPUS_INVALID_PACKET;
      data += bytes;
      last_size = len-size[0];
      break;
   
   default: 
      if (len<1)
         return OPUS_INVALID_PACKET;
      
      ch = *data++;
      count = ch&0x3F;
      if (count <= 0 || framesize*count > 5760)
         return OPUS_INVALID_PACKET;
      len--;
      
      if (ch&0x40)
      {
         int p;
         do {
            int tmp;
            if (len<=0)
               return OPUS_INVALID_PACKET;
            p = *data++;
            len--;
            tmp = p==255 ? 254: p;
            len -= tmp;
            pad += tmp;
         } while (p==255);
      }
      if (len<0)
         return OPUS_INVALID_PACKET;
      
      cbr = !(ch&0x80);
      if (!cbr)
      {
         
         last_size = len;
         for (i=0;i<count-1;i++)
         {
            bytes = parse_size(data, len, size+i);
            len -= bytes;
            if (size[i]<0 || size[i] > len)
               return OPUS_INVALID_PACKET;
            data += bytes;
            last_size -= bytes+size[i];
         }
         if (last_size<0)
            return OPUS_INVALID_PACKET;
      } else if (!self_delimited)
      {
         
         last_size = len/count;
         if (last_size*count!=len)
            return OPUS_INVALID_PACKET;
         for (i=0;i<count-1;i++)
            size[i] = (opus_int16)last_size;
      }
      break;
   }
   
   if (self_delimited)
   {
      bytes = parse_size(data, len, size+count-1);
      len -= bytes;
      if (size[count-1]<0 || size[count-1] > len)
         return OPUS_INVALID_PACKET;
      data += bytes;
      
      if (cbr)
      {
         if (size[count-1]*count > len)
            return OPUS_INVALID_PACKET;
         for (i=0;i<count-1;i++)
            size[i] = size[count-1];
      } else if (bytes+size[count-1] > last_size)
         return OPUS_INVALID_PACKET;
   } else
   {
      


      if (last_size > 1275)
         return OPUS_INVALID_PACKET;
      size[count-1] = (opus_int16)last_size;
   }

   if (payload_offset)
      *payload_offset = (int)(data-data0);

   for (i=0;i<count;i++)
   {
      if (frames)
         frames[i] = data;
      data += size[i];
   }

   if (packet_offset)
      *packet_offset = pad+(opus_int32)(data-data0);

   if (out_toc)
      *out_toc = toc;

   return count;
}

int opus_packet_parse(const unsigned char *data, opus_int32 len,
      unsigned char *out_toc, const unsigned char *frames[48],
      opus_int16 size[48], int *payload_offset)
{
   return opus_packet_parse_impl(data, len, 0, out_toc,
                                 frames, size, payload_offset, NULL);
}

