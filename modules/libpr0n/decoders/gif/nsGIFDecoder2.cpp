








































































#include <stddef.h>
#include "prmem.h"

#include "nsIInterfaceRequestorUtils.h"

#include "nsGIFDecoder2.h"
#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "imgIContainerObserver.h"

#include "imgILoad.h"

#include "gfxColor.h"
#include "gfxPlatform.h"
#include "qcms.h"










#define GETN(n,s)                      \
  PR_BEGIN_MACRO                       \
    mGIFStruct.bytes_to_consume = (n); \
    mGIFStruct.state = (s);            \
  PR_END_MACRO


#define GETINT16(p)   ((p)[1]<<8|(p)[0])






NS_IMPL_ISUPPORTS1(nsGIFDecoder2, imgIDecoder)

nsGIFDecoder2::nsGIFDecoder2()
  : mCurrentRow(-1)
  , mLastFlushedRow(-1)
  , mImageData(nsnull)
  , mOldColor(0)
  , mCurrentFrame(-1)
  , mCurrentPass(0)
  , mLastFlushedPass(0)
  , mGIFOpen(PR_FALSE)
  , mSawTransparency(PR_FALSE)
{
  
  memset(&mGIFStruct, 0, sizeof(mGIFStruct));
}

nsGIFDecoder2::~nsGIFDecoder2()
{
  Close();
}







NS_IMETHODIMP nsGIFDecoder2::Init(imgILoad *aLoad)
{
  mObserver = do_QueryInterface(aLoad);

  mImageContainer = do_CreateInstance("@mozilla.org/image/container;2");
  aLoad->SetImage(mImageContainer);
  
  
  mGIFStruct.state = gif_type;
  mGIFStruct.bytes_to_consume = 6;

  return NS_OK;
}








NS_IMETHODIMP nsGIFDecoder2::Close()
{
  if (mCurrentFrame == mGIFStruct.images_decoded)
    EndImageFrame();
  EndGIF();

  PR_FREEIF(mGIFStruct.local_colormap);

  return NS_OK;
}



NS_IMETHODIMP nsGIFDecoder2::Flush()
{
    return NS_OK;
}



static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  nsGIFDecoder2 *decoder = static_cast<nsGIFDecoder2*>(closure);
  nsresult rv = decoder->ProcessData((unsigned char*)fromRawSegment, count, writeCount);
  if (NS_FAILED(rv)) {
    *writeCount = 0;
    return rv;
  }

  return NS_OK;
}




nsresult
nsGIFDecoder2::FlushImageData(PRUint32 fromRow, PRUint32 rows)
{
  nsIntRect r(0, fromRow, mGIFStruct.width, rows);

  
  nsresult rv = mImageContainer->FrameUpdated(mGIFStruct.images_decoded, r);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  if (!mGIFStruct.images_decoded && mObserver) {
    PRUint32 imgCurFrame;
    mImageContainer->GetCurrentFrameIndex(&imgCurFrame);
    r.y += mGIFStruct.y_offset;
    mObserver->OnDataAvailable(nsnull, imgCurFrame == PRUint32(mGIFStruct.images_decoded), &r);
  }
  return NS_OK;
}

nsresult
nsGIFDecoder2::FlushImageData()
{
  nsresult rv = NS_OK;

  switch (mCurrentPass - mLastFlushedPass) {
    case 0:  
      if (mCurrentRow - mLastFlushedRow)
        rv = FlushImageData(mLastFlushedRow + 1, mCurrentRow - mLastFlushedRow);
      break;
  
    case 1:  
      rv = FlushImageData(0, mCurrentRow + 1);
      rv |= FlushImageData(mLastFlushedRow + 1, mGIFStruct.height - (mLastFlushedRow + 1));
      break;

    default:   
      rv = FlushImageData(0, mGIFStruct.height);
  }
  return rv;
}


nsresult nsGIFDecoder2::ProcessData(unsigned char *data, PRUint32 count, PRUint32 *_retval)
{
  
  
  nsresult rv = GifWrite(data, count);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!mGIFStruct.images_decoded) {
    rv = FlushImageData();
    mLastFlushedRow = mCurrentRow;
    mLastFlushedPass = mCurrentPass;
  }

  *_retval = count;

  return rv;
}



NS_IMETHODIMP nsGIFDecoder2::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  nsresult rv = inStr->ReadSegments(ReadDataOut, this,  count, _retval);

  


  if (mGIFStruct.state == gif_error || mGIFStruct.state == gif_oom) {
    PRUint32 numFrames = 0;
    if (mImageContainer)
      mImageContainer->GetNumFrames(&numFrames);
    if (numFrames <= 1)
      rv = NS_ERROR_FAILURE;
  }

  return rv;
}







void nsGIFDecoder2::BeginGIF()
{
  if (mGIFOpen)
    return;
    
  if (mObserver)
    mObserver->OnStartDecode(nsnull);

  mImageContainer->Init(mGIFStruct.screen_width, mGIFStruct.screen_height, mObserver);

  if (mObserver)
    mObserver->OnStartContainer(nsnull, mImageContainer);

  mGIFOpen = PR_TRUE;
}


void nsGIFDecoder2::EndGIF()
{
  if (!mGIFOpen)
    return;

  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImageContainer);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }
  
  mImageContainer->SetLoopCount(mGIFStruct.loop_count);
  mImageContainer->DecodingComplete();

  mGIFOpen = PR_FALSE;
}


nsresult nsGIFDecoder2::BeginImageFrame(gfx_depth aDepth)
{
  if (!mGIFStruct.images_decoded) {
    
    
    
    if (mGIFStruct.y_offset > 0) {
      PRInt32 imgWidth;
      mImageContainer->GetWidth(&imgWidth);
      PRUint32 imgCurFrame;
      mImageContainer->GetCurrentFrameIndex(&imgCurFrame);
      nsIntRect r(0, 0, imgWidth, mGIFStruct.y_offset);
      mObserver->OnDataAvailable(nsnull, imgCurFrame == PRUint32(mGIFStruct.images_decoded), &r);
    }
  }

  PRUint32 imageDataLength;
  nsresult rv;
  gfxASurface::gfxImageFormat format;
  if (mGIFStruct.is_transparent)
    format = gfxASurface::ImageFormatARGB32;
  else
    format = gfxASurface::ImageFormatRGB24;

  
  
  if (mGIFStruct.images_decoded) {
    
    rv = mImageContainer->AppendPalettedFrame(mGIFStruct.x_offset, mGIFStruct.y_offset,
                                              mGIFStruct.width, mGIFStruct.height,
                                              format, aDepth, &mImageData, &imageDataLength,
                                              &mColormap, &mColormapSize);
  } else {
    
    rv = mImageContainer->AppendFrame(mGIFStruct.x_offset, mGIFStruct.y_offset,
                                      mGIFStruct.width, mGIFStruct.height,
                                      format, &mImageData, &imageDataLength);
  }

  if (NS_FAILED(rv))
    return rv;

  mImageContainer->SetFrameDisposalMethod(mGIFStruct.images_decoded,
                                          mGIFStruct.disposal_method);

  if (mObserver)
    mObserver->OnStartFrame(nsnull, mGIFStruct.images_decoded);

  mCurrentFrame = mGIFStruct.images_decoded;
  return NS_OK;
}



void nsGIFDecoder2::EndImageFrame()
{
  
  if (!mGIFStruct.images_decoded) {
    
    (void) FlushImageData();

    
    
    
    const PRUint32 realFrameHeight = mGIFStruct.height + mGIFStruct.y_offset;
    if (realFrameHeight < mGIFStruct.screen_height) {
      PRUint32 imgCurFrame;
      mImageContainer->GetCurrentFrameIndex(&imgCurFrame);
      nsIntRect r(0, realFrameHeight, 
                  mGIFStruct.screen_width, 
				  mGIFStruct.screen_height - realFrameHeight);
      mObserver->OnDataAvailable(nsnull, imgCurFrame == PRUint32(mGIFStruct.images_decoded), &r);
    }
    
    if (mGIFStruct.is_transparent && !mSawTransparency) {
      mImageContainer->SetFrameHasNoAlpha(mGIFStruct.images_decoded);
    }
  }
  mCurrentRow = mLastFlushedRow = -1;
  mCurrentPass = mLastFlushedPass = 0;

  PRUint32 curframe = mGIFStruct.images_decoded;

  
  if (mGIFStruct.rows_remaining != mGIFStruct.height) {
    if (mGIFStruct.rows_remaining && mGIFStruct.images_decoded) {
      
      PRUint8 *rowp = mImageData + ((mGIFStruct.height - mGIFStruct.rows_remaining) * mGIFStruct.width);
      memset(rowp, 0, mGIFStruct.rows_remaining * mGIFStruct.width);
    }

    
    
    
    
    mImageContainer->SetFrameTimeout(mGIFStruct.images_decoded, mGIFStruct.delay_time);
    mImageContainer->EndFrameDecode(mGIFStruct.images_decoded);
  }

  
  
  
  
  mGIFStruct.images_decoded++;

  if (mObserver)
    mObserver->OnStopFrame(nsnull, curframe);

  
  if (mOldColor) {
    mColormap[mGIFStruct.tpixel] = mOldColor;
    mOldColor = 0;
  }

  mCurrentFrame = -1;
}




PRUint32 nsGIFDecoder2::OutputRow()
{
  int drow_start, drow_end;
  drow_start = drow_end = mGIFStruct.irow;

  
  if ((PRUintn)drow_start >= mGIFStruct.height) {
    NS_WARNING("GIF2.cpp::OutputRow - too much image data");
    return 0;
  }

  if (!mGIFStruct.images_decoded) {
    





    if (mGIFStruct.progressive_display && mGIFStruct.interlaced && (mGIFStruct.ipass < 4)) {
      
      const PRUint32 row_dup = 15 >> mGIFStruct.ipass;
      const PRUint32 row_shift = row_dup >> 1;
  
      drow_start -= row_shift;
      drow_end = drow_start + row_dup;
  
      
      if (((mGIFStruct.height - 1) - drow_end) <= row_shift)
        drow_end = mGIFStruct.height - 1;
  
      
      if (drow_start < 0)
        drow_start = 0;
      if ((PRUintn)drow_end >= mGIFStruct.height)
        drow_end = mGIFStruct.height - 1;
    }

    
    const PRUint32 bpr = sizeof(PRUint32) * mGIFStruct.width; 
    PRUint8 *rowp = mImageData + (mGIFStruct.irow * bpr);

    
    PRUint8 *from = rowp + mGIFStruct.width;
    PRUint32 *to = ((PRUint32*)rowp) + mGIFStruct.width;
    PRUint32 *cmap = mColormap;
    if (mColorMask == 0xFF) {
      for (PRUint32 c = mGIFStruct.width; c > 0; c--) {
        *--to = cmap[*--from];
      }
    } else {
      
      PRUint8 mask = mColorMask;
      for (PRUint32 c = mGIFStruct.width; c > 0; c--) {
        *--to = cmap[(*--from) & mask];
      }
    }
  
    
    if (mGIFStruct.is_transparent && !mSawTransparency) {
      const PRUint32 *rgb = (PRUint32*)rowp;
      for (PRUint32 i = mGIFStruct.width; i > 0; i--) {
        if (*rgb++ == 0) {
          mSawTransparency = PR_TRUE;
          break;
        }
      }
    }

    
    if (drow_end > drow_start) {
      
      for (int r = drow_start; r <= drow_end; r++) {
        if (r != int(mGIFStruct.irow)) {
          memcpy(mImageData + (r * bpr), rowp, bpr);
        }
      }
    }
  }

  mCurrentRow = drow_end;
  mCurrentPass = mGIFStruct.ipass;
  if (mGIFStruct.ipass == 1)
    mLastFlushedPass = mGIFStruct.ipass;   

  if (!mGIFStruct.interlaced) {
    mGIFStruct.irow++;
  } else {
    static const PRUint8 kjump[5] = { 1, 8, 8, 4, 2 };
    do {
      
      mGIFStruct.irow += kjump[mGIFStruct.ipass];
      if (mGIFStruct.irow >= mGIFStruct.height) {
        
        mGIFStruct.irow = 8 >> mGIFStruct.ipass;
        mGIFStruct.ipass++;
      }
    } while (mGIFStruct.irow >= mGIFStruct.height);
  }

  return --mGIFStruct.rows_remaining;
}



PRBool
nsGIFDecoder2::DoLzw(const PRUint8 *q)
{
  if (!mGIFStruct.rows_remaining)
    return PR_TRUE;

  



  int avail       = mGIFStruct.avail;
  int bits        = mGIFStruct.bits;
  int codesize    = mGIFStruct.codesize;
  int codemask    = mGIFStruct.codemask;
  int count       = mGIFStruct.count;
  int oldcode     = mGIFStruct.oldcode;
  const int clear_code = ClearCode();
  PRUint8 firstchar = mGIFStruct.firstchar;
  PRInt32 datum     = mGIFStruct.datum;
  PRUint16 *prefix  = mGIFStruct.prefix;
  PRUint8 *stackp   = mGIFStruct.stackp;
  PRUint8 *suffix   = mGIFStruct.suffix;
  PRUint8 *stack    = mGIFStruct.stack;
  PRUint8 *rowp     = mGIFStruct.rowp;

  PRUint32 bpr = mGIFStruct.width;
  if (!mGIFStruct.images_decoded) 
    bpr *= sizeof(PRUint32);
  PRUint8 *rowend   = mImageData + (bpr * mGIFStruct.irow) + mGIFStruct.width;

#define OUTPUT_ROW()                                        \
  PR_BEGIN_MACRO                                            \
    if (!OutputRow())                                       \
      goto END;                                             \
    rowp = mImageData + mGIFStruct.irow * bpr;              \
    rowend = rowp + mGIFStruct.width;                       \
  PR_END_MACRO

  for (const PRUint8* ch = q; count-- > 0; ch++)
  {
    
    datum += ((int32) *ch) << bits;
    bits += 8;

    
    while (bits >= codesize)
    {
      
      int code = datum & codemask;
      datum >>= codesize;
      bits -= codesize;

      
      if (code == clear_code) {
        codesize = mGIFStruct.datasize + 1;
        codemask = (1 << codesize) - 1;
        avail = clear_code + 2;
        oldcode = -1;
        continue;
      }

      
      if (code == (clear_code + 1)) {
        
        return (mGIFStruct.rows_remaining == 0);
      }

      if (oldcode == -1) {
        if (code >= MAX_BITS)
          return PR_FALSE;
        *rowp++ = suffix[code];
        if (rowp == rowend)
          OUTPUT_ROW();

        firstchar = oldcode = code;
        continue;
      }

      int incode = code;
      if (code >= avail) {
        *stackp++ = firstchar;
        code = oldcode;

        if (stackp >= stack + MAX_BITS)
          return PR_FALSE;
      }

      while (code >= clear_code)
      {
        if ((code >= MAX_BITS) || (code == prefix[code]))
          return PR_FALSE;

        *stackp++ = suffix[code];
        code = prefix[code];

        if (stackp == stack + MAX_BITS)
          return PR_FALSE;
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
        if (rowp == rowend)
          OUTPUT_ROW();
      } while (stackp > stack);
    }
  }

  END:

  
  mGIFStruct.avail = avail;
  mGIFStruct.bits = bits;
  mGIFStruct.codesize = codesize;
  mGIFStruct.codemask = codemask;
  mGIFStruct.count = count;
  mGIFStruct.oldcode = oldcode;
  mGIFStruct.firstchar = firstchar;
  mGIFStruct.datum = datum;
  mGIFStruct.stackp = stackp;
  mGIFStruct.rowp = rowp;

  return PR_TRUE;
}





static void ConvertColormap(PRUint32 *aColormap, PRUint32 aColors)
{
  
  if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
    qcms_transform *transform = gfxPlatform::GetCMSRGBTransform();
    if (transform)
      qcms_transform_data(transform, aColormap, aColormap, aColors);
  }
  
  
  PRUint8 *from = ((PRUint8 *)aColormap) + 3 * aColors;
  PRUint32 *to = aColormap + aColors;

  

  
  if (!aColors) return;
  PRUint32 c = aColors;

  
  
  for (; (NS_PTR_TO_UINT32(from) & 0x3) && c; --c) {
    from -= 3;
    *--to = GFX_PACKED_PIXEL(0xFF, from[0], from[1], from[2]);
  }

  
  while (c >= 4) {
    from -= 12;
    to   -=  4;
    c    -=  4;
    GFX_BLOCK_RGB_TO_FRGB(from,to);
  }

  
  
  while (c--) {
    from -= 3;
    *--to = GFX_PACKED_PIXEL(0xFF, from[0], from[1], from[2]);
  }
}






nsresult nsGIFDecoder2::GifWrite(const PRUint8 *buf, PRUint32 len)
{
  if (!buf || !len)
    return NS_ERROR_FAILURE;

  const PRUint8 *q = buf;

  
  
  
  PRUint8* p = (mGIFStruct.state == gif_global_colormap) ? (PRUint8*)mGIFStruct.global_colormap :
               (mGIFStruct.state == gif_image_colormap) ? (PRUint8*)mColormap :
               (mGIFStruct.bytes_in_hold) ? mGIFStruct.hold : nsnull;
  if (p) {
    
    PRUint32 l = PR_MIN(len, mGIFStruct.bytes_to_consume);
    memcpy(p+mGIFStruct.bytes_in_hold, buf, l);

    if (l < mGIFStruct.bytes_to_consume) {
      
      mGIFStruct.bytes_in_hold += l;
      mGIFStruct.bytes_to_consume -= l;
      return NS_OK;
    }
    
    mGIFStruct.bytes_in_hold = 0;
    
    q = p;
  }

  
  
  
  
  
  
  
  

  for (;len >= mGIFStruct.bytes_to_consume; q=buf) {
    
    buf += mGIFStruct.bytes_to_consume;
    len -= mGIFStruct.bytes_to_consume;

    switch (mGIFStruct.state)
    {
    case gif_lzw:
      if (!DoLzw(q)) {
        mGIFStruct.state = gif_error;
        break;
      }
      GETN(1, gif_sub_block);
      break;

    case gif_lzw_start:
    {
      
      if (mGIFStruct.is_transparent) {
        
        if (mColormap == mGIFStruct.global_colormap)
            mOldColor = mColormap[mGIFStruct.tpixel];
        mColormap[mGIFStruct.tpixel] = 0;
      }

      
      mGIFStruct.datasize = *q;
      const int clear_code = ClearCode();
      if (mGIFStruct.datasize > MAX_LZW_BITS ||
          clear_code >= MAX_BITS) {
        mGIFStruct.state = gif_error;
        break;
      }

      mGIFStruct.avail = clear_code + 2;
      mGIFStruct.oldcode = -1;
      mGIFStruct.codesize = mGIFStruct.datasize + 1;
      mGIFStruct.codemask = (1 << mGIFStruct.codesize) - 1;
      mGIFStruct.datum = mGIFStruct.bits = 0;

      
      for (int i = 0; i < clear_code; i++)
        mGIFStruct.suffix[i] = i;

      mGIFStruct.stackp = mGIFStruct.stack;

      GETN(1, gif_sub_block);
    }
    break;

    
    case gif_type:
      if (!strncmp((char*)q, "GIF89a", 6)) {
        mGIFStruct.version = 89;
      } else if (!strncmp((char*)q, "GIF87a", 6)) {
        mGIFStruct.version = 87;
      } else {
        mGIFStruct.state = gif_error;
        break;
      }
      GETN(7, gif_global_header);
      break;

    case gif_global_header:
      






      mGIFStruct.screen_width = GETINT16(q);
      mGIFStruct.screen_height = GETINT16(q + 2);
      mGIFStruct.global_colormap_depth = (q[4]&0x07) + 1;

      
      
      
      
      

      if (q[4] & 0x80) { 
        
        const PRUint32 size = (3 << mGIFStruct.global_colormap_depth);
        if (len < size) {
          
          GETN(size, gif_global_colormap);
          break;
        }
        
        memcpy(mGIFStruct.global_colormap, buf, size);
        buf += size;
        len -= size;
        GETN(0, gif_global_colormap);
        break;
      }

      GETN(1, gif_image_start);
      break;

    case gif_global_colormap:
      
      
      ConvertColormap(mGIFStruct.global_colormap, 1<<mGIFStruct.global_colormap_depth);
      GETN(1, gif_image_start);
      break;

    case gif_image_start:
      switch (*q) {
        case GIF_TRAILER:
          mGIFStruct.state = gif_done;
          break;

        case GIF_EXTENSION_INTRODUCER:
          GETN(2, gif_extension);
          break;

        case GIF_IMAGE_SEPARATOR:
          GETN(9, gif_image_header);
          break;

        default:
          




          if (mGIFStruct.images_decoded > 0) {
            




            mGIFStruct.state = gif_done;
          } else {
            
            mGIFStruct.state = gif_error;
          }
      }
      break;

    case gif_extension:
      mGIFStruct.bytes_to_consume = q[1];
      if (mGIFStruct.bytes_to_consume) {
        switch (*q) {
        case GIF_GRAPHIC_CONTROL_LABEL:
          mGIFStruct.state = gif_control_extension;
          break;
  
        case GIF_APPLICATION_EXTENSION_LABEL:
          mGIFStruct.state = gif_application_extension;
          break;
  
        case GIF_COMMENT_LABEL:
          mGIFStruct.state = gif_consume_comment;
          break;
  
        default:
          mGIFStruct.state = gif_skip_block;
        }
      } else {
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
      mGIFStruct.is_transparent = *q & 0x1;
      mGIFStruct.tpixel = q[3];
      mGIFStruct.disposal_method = ((*q) >> 2) & 0x7;
      
      
      if (mGIFStruct.disposal_method == 4)
        mGIFStruct.disposal_method = 3;
      mGIFStruct.delay_time = GETINT16(q + 1) * 10;
      GETN(1, gif_consume_block);
      break;

    case gif_comment_extension:
      if (*q)
        GETN(*q, gif_consume_comment);
      else
        GETN(1, gif_image_start);
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
      switch (q[0] & 7) {
        case 1:
          

          mGIFStruct.loop_count = GETINT16(q + 1);
  
          
          if (mGIFStruct.loop_count == 0)
            mGIFStruct.loop_count = -1;
  
          GETN(1, gif_netscape_extension_block);
          break;
        
        case 2:
          
          
          
          
          GETN(1, gif_netscape_extension_block);
          break;
  
        default:
          
          mGIFStruct.state = gif_error;
      }
      break;

    case gif_image_header:
    {
      
      mGIFStruct.x_offset = GETINT16(q);
      mGIFStruct.y_offset = GETINT16(q + 2);

      
      mGIFStruct.width  = GETINT16(q + 4);
      mGIFStruct.height = GETINT16(q + 6);

      if (!mGIFStruct.images_decoded) {
        



        if ((mGIFStruct.screen_height < mGIFStruct.height) ||
            (mGIFStruct.screen_width < mGIFStruct.width) ||
            (mGIFStruct.version == 87)) {
          mGIFStruct.screen_height = mGIFStruct.height;
          mGIFStruct.screen_width = mGIFStruct.width;
          mGIFStruct.x_offset = 0;
          mGIFStruct.y_offset = 0;
        }    
        
        BeginGIF();
      }

      

      if (!mGIFStruct.height || !mGIFStruct.width) {
        mGIFStruct.height = mGIFStruct.screen_height;
        mGIFStruct.width = mGIFStruct.screen_width;
        if (!mGIFStruct.height || !mGIFStruct.width) {
          mGIFStruct.state = gif_error;
          break;
        }
      }

      
      
      
      PRUint32 depth = mGIFStruct.global_colormap_depth;
      if (q[8] & 0x80)
        depth = (q[8]&0x07) + 1;
      
      if (mGIFStruct.tpixel >= (1 << depth)) {
        mGIFStruct.is_transparent = PR_FALSE;
        mGIFStruct.tpixel = 0;
      }
      
      mColorMask = 0xFF >> (8 - depth);
      nsresult rv = BeginImageFrame(depth);
      if (NS_FAILED(rv) || !mImageData) {
        mGIFStruct.state = gif_error;
        break;
      }

      if (q[8] & 0x40) {
        mGIFStruct.interlaced = PR_TRUE;
        mGIFStruct.ipass = 1;
      } else {
        mGIFStruct.interlaced = PR_FALSE;
        mGIFStruct.ipass = 0;
      }

      
      mGIFStruct.progressive_display = (mGIFStruct.images_decoded == 0);

      
      mGIFStruct.irow = 0;
      mGIFStruct.rows_remaining = mGIFStruct.height;
      mGIFStruct.rowp = mImageData;

      

      if (q[8] & 0x80) 
      {
        mGIFStruct.local_colormap_size = 1 << depth;
        if (!mGIFStruct.images_decoded) {
          
          
          mColormapSize = sizeof(PRUint32) << depth;
          if (!mGIFStruct.local_colormap) {
            mGIFStruct.local_colormap = (PRUint32*)PR_MALLOC(mColormapSize);
            if (!mGIFStruct.local_colormap) {
              mGIFStruct.state = gif_oom;
              break;
            }
          }
          mColormap = mGIFStruct.local_colormap;
        }
        const PRUint32 size = 3 << depth;
        if (mColormapSize > size) {
          
          memset(((PRUint8*)mColormap) + size, 0, mColormapSize - size);
        }
        if (len < size) {
          
          GETN(size, gif_image_colormap);
          break;
        }
        
        memcpy(mColormap, buf, size);
        buf += size;
        len -= size;
        GETN(0, gif_image_colormap);
        break;
      } else {
        
        if (mGIFStruct.images_decoded) {
          
          memcpy(mColormap, mGIFStruct.global_colormap, mColormapSize);
        } else {
          mColormap = mGIFStruct.global_colormap;
        }
      }
      GETN(1, gif_lzw_start);
    }
    break;

    case gif_image_colormap:
      
      
      ConvertColormap(mColormap, mGIFStruct.local_colormap_size);
      GETN(1, gif_lzw_start);
      break;

    case gif_sub_block:
      mGIFStruct.count = *q;
      if (mGIFStruct.count) {
        
        
        
        if (!mGIFStruct.rows_remaining) {
#ifdef DONT_TOLERATE_BROKEN_GIFS
          mGIFStruct.state = gif_error;
          break;
#else
          
          GETN(1, gif_sub_block);
#endif
          if (mGIFStruct.count == GIF_TRAILER) {
            
            GETN(1, gif_done);
            break;
          }
        }
        GETN(mGIFStruct.count, gif_lzw);
      } else {
        
        EndImageFrame();
        GETN(1, gif_image_start);
      }
      break;

    case gif_done:
      EndGIF();
      return NS_OK;
      break;

    case gif_error:
      EndGIF();
      return NS_ERROR_FAILURE;
      break;

    
    case gif_oom:
      return NS_ERROR_OUT_OF_MEMORY;

    
    default:
      break;
    }
  }

  
  if (mGIFStruct.state == gif_error) {
      EndGIF();
      return NS_ERROR_FAILURE;
  }
  
  
  mGIFStruct.bytes_in_hold = len;
  if (len) {
    
    PRUint8* p = (mGIFStruct.state == gif_global_colormap) ? (PRUint8*)mGIFStruct.global_colormap :
                 (mGIFStruct.state == gif_image_colormap) ? (PRUint8*)mColormap :
                 mGIFStruct.hold;
    memcpy(p, buf, len);
    mGIFStruct.bytes_to_consume -= len;
  }

  return NS_OK;
}
