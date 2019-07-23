








































































#include <stddef.h>
#include "prtypes.h"
#include "prmem.h"
#include "prlog.h"
#include "GIF2.h"

#include "nsGIFDecoder2.h"
#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "imgIContainerObserver.h"

#include "imgILoad.h"

#include "imgContainer.h"










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
  , mRGBLine(nsnull)
  , mRGBLineMaxSize(0)
  , mCurrentPass(0)
  , mLastFlushedPass(0)
  , mGIFOpen(PR_FALSE)
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

  mImageContainer = do_CreateInstance("@mozilla.org/image/container;1");
  aLoad->SetImage(mImageContainer);
  
  
  mGIFStruct.state = gif_type;
  mGIFStruct.bytes_to_consume = 6;

  return NS_OK;
}








NS_IMETHODIMP nsGIFDecoder2::Close()
{
  if (mImageFrame) 
    EndImageFrame();
  EndGIF();

  PR_FREEIF(mGIFStruct.rowbuf);
  PR_FREEIF(mGIFStruct.local_colormap);
  PR_FREEIF(mRGBLine);

  return NS_OK;
}



NS_IMETHODIMP nsGIFDecoder2::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, closure);
  nsresult rv = decoder->ProcessData((unsigned char*)fromRawSegment, count, writeCount);
  if (NS_FAILED(rv)) {
    *writeCount = 0;
    return rv;
  }

  return NS_OK;
}




void
nsGIFDecoder2::FlushImageData()
{
  PRInt32 imgWidth;
  mImageContainer->GetWidth(&imgWidth);
  nsIntRect frameRect;
  mImageFrame->GetRect(frameRect);
  
  switch (mCurrentPass - mLastFlushedPass) {
    case 0: {  
      PRInt32 remainingRows = mCurrentRow - mLastFlushedRow;
      if (remainingRows) {
        nsIntRect r(0, frameRect.y + mLastFlushedRow + 1,
                    imgWidth, remainingRows);
        mObserver->OnDataAvailable(nsnull, mImageFrame, &r);
      }    
    }
    break;
  
    case 1: {  
      nsIntRect r(0, frameRect.y, imgWidth, mCurrentRow + 1);
      mObserver->OnDataAvailable(nsnull, mImageFrame, &r);
      nsIntRect r2(0, frameRect.y + mLastFlushedRow + 1,
                   imgWidth, frameRect.height - mLastFlushedRow - 1);
      mObserver->OnDataAvailable(nsnull, mImageFrame, &r2);
    }
    break;

    default: {  
      nsIntRect r(0, frameRect.y, imgWidth, frameRect.height);
      mObserver->OnDataAvailable(nsnull, mImageFrame, &r);
    }
  }
}


nsresult nsGIFDecoder2::ProcessData(unsigned char *data, PRUint32 count, PRUint32 *_retval)
{
  
  
  nsresult rv = GifWrite(data, count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mImageFrame && mObserver) {
    FlushImageData();
    mLastFlushedRow = mCurrentRow;
    mLastFlushedPass = mCurrentPass;
  }

  *_retval = count;

  return NS_OK;
}



NS_IMETHODIMP nsGIFDecoder2::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  nsresult rv = inStr->ReadSegments(ReadDataOut, this,  count, _retval);

  


  if (NS_SUCCEEDED(rv) && mGIFStruct.state == gif_error) {
    PRUint32 numFrames = 0;
    if (mImageContainer)
      mImageContainer->GetNumFrames(&numFrames);
    if (numFrames <= 0)
      return NS_ERROR_FAILURE;
  }

  return rv;
}







void nsGIFDecoder2::BeginGIF()
{
  
  
  if (mGIFStruct.screen_width == 0 || mGIFStruct.screen_height == 0)
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


void nsGIFDecoder2::BeginImageFrame()
{
  mImageFrame = nsnull; 

  if (!mGIFStruct.images_decoded) {
    
    
    
    if (mGIFStruct.y_offset > 0) {
      PRInt32 imgWidth;
      mImageContainer->GetWidth(&imgWidth);
      nsIntRect r(0, 0, imgWidth, mGIFStruct.y_offset);
      mObserver->OnDataAvailable(nsnull, mImageFrame, &r);
    }
  }
}


void nsGIFDecoder2::EndImageFrame()
{
  
  
  if (mGIFStruct.delay_time < MINIMUM_DELAY_TIME)
    mGIFStruct.delay_time = MINIMUM_DELAY_TIME;

  mGIFStruct.images_decoded++;

  
  
  
  if (!mImageFrame) {
    HaveDecodedRow(nsnull,0,0,0);
  } else {
    
    
    
    
    mImageFrame->SetTimeout(mGIFStruct.delay_time);
  }
  mImageContainer->EndFrameDecode(mGIFStruct.images_decoded, mGIFStruct.delay_time);

  if (mObserver && mImageFrame) {
    FlushImageData();

    if (mGIFStruct.images_decoded == 1) {
      
      
      
      PRInt32 imgHeight;
      PRInt32 realFrameHeight = mGIFStruct.height + mGIFStruct.y_offset;

      mImageContainer->GetHeight(&imgHeight);
      if (imgHeight > realFrameHeight) {
        PRInt32 imgWidth;
        mImageContainer->GetWidth(&imgWidth);

        nsIntRect r(0, realFrameHeight, imgWidth, imgHeight - realFrameHeight);
        mObserver->OnDataAvailable(nsnull, mImageFrame, &r);
      }
    }

    mCurrentRow = mLastFlushedRow = -1;
    mCurrentPass = mLastFlushedPass = 0;

    mObserver->OnStopFrame(nsnull, mImageFrame);
  }

  
  mImageFrame = nsnull;
  mGIFStruct.is_local_colormap_defined = PR_FALSE;
  mGIFStruct.is_transparent = PR_FALSE;
}
  


void nsGIFDecoder2::HaveDecodedRow(
  PRUint8* aRowBufPtr,   
  int aRowNumber,        
  int aDuplicateCount,   
  int aInterlacePass)    
{
  const PRUint32 bpr = mGIFStruct.width * sizeof(PRUint32);

  
  
  
  
  
  if (!mImageFrame) {
    gfx_format format = gfxIFormats::RGB;
    if (mGIFStruct.is_transparent) {
      format = gfxIFormats::RGB_A1;  
    }

    
    mImageFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
    if (!mImageFrame || NS_FAILED(mImageFrame->Init(
          mGIFStruct.x_offset, mGIFStruct.y_offset, 
          mGIFStruct.width, mGIFStruct.height, format, 24))) {
      mImageFrame = 0;
      return;
    }

    mImageFrame->SetFrameDisposalMethod(mGIFStruct.disposal_method);
    mImageContainer->AppendFrame(mImageFrame);

    if (mObserver)
      mObserver->OnStartFrame(nsnull, mImageFrame);

    if (bpr > mRGBLineMaxSize) {
      mRGBLine = (PRUint8 *)PR_REALLOC(mRGBLine, bpr);
      mRGBLineMaxSize = bpr;
    }
  }
  
  if (aRowBufPtr) {
    
    int cmapsize = mGIFStruct.global_colormap_size;
    PRUint8* cmap = mGIFStruct.global_colormap;
    if (mGIFStruct.is_local_colormap_defined) {
      cmapsize = mGIFStruct.local_colormap_size;
      cmap = mGIFStruct.local_colormap;
    }

    if (!cmap) { 
      nsIntRect r(0, aRowNumber, mGIFStruct.width, aDuplicateCount);
      imgContainer::ClearFrame(mImageFrame, r);
    } else {
      PRUint8* rowBufIndex = aRowBufPtr;
      PRUint32* rgbRowIndex = (PRUint32*)mRGBLine;

      const PRInt32 tpixel = 
        mGIFStruct.is_transparent ? mGIFStruct.tpixel : -1;

      while (rowBufIndex != mGIFStruct.rowend) {
        if (*rowBufIndex >= cmapsize || *rowBufIndex == tpixel) {
          *rgbRowIndex++ = 0x00000000;
          ++rowBufIndex;
          continue;
        }

        PRUint32 colorIndex = *rowBufIndex * 3;
        *rgbRowIndex++ = (0xFF << 24) |
          (cmap[colorIndex] << 16) |
          (cmap[colorIndex+1] << 8) |
          (cmap[colorIndex+2]);
        ++rowBufIndex;
      }
      for (int i=0; i<aDuplicateCount; i++)
        mImageFrame->SetImageData(mRGBLine, bpr, (aRowNumber+i)*bpr);
    }

    mCurrentRow = aRowNumber + aDuplicateCount - 1;
    mCurrentPass = aInterlacePass;
    if (aInterlacePass == 1)
      mLastFlushedPass = aInterlacePass;   
  }
}




PRUint32 nsGIFDecoder2::OutputRow()
{
  int width, drow_start, drow_end;

  drow_start = drow_end = mGIFStruct.irow;

  





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

  
  if ((PRUintn)drow_start >= mGIFStruct.height) {
    NS_WARNING("GIF2.cpp::OutputRow - too much image data");
    return 0;
  }

  
  if ((mGIFStruct.y_offset + mGIFStruct.irow) < mGIFStruct.screen_height) {
    
    if ((mGIFStruct.x_offset + mGIFStruct.width) > mGIFStruct.screen_width)
      width = mGIFStruct.screen_width - mGIFStruct.x_offset;
    else
      width = mGIFStruct.width;

    if (width > 0)
      
      HaveDecodedRow(
        mGIFStruct.rowbuf,                
        drow_start,                
        drow_end - drow_start + 1, 
        mGIFStruct.ipass);                
  }

  mGIFStruct.rowp = mGIFStruct.rowbuf;

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
  PRUint8 *rowend   = mGIFStruct.rowend;

  if (rowp == rowend)
    return PR_TRUE;

#define OUTPUT_ROW()                                        \
  PR_BEGIN_MACRO                                            \
    if (!OutputRow())                                       \
      goto END;                                             \
    rowp = mGIFStruct.rowp;                                 \
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

        if (stackp == stack + MAX_BITS)
          return PR_FALSE;
      }

      while (code >= clear_code)
      {
        if (code == prefix[code])
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






nsresult nsGIFDecoder2::GifWrite(const PRUint8 *buf, PRUint32 len)
{
  if (!buf || !len)
    return NS_ERROR_FAILURE;

  const PRUint8 *q = buf;

  
  
  
  PRUint8* p = (mGIFStruct.state == gif_global_colormap) ? mGIFStruct.global_colormap :
               (mGIFStruct.state == gif_image_colormap) ? mGIFStruct.local_colormap :
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
      mGIFStruct.global_colormap_size = 2<<(q[4]&0x07);

      
      
      
      
      

      
      BeginGIF();

      if (q[4] & 0x80) { 
        
        const PRUint32 size = 3*mGIFStruct.global_colormap_size;
        if (len < size) {
          
          GETN(size, gif_global_colormap);
          break;
        }
        
        memcpy(mGIFStruct.global_colormap, buf, size);
        buf += size;
        len -= size;
      }

      GETN(1, gif_image_start);
      break;

    case gif_global_colormap:
      
      GETN(1, gif_image_start);
      break;

    case gif_image_start:
      switch (*q) {
        case ';':  
          mGIFStruct.state = gif_done;
          break;

        case '!': 
          GETN(2, gif_extension);
          break;

        case ',':
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
        case 0xf9:
          mGIFStruct.state = gif_control_extension;
          break;
  
        case 0xff:
          mGIFStruct.state = gif_application_extension;
          break;
  
        case 0xfe:
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
      
      mGIFStruct.x_offset = GETINT16(q);
      mGIFStruct.y_offset = GETINT16(q + 2);

      
      mGIFStruct.width  = GETINT16(q + 4);
      mGIFStruct.height = GETINT16(q + 6);

      



      if (!mGIFStruct.images_decoded &&
          ((mGIFStruct.screen_height < mGIFStruct.height) ||
           (mGIFStruct.screen_width < mGIFStruct.width) ||
           (mGIFStruct.version == 87)))
      {
        mGIFStruct.screen_height = mGIFStruct.height;
        mGIFStruct.screen_width = mGIFStruct.width;
        mGIFStruct.x_offset = 0;
        mGIFStruct.y_offset = 0;

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

      BeginImageFrame();

      
      
      
      if (mGIFStruct.screen_width < mGIFStruct.width) {
        

        mGIFStruct.rowbuf = (PRUint8*)PR_REALLOC(mGIFStruct.rowbuf, mGIFStruct.width);
        mGIFStruct.screen_width = mGIFStruct.width;
      } else if (!mGIFStruct.rowbuf) {
          mGIFStruct.rowbuf = (PRUint8*)PR_MALLOC(mGIFStruct.screen_width);
      }

      if (!mGIFStruct.rowbuf) {
          mGIFStruct.state = gif_oom;
          break;
      }
      if (mGIFStruct.screen_height < mGIFStruct.height)
        mGIFStruct.screen_height = mGIFStruct.height;

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
      mGIFStruct.rowend = mGIFStruct.rowbuf + mGIFStruct.width;
      mGIFStruct.rowp = mGIFStruct.rowbuf;

      

      if (q[8] & 0x80) 
      {
        const int num_colors = 2 << (q[8] & 0x7);
        const PRUint32 size = 3*num_colors;
        PRUint8 *map = mGIFStruct.local_colormap;
        if (!map || (num_colors > mGIFStruct.local_colormap_size)) {
          map = (PRUint8*)PR_REALLOC(map, size);
          if (!map) {
            mGIFStruct.state = gif_oom;
            break;
          }
          mGIFStruct.local_colormap = map;
        }

        
        mGIFStruct.local_colormap_size = num_colors;
        mGIFStruct.is_local_colormap_defined = PR_TRUE;

        if (len < size) {
          
          GETN(size, gif_image_colormap);
          break;
        }
        
        memcpy(mGIFStruct.local_colormap, buf, size);
        buf += size;
        len -= size;
      } else {
        
        mGIFStruct.is_local_colormap_defined = PR_FALSE;
      }
      GETN(1, gif_lzw_start);
      break;

    case gif_image_colormap:
      
      GETN(1, gif_lzw_start);
      break;

    case gif_sub_block:
      mGIFStruct.count = *q;
      if (mGIFStruct.count) {
        
        
        
        if (!mGIFStruct.rows_remaining) {
#ifdef DONT_TOLERATE_BROKEN_GIFS
          mGIFStruct.state = gif_error;
#else
          
          GETN(1, gif_sub_block);
#endif
          break;
        }
        GETN(mGIFStruct.count, gif_lzw);
      } else {
        
        EndImageFrame();
        GETN(1, gif_image_start);
      }
      break;

    case gif_done:
    case gif_error:
      EndGIF();
      return NS_OK;
      break;

    
    case gif_oom:
      return NS_ERROR_OUT_OF_MEMORY;

    
    default:
      break;
    }
  }

  
  mGIFStruct.bytes_in_hold = len;
  if (len) {
    
    PRUint8* p = (mGIFStruct.state == gif_global_colormap) ? mGIFStruct.global_colormap :
                 (mGIFStruct.state == gif_image_colormap) ? mGIFStruct.local_colormap :
                 mGIFStruct.hold;
    memcpy(p, buf, len);
    mGIFStruct.bytes_to_consume -= len;
  }

  return NS_OK;
}
