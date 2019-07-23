








































































#include <stddef.h>
#include "prtypes.h"
#include "prmem.h"
#include "prlog.h"
#include "GIF2.h"

#include "nsGIFDecoder2.h"










#define GETN(n,s)                    \
  PR_BEGIN_MACRO                     \
    gs->bytes_to_consume = (n);      \
    gs->state = (s);                 \
  PR_END_MACRO


#define GETINT16(p)   ((p)[1]<<8|(p)[0])



static void output_row(gif_struct *gs)
{
  int width, drow_start, drow_end;

  drow_start = drow_end = gs->irow;

  





  if (gs->progressive_display && gs->interlaced && (gs->ipass < 4)) {
    PRUintn row_dup = 0, row_shift = 0;

    switch (gs->ipass) {
    case 1:
      row_dup = 7;
      row_shift = 3;
      break;
    case 2:
      row_dup = 3;
      row_shift = 1;
      break;
    case 3:
      row_dup = 1;
      row_shift = 0;
      break;
    default:
      break;
    }

    drow_start -= row_shift;
    drow_end = drow_start + row_dup;

    
    if (((gs->height - 1) - drow_end) <= row_shift)
      drow_end = gs->height - 1;

    
    if (drow_start < 0)
      drow_start = 0;
    if ((PRUintn)drow_end >= gs->height)
      drow_end = gs->height - 1;
  }

  
  if ((PRUintn)drow_start >= gs->height) {
    NS_WARNING("GIF2.cpp::output_row - too much image data");
    return;
  }

  
  if ((gs->y_offset + gs->irow) < gs->screen_height) {
    
    if ((gs->x_offset + gs->width) > gs->screen_width)
      width = gs->screen_width - gs->x_offset;
    else
      width = gs->width;

    if (width > 0)
      
      nsGIFDecoder2::HaveDecodedRow(
        gs->clientptr,
        gs->rowbuf,      
        drow_start,      
        drow_end - drow_start + 1, 
        gs->ipass);      
  }

  gs->rowp = gs->rowbuf;

  if (!gs->interlaced)
    gs->irow++;
  else {
    do {
      switch (gs->ipass)
      {
        case 1:
          gs->irow += 8;
          if (gs->irow >= gs->height) {
            gs->ipass++;
            gs->irow = 4;
          }
          break;

        case 2:
          gs->irow += 8;
          if (gs->irow >= gs->height) {
            gs->ipass++;
            gs->irow = 2;
          }
          break;

        case 3:
          gs->irow += 4;
          if (gs->irow >= gs->height) {
            gs->ipass++;
            gs->irow = 1;
          }
          break;

        case 4:
          gs->irow += 2;
          if (gs->irow >= gs->height){
            gs->ipass++;
            gs->irow = 0;
          }
          break;

        default:
          break;
      }
    } while (gs->irow > (gs->height - 1));
  }
}



static int do_lzw(gif_struct *gs, const PRUint8 *q)
{
  int code;
  int incode;
  const PRUint8 *ch;

  



  int avail       = gs->avail;
  int bits        = gs->bits;
  int codesize    = gs->codesize;
  int codemask    = gs->codemask;
  int count       = gs->count;
  int oldcode     = gs->oldcode;
  int clear_code  = gs->clear_code;
  PRUint8 firstchar = gs->firstchar;
  PRInt32 datum     = gs->datum;
  PRUint16 *prefix  = gs->prefix;
  PRUint8 *stackp   = gs->stackp;
  PRUint8 *suffix   = gs->suffix;
  PRUint8 *stack    = gs->stack;
  PRUint8 *rowp     = gs->rowp;
  PRUint8 *rowend   = gs->rowend;
  PRUintn rows_remaining = gs->rows_remaining;

  if (rowp == rowend)
    return 0;

#define OUTPUT_ROW(gs)                                                  \
  PR_BEGIN_MACRO                                                        \
    output_row(gs);                                                     \
    rows_remaining--;                                                   \
    rowp = gs->rowp;                                                    \
    if (!rows_remaining)                                                \
      goto END;                                                         \
  PR_END_MACRO

  for (ch = q; count-- > 0; ch++)
  {
    
    datum += ((int32) *ch) << bits;
    bits += 8;

    
    while (bits >= codesize)
    {
      
      code = datum & codemask;
      datum >>= codesize;
      bits -= codesize;

      
      if (code == clear_code) {
        codesize = gs->datasize + 1;
        codemask = (1 << codesize) - 1;
        avail = clear_code + 2;
        oldcode = -1;
        continue;
      }

      
      if (code == (clear_code + 1)) {
        
        if (rows_remaining != 0)
          return -1;
        return 0;
      }

      if (oldcode == -1) {
        *rowp++ = suffix[code];
        if (rowp == rowend)
          OUTPUT_ROW(gs);

        firstchar = oldcode = code;
        continue;
      }

      incode = code;
      if (code >= avail) {
        *stackp++ = firstchar;
        code = oldcode;

        if (stackp == stack + MAX_BITS)
          return -1;
      }

      while (code >= clear_code)
      {
        if (code == prefix[code])
          return -1;

        *stackp++ = suffix[code];
        code = prefix[code];

        if (stackp == stack + MAX_BITS)
          return -1;
      }

      *stackp++ = firstchar = suffix[code];

      
      if (avail < 4096) {
        prefix[avail] = oldcode;
        suffix[avail] = firstchar;
        avail++;

        



        if (((avail & codemask) == 0) && (avail < 4096)) {
          codesize++;
          codemask += avail;
        }
      }
      oldcode = incode;

        
      do {
        *rowp++ = *--stackp;
        if (rowp == rowend) {
          OUTPUT_ROW(gs);
        }
      } while (stackp > stack);
    }
  }

  END:

  
  gs->avail = avail;
  gs->bits = bits;
  gs->codesize = codesize;
  gs->codemask = codemask;
  gs->count = count;
  gs->oldcode = oldcode;
  gs->firstchar = firstchar;
  gs->datum = datum;
  gs->stackp = stackp;
  gs->rowp = rowp;
  gs->rows_remaining = rows_remaining;

  return 0;
}




PRBool GIFInit(gif_struct* gs, void* aClientData)
{
  NS_ASSERTION(gs, "Got null argument");
  if (!gs)
    return PR_FALSE;

  
  memset(gs, 0, sizeof(gif_struct));
  gs->clientptr = aClientData;

  
  gs->state = gif_type;
  gs->bytes_to_consume = 6;

  return PR_TRUE;
}






PRStatus gif_write(gif_struct *gs, const PRUint8 *buf, PRUint32 len)
{
  if (!gs || !len)
    return PR_FAILURE;

  const PRUint8 *q = buf;

  
  
  
  PRUint8* p;
  if (gs->state == gif_global_colormap)
    p = gs->global_colormap;
  else if (gs->state == gif_image_colormap)
    p = gs->local_colormap;
  else if (gs->bytes_in_hold)
    p = gs->hold;
  else
    p = nsnull;

  if (p) {
    
    PRUint32 l = PR_MIN(len, gs->bytes_to_consume);
    memcpy(p+gs->bytes_in_hold, buf, l);

    if (l < gs->bytes_to_consume) {
      
      gs->bytes_in_hold += l;
      gs->bytes_to_consume -= l;
      return PR_SUCCESS;
    }
    
    gs->bytes_in_hold = 0;
    
    q = p;
  }

  
  
  
  
  
  
  
  

  for (;len >= gs->bytes_to_consume; q=buf) {
    
    buf += gs->bytes_to_consume;
    len -= gs->bytes_to_consume;

    switch (gs->state)
    {
    case gif_lzw:
      if (do_lzw(gs, q) < 0) {
        gs->state = gif_error;
        break;
      }
      GETN(1, gif_sub_block);
      break;

    case gif_lzw_start:
    {
      
      gs->datasize = *q;
      if (gs->datasize > MAX_LZW_BITS) {
        gs->state = gif_error;
        break;
      }

      gs->clear_code = 1 << gs->datasize;
      gs->avail = gs->clear_code + 2;
      gs->oldcode = -1;
      gs->codesize = gs->datasize + 1;
      gs->codemask = (1 << gs->codesize) - 1;

      gs->datum = gs->bits = 0;

      if (gs->clear_code >= MAX_BITS) {
        gs->state = gif_error;
        break;
      }

      
      for (int i = 0; i < gs->clear_code; i++)
        gs->suffix[i] = i;

      gs->stackp = gs->stack;

      GETN(1, gif_sub_block);
    }
    break;

    
    case gif_type:
    {
      if (!strncmp((char*)q, "GIF89a", 6)) {
        gs->version = 89;
      } else if (!strncmp((char*)q, "GIF87a", 6)) {
        gs->version = 87;
      } else {
        gs->state = gif_error;
        break;
      }
      GETN(7, gif_global_header);
    }
    break;

    case gif_global_header:
    {
      






      gs->screen_width = GETINT16(q);
      gs->screen_height = GETINT16(q + 2);

      gs->screen_bgcolor = q[5];

      gs->global_colormap_size = 2<<(q[4]&0x07);

      
      nsGIFDecoder2::BeginGIF(
        gs->clientptr,
        gs->screen_width,
        gs->screen_height,
        gs->screen_bgcolor);

      if (q[4] & 0x80) { 
        
        const PRUint32 size = 3*gs->global_colormap_size;
        if (len < size) {
          
          GETN(size, gif_global_colormap);
          break;
        }
        
        memcpy(gs->global_colormap, buf, size);
        buf += size;
        len -= size;
      }

      GETN(1, gif_image_start);

      
      
      
    }
    break;

    case gif_global_colormap:
      
      GETN(1, gif_image_start);
    break;

    case gif_image_start:
    {
      if (*q == ';') { 
        gs->state = gif_done;
        break;
      }

      if (*q == '!') { 
        GETN(2, gif_extension);
        break;
      }

      




      if (*q != ',') {
        if (gs->images_decoded > 0) {
          




          gs->state = gif_done;
        } else {
          
          gs->state = gif_error;
        }
        break;
      } else
        GETN(9, gif_image_header);
    }
    break;

    case gif_extension:
    {
      int len = gs->count = q[1];
      gstate es = gif_skip_block;

      switch (*q)
      {
      case 0xf9:
        es = gif_control_extension;
        break;

      case 0x01:
        
        break;

      case 0xff:
        es = gif_application_extension;
        break;

      case 0xfe:
        es = gif_consume_comment;
        break;
      }

      if (len)
        GETN(len, es);
      else
        GETN(1, gif_image_start);
    }
    break;

    case gif_consume_block:
      if (!*q)
        GETN(1, gif_image_start);
      else
        GETN(*q, gif_skip_block);
    break;

    case gif_skip_block:
      GETN(1, gif_consume_block);
      break;

    case gif_control_extension:
    {
      if (*q & 0x1) {
        gs->tpixel = q[3];
        gs->is_transparent = PR_TRUE;
      } else {
        gs->is_transparent = PR_FALSE;
        
      }
      gs->disposal_method = ((*q) >> 2) & 0x7;
      
      
      if (gs->disposal_method == 4)
        gs->disposal_method = 3;
      gs->delay_time = GETINT16(q + 1) * 10;
      GETN(1, gif_consume_block);
    }
    break;

    case gif_comment_extension:
    {
      if (*q)
        GETN(*q, gif_consume_comment);
      else
        GETN(1, gif_image_start);
    }
    break;

    case gif_consume_comment:
      GETN(1, gif_comment_extension);
    break;

    case gif_application_extension:
      
      if (!strncmp((char*)q, "NETSCAPE2.0", 11) ||
        !strncmp((char*)q, "ANIMEXTS1.0", 11))
        GETN(1, gif_netscape_extension_block);
      else
        GETN(1, gif_consume_block);
    break;

    
    case gif_netscape_extension_block:
      if (*q)
        GETN(*q, gif_consume_netscape_extension);
      else
        GETN(1, gif_image_start);
    break;

    
    case gif_consume_netscape_extension:
    {
      int netscape_extension = q[0] & 7;

      

      if (netscape_extension == 1) {
        gs->loop_count = GETINT16(q + 1);

        
        if (gs->loop_count == 0)
          gs->loop_count = -1;

        GETN(1, gif_netscape_extension_block);
      }
      
      else if (netscape_extension == 2) {
        
        
        
        GETN(1, gif_netscape_extension_block);
      } else
        gs->state = gif_error; 
                               

      break;
    }

    case gif_image_header:
    {
      PRUintn height, width;

      
      gs->x_offset = GETINT16(q);
      gs->y_offset = GETINT16(q + 2);

      
      width  = GETINT16(q + 4);
      height = GETINT16(q + 6);

      



      if ((gs->images_decoded == 0) &&
          ((gs->screen_height < height) || (gs->screen_width < width) ||
           (gs->version == 87)))
      {
        gs->screen_height = height;
        gs->screen_width = width;
        gs->x_offset = 0;
        gs->y_offset = 0;

        nsGIFDecoder2::BeginGIF(gs->clientptr,
                                gs->screen_width,
                                gs->screen_height,
                                gs->screen_bgcolor);
      }

      

      if (!height || !width) {
        height = gs->screen_height;
        width = gs->screen_width;
        if (!height || !width) {
          gs->state = gif_error;
          break;
        }
      }

      gs->height = height;
      gs->width = width;

      nsGIFDecoder2::BeginImageFrame(gs->clientptr,
                                     gs->images_decoded + 1,   
                                     gs->x_offset,  
                                     gs->y_offset,  
                                     width,
                                     height);

      
      
      
      if (gs->screen_width < width) {
        

        gs->rowbuf = (PRUint8*)PR_REALLOC(gs->rowbuf, width);

        if (!gs->rowbuf) {
          gs->state = gif_oom;
          break;
        }

        gs->screen_width = width;
        if (gs->screen_height < gs->height)
          gs->screen_height = gs->height;

      }
      else {
        if (!gs->rowbuf)
          gs->rowbuf = (PRUint8*)PR_MALLOC(gs->screen_width);
      }

      if (!gs->rowbuf) {
          gs->state = gif_oom;
          break;
      }

      if (q[8] & 0x40) {
        gs->interlaced = PR_TRUE;
        gs->ipass = 1;
      } else {
        gs->interlaced = PR_FALSE;
        gs->ipass = 0;
      }

      if (gs->images_decoded == 0) {
        gs->progressive_display = PR_TRUE;
      } else {
        





        gs->progressive_display = PR_FALSE;
      }

      
      gs->irow = 0;
      gs->rows_remaining = gs->height;
      gs->rowend = gs->rowbuf + gs->width;
      gs->rowp = gs->rowbuf;

      

      if (q[8] & 0x80) 
      {
        int num_colors = 2 << (q[8] & 0x7);
        const PRUint32 size = 3*num_colors;
        PRUint8 *map = gs->local_colormap;
        if (!map || (num_colors > gs->local_colormap_size)) {
          map = (PRUint8*)PR_REALLOC(map, size);
          if (!map) {
            gs->state = gif_oom;
            break;
          }
        }

        
        gs->local_colormap = map;
        gs->local_colormap_size = num_colors;
        gs->is_local_colormap_defined = PR_TRUE;

        if (len < size) {
          
          GETN(size, gif_image_colormap);
          break;
        }
        
        memcpy(gs->local_colormap, buf, size);
        buf += size;
        len -= size;
      } else {
        
        gs->is_local_colormap_defined = PR_FALSE;
      }
      GETN(1, gif_lzw_start);
    }
    break;

    case gif_image_colormap:
      
      GETN(1, gif_lzw_start);
    break;

    case gif_sub_block:
    {
      if ((gs->count = *q) != 0)
      
      {
        
        
        if (gs->rows_remaining == 0) {
          
#ifdef DONT_TOLERATE_BROKEN_GIFS
          gs->state = gif_error;
          break;
#else
          GETN(1, gif_sub_block);
#endif
        }
        GETN(gs->count, gif_lzw);
      }
      else
      
      {
        gs->images_decoded++;

        nsGIFDecoder2::EndImageFrame(gs->clientptr,
                                     gs->images_decoded,
                                     gs->delay_time);

        
        gs->is_local_colormap_defined = PR_FALSE;
        gs->is_transparent = PR_FALSE;

        

        if (gs->delay_time < MINIMUM_DELAY_TIME)
          gs->delay_time = MINIMUM_DELAY_TIME;

        GETN(1, gif_image_start);
      }
    }
    break;

    case gif_done:
      nsGIFDecoder2::EndGIF(gs->clientptr, gs->loop_count);
      return PR_SUCCESS;
      break;

    
    case gif_oom:
      return PR_FAILURE;

    
    case gif_error:
      nsGIFDecoder2::EndGIF(gs->clientptr, gs->loop_count);
      return PR_SUCCESS;

    
    default:
      break;
    }
  }

  
  gs->bytes_in_hold = len;
  if (len) {
    
    PRUint8* p;
    if (gs->state == gif_global_colormap)
      p = gs->global_colormap;
    else if (gs->state == gif_image_colormap)
      p = gs->local_colormap;
    else
      p = gs->hold;
    memcpy(p, buf, len);
    gs->bytes_to_consume -= len;
  }

  return PR_SUCCESS;
}



void gif_destroy(gif_struct *gs)
{
  if (!gs)
    return;

  
  if (gs->delay_time)
    gs->delay_time = 0;

  PR_FREEIF(gs->rowbuf);
  PR_FREEIF(gs->local_colormap);
}

