







































#include <stddef.h>

#include "nsGIFDecoder2.h"
#include "nsIInputStream.h"
#include "RasterImage.h"

#include "gfxColor.h"
#include "gfxPlatform.h"
#include "qcms.h"
#include <algorithm>
#include "mozilla/Telemetry.h"

namespace mozilla {
namespace image {






#define GETN(n,s)                      \
  PR_BEGIN_MACRO                       \
    mGIFStruct.bytes_to_consume = (n); \
    mGIFStruct.state = (s);            \
  PR_END_MACRO


#define GETINT16(p)   ((p)[1]<<8|(p)[0])



nsGIFDecoder2::nsGIFDecoder2(RasterImage* aImage)
  : Decoder(aImage)
  , mCurrentRow(-1)
  , mLastFlushedRow(-1)
  , mOldColor(0)
  , mCurrentFrameIndex(-1)
  , mCurrentPass(0)
  , mLastFlushedPass(0)
  , mGIFOpen(false)
  , mSawTransparency(false)
{
  
  memset(&mGIFStruct, 0, sizeof(mGIFStruct));

  
  mGIFStruct.loop_count = 1;

  
  mGIFStruct.state = gif_type;
  mGIFStruct.bytes_to_consume = 6;
}

nsGIFDecoder2::~nsGIFDecoder2()
{
  free(mGIFStruct.local_colormap);
  free(mGIFStruct.hold);
}

void
nsGIFDecoder2::FinishInternal()
{
  MOZ_ASSERT(!HasError(), "Shouldn't call FinishInternal after error!");

  
  if (!IsSizeDecode() && mGIFOpen) {
    if (mCurrentFrameIndex == mGIFStruct.images_decoded) {
      EndImageFrame();
    }
    PostDecodeDone(mGIFStruct.loop_count - 1);
    mGIFOpen = false;
  }
}




void
nsGIFDecoder2::FlushImageData(uint32_t fromRow, uint32_t rows)
{
  nsIntRect r(mGIFStruct.x_offset, mGIFStruct.y_offset + fromRow,
              mGIFStruct.width, rows);
  PostInvalidation(r);
}

void
nsGIFDecoder2::FlushImageData()
{
  switch (mCurrentPass - mLastFlushedPass) {
    case 0:  
      if (mCurrentRow - mLastFlushedRow) {
        FlushImageData(mLastFlushedRow + 1, mCurrentRow - mLastFlushedRow);
      }
      break;

    case 1:  
      FlushImageData(0, mCurrentRow + 1);
      FlushImageData(mLastFlushedRow + 1,
                     mGIFStruct.height - (mLastFlushedRow + 1));
      break;

    default: 
      FlushImageData(0, mGIFStruct.height);
  }
}






void
nsGIFDecoder2::BeginGIF()
{
  if (mGIFOpen) {
    return;
  }

  mGIFOpen = true;

  PostSize(mGIFStruct.screen_width, mGIFStruct.screen_height);
}


void
nsGIFDecoder2::BeginImageFrame(uint16_t aDepth)
{
  MOZ_ASSERT(HasSize());

  gfx::SurfaceFormat format;
  if (mGIFStruct.is_transparent) {
    format = gfx::SurfaceFormat::B8G8R8A8;
    PostHasTransparency();
  } else {
    format = gfx::SurfaceFormat::B8G8R8X8;
  }

  
  
  if (mGIFStruct.images_decoded) {
    
    NeedNewFrame(mGIFStruct.images_decoded, mGIFStruct.x_offset,
                 mGIFStruct.y_offset, mGIFStruct.width, mGIFStruct.height,
                 format, aDepth);
  } else {
    nsRefPtr<imgFrame> currentFrame = GetCurrentFrame();

    
    
    if (!currentFrame->GetRect().IsEqualEdges(nsIntRect(mGIFStruct.x_offset,
                                                        mGIFStruct.y_offset,
                                                        mGIFStruct.width,
                                                        mGIFStruct.height))) {

      
      
      PostHasTransparency();

      
      NeedNewFrame(mGIFStruct.images_decoded, mGIFStruct.x_offset,
                   mGIFStruct.y_offset, mGIFStruct.width, mGIFStruct.height,
                   format);
    }
  }

  mCurrentFrameIndex = mGIFStruct.images_decoded;
}



void
nsGIFDecoder2::EndImageFrame()
{
  Opacity opacity = Opacity::SOME_TRANSPARENCY;

  
  if (!mGIFStruct.images_decoded) {
    
    FlushImageData();

    
    
    
    const uint32_t realFrameHeight = mGIFStruct.height + mGIFStruct.y_offset;
    if (realFrameHeight < mGIFStruct.screen_height) {
      nsIntRect r(0, realFrameHeight,
                  mGIFStruct.screen_width,
                  mGIFStruct.screen_height - realFrameHeight);
      PostInvalidation(r);
    }

    
    
    
    
    if (!mGIFStruct.is_transparent || !mSawTransparency) {
      opacity = Opacity::OPAQUE;
    }
  }
  mCurrentRow = mLastFlushedRow = -1;
  mCurrentPass = mLastFlushedPass = 0;

  
  if (mGIFStruct.rows_remaining != mGIFStruct.height) {
    if (mGIFStruct.rows_remaining && mGIFStruct.images_decoded) {
      
      uint8_t* rowp =
        mImageData + ((mGIFStruct.height - mGIFStruct.rows_remaining) *
                      mGIFStruct.width);
      memset(rowp, 0, mGIFStruct.rows_remaining * mGIFStruct.width);
    }
  }

  
  
  
  
  mGIFStruct.images_decoded++;

  
  PostFrameStop(opacity,
                DisposalMethod(mGIFStruct.disposal_method),
                mGIFStruct.delay_time);

  
  if (mOldColor) {
    mColormap[mGIFStruct.tpixel] = mOldColor;
    mOldColor = 0;
  }

  mCurrentFrameIndex = -1;
}




uint32_t
nsGIFDecoder2::OutputRow()
{
  int drow_start, drow_end;
  drow_start = drow_end = mGIFStruct.irow;

  
  if ((unsigned)drow_start >= mGIFStruct.height) {
    NS_WARNING("GIF2.cpp::OutputRow - too much image data");
    return 0;
  }

  if (!mGIFStruct.images_decoded) {
    
    
    
    
    if (mGIFStruct.progressive_display && mGIFStruct.interlaced &&
        (mGIFStruct.ipass < 4)) {
      
      const uint32_t row_dup = 15 >> mGIFStruct.ipass;
      const uint32_t row_shift = row_dup >> 1;

      drow_start -= row_shift;
      drow_end = drow_start + row_dup;

      
      if (((mGIFStruct.height - 1) - drow_end) <= row_shift) {
        drow_end = mGIFStruct.height - 1;
      }

      
      if (drow_start < 0) {
        drow_start = 0;
      }
      if ((unsigned)drow_end >= mGIFStruct.height) {
        drow_end = mGIFStruct.height - 1;
      }
    }

    
    const uint32_t bpr = sizeof(uint32_t) * mGIFStruct.width;
    uint8_t* rowp = mImageData + (mGIFStruct.irow * bpr);

    
    uint8_t* from = rowp + mGIFStruct.width;
    uint32_t* to = ((uint32_t*)rowp) + mGIFStruct.width;
    uint32_t* cmap = mColormap;
    for (uint32_t c = mGIFStruct.width; c > 0; c--) {
      *--to = cmap[*--from];
    }

    
    if (mGIFStruct.is_transparent && !mSawTransparency) {
      const uint32_t* rgb = (uint32_t*)rowp;
      for (uint32_t i = mGIFStruct.width; i > 0; i--) {
        if (*rgb++ == 0) {
          mSawTransparency = true;
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
  if (mGIFStruct.ipass == 1) {
    mLastFlushedPass = mGIFStruct.ipass;   
  }

  if (!mGIFStruct.interlaced) {
    mGIFStruct.irow++;
  } else {
    static const uint8_t kjump[5] = { 1, 8, 8, 4, 2 };
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



bool
nsGIFDecoder2::DoLzw(const uint8_t* q)
{
  if (!mGIFStruct.rows_remaining) {
    return true;
  }

  
  
  
  int avail       = mGIFStruct.avail;
  int bits        = mGIFStruct.bits;
  int codesize    = mGIFStruct.codesize;
  int codemask    = mGIFStruct.codemask;
  int count       = mGIFStruct.count;
  int oldcode     = mGIFStruct.oldcode;
  const int clear_code = ClearCode();
  uint8_t firstchar = mGIFStruct.firstchar;
  int32_t datum     = mGIFStruct.datum;
  uint16_t* prefix  = mGIFStruct.prefix;
  uint8_t* stackp   = mGIFStruct.stackp;
  uint8_t* suffix   = mGIFStruct.suffix;
  uint8_t* stack    = mGIFStruct.stack;
  uint8_t* rowp     = mGIFStruct.rowp;

  uint32_t bpr = mGIFStruct.width;
  if (!mGIFStruct.images_decoded) {
    bpr *= sizeof(uint32_t);
  }
  uint8_t* rowend   = mImageData + (bpr * mGIFStruct.irow) + mGIFStruct.width;

#define OUTPUT_ROW()                                        \
  PR_BEGIN_MACRO                                            \
    if (!OutputRow())                                       \
      goto END;                                             \
    rowp = mImageData + mGIFStruct.irow * bpr;              \
    rowend = rowp + mGIFStruct.width;                       \
  PR_END_MACRO

  for (const uint8_t* ch = q; count-- > 0; ch++) {
    
    datum += ((int32_t)* ch) << bits;
    bits += 8;

    
    while (bits >= codesize) {
      
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
        if (code >= MAX_BITS) {
          return false;
        }
        *rowp++ = suffix[code] & mColorMask; 
        if (rowp == rowend) {
          OUTPUT_ROW();
        }

        firstchar = oldcode = code;
        continue;
      }

      int incode = code;
      if (code >= avail) {
        *stackp++ = firstchar;
        code = oldcode;

        if (stackp >= stack + MAX_BITS) {
          return false;
        }
      }

      while (code >= clear_code) {
        if ((code >= MAX_BITS) || (code == prefix[code])) {
          return false;
        }

        *stackp++ = suffix[code];
        code = prefix[code];

        if (stackp == stack + MAX_BITS) {
          return false;
        }
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
        *rowp++ = *--stackp & mColorMask; 
        if (rowp == rowend) {
          OUTPUT_ROW();
        }
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

  return true;
}



static void
ConvertColormap(uint32_t* aColormap, uint32_t aColors)
{
  
  if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
    qcms_transform* transform = gfxPlatform::GetCMSRGBTransform();
    if (transform) {
      qcms_transform_data(transform, aColormap, aColormap, aColors);
    }
  }
  
  
  uint8_t* from = ((uint8_t*)aColormap) + 3 * aColors;
  uint32_t* to = aColormap + aColors;

  

  
  if (!aColors) {
    return;
  }
  uint32_t c = aColors;

  
  
  for (; (NS_PTR_TO_UINT32(from) & 0x3) && c; --c) {
    from -= 3;
    *--to = gfxPackedPixel(0xFF, from[0], from[1], from[2]);
  }

  
  while (c >= 4) {
    from -= 12;
    to   -=  4;
    c    -=  4;
    GFX_BLOCK_RGB_TO_FRGB(from,to);
  }

  
  
  while (c--) {
    from -= 3;
    *--to = gfxPackedPixel(0xFF, from[0], from[1], from[2]);
  }
}

void
nsGIFDecoder2::WriteInternal(const char* aBuffer, uint32_t aCount)
{
  MOZ_ASSERT(!HasError(), "Shouldn't call WriteInternal after error!");

  
  const uint8_t* buf = (const uint8_t*)aBuffer;
  uint32_t len = aCount;

  const uint8_t* q = buf;

  
  
  
  uint8_t* p =
    (mGIFStruct.state ==
      gif_global_colormap) ? (uint8_t*) mGIFStruct.global_colormap :
        (mGIFStruct.state == gif_image_colormap) ? (uint8_t*) mColormap :
          (mGIFStruct.bytes_in_hold) ? mGIFStruct.hold : nullptr;

  if (len == 0 && buf == nullptr) {
    
    
    len = mGIFStruct.bytes_in_hold;
    q = buf = p;
  } else if (p) {
    
    uint32_t l = std::min(len, mGIFStruct.bytes_to_consume);
    memcpy(p+mGIFStruct.bytes_in_hold, buf, l);

    if (l < mGIFStruct.bytes_to_consume) {
      
      mGIFStruct.bytes_in_hold += l;
      mGIFStruct.bytes_to_consume -= l;
      return;
    }
    
    q = p;
  }

  
  
  
  
  
  
  
  

  for (;len >= mGIFStruct.bytes_to_consume; q=buf, mGIFStruct.bytes_in_hold = 0)
  {
    
    buf += mGIFStruct.bytes_to_consume;
    len -= mGIFStruct.bytes_to_consume;

    switch (mGIFStruct.state) {
    case gif_lzw:
      if (!DoLzw(q)) {
        mGIFStruct.state = gif_error;
        break;
      }
      GETN(1, gif_sub_block);
      break;

    case gif_lzw_start: {
      
      if (mGIFStruct.is_transparent) {
        
        if (mColormap == mGIFStruct.global_colormap) {
            mOldColor = mColormap[mGIFStruct.tpixel];
        }
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

      
      for (int i = 0; i < clear_code; i++) {
        mGIFStruct.suffix[i] = i;
      }

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

      if (IsSizeDecode()) {
        MOZ_ASSERT(!mGIFOpen, "Gif should not be open at this point");
        PostSize(mGIFStruct.screen_width, mGIFStruct.screen_height);
        return;
      }

      
      
      
      
      

      if (q[4] & 0x80) {
        
        const uint32_t size = (3 << mGIFStruct.global_colormap_depth);
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
      
      
      ConvertColormap(mGIFStruct.global_colormap,
                      1<<mGIFStruct.global_colormap_depth);
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
          mGIFStruct.bytes_to_consume =
            std::max(mGIFStruct.bytes_to_consume, 4u);
          break;

        
        
        
        
        
        
        
        
        case GIF_APPLICATION_EXTENSION_LABEL:
          mGIFStruct.state = gif_application_extension;
          break;

        case GIF_PLAIN_TEXT_LABEL:
          mGIFStruct.state = gif_skip_block;
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
      if (!*q) {
        GETN(1, gif_image_start);
      } else {
        GETN(*q, gif_skip_block);
      }
      break;

    case gif_skip_block:
      GETN(1, gif_consume_block);
      break;

    case gif_control_extension:
      mGIFStruct.is_transparent = *q & 0x1;
      mGIFStruct.tpixel = q[3];
      mGIFStruct.disposal_method = ((*q) >> 2) & 0x7;
      
      
      if (mGIFStruct.disposal_method == 4) {
        mGIFStruct.disposal_method = 3;
      }

      {
        DisposalMethod method = DisposalMethod(mGIFStruct.disposal_method);
        if (method == DisposalMethod::CLEAR_ALL ||
            method == DisposalMethod::CLEAR) {
          
          
          PostHasTransparency();
        }
      }

      mGIFStruct.delay_time = GETINT16(q + 1) * 10;
      GETN(1, gif_consume_block);
      break;

    case gif_comment_extension:
      if (*q) {
        GETN(*q, gif_consume_comment);
      } else {
        GETN(1, gif_image_start);
      }
      break;

    case gif_consume_comment:
      GETN(1, gif_comment_extension);
      break;

    case gif_application_extension:
      
      if (mGIFStruct.bytes_to_consume == 11 &&
          (!strncmp((char*)q, "NETSCAPE2.0", 11) ||
           !strncmp((char*)q, "ANIMEXTS1.0", 11))) {
        GETN(1, gif_netscape_extension_block);
      } else {
        GETN(1, gif_consume_block);
      }
      break;

    
    case gif_netscape_extension_block:
      if (*q) {
        
        
        GETN(std::max(3, static_cast<int>(*q)), gif_consume_netscape_extension);
      } else {
        GETN(1, gif_image_start);
      }
      break;

    
    case gif_consume_netscape_extension:
      switch (q[0] & 7) {
        case 1:
          
          
          mGIFStruct.loop_count = GETINT16(q + 1);
          GETN(1, gif_netscape_extension_block);
          break;

        case 2:
          

          
          
          
          
          GETN(1, gif_netscape_extension_block);
          break;

        default:
          
          mGIFStruct.state = gif_error;
      }
      break;

    case gif_image_header: {
      
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
        if (HasError()) {
          
          mGIFStruct.state = gif_error;
          return;
        }

        
        if (IsSizeDecode()) {
          return;
        }
      }

      
      if (!mGIFStruct.height || !mGIFStruct.width) {
        mGIFStruct.height = mGIFStruct.screen_height;
        mGIFStruct.width = mGIFStruct.screen_width;
        if (!mGIFStruct.height || !mGIFStruct.width) {
          mGIFStruct.state = gif_error;
          break;
        }
      }

      
      
      
      uint32_t depth = mGIFStruct.global_colormap_depth;
      if (q[8] & 0x80) {
        depth = (q[8]&0x07) + 1;
      }
      uint32_t realDepth = depth;
      while (mGIFStruct.tpixel >= (1 << realDepth) && (realDepth < 8)) {
        realDepth++;
      }
      
      mColorMask = 0xFF >> (8 - realDepth);
      BeginImageFrame(realDepth);

      if (NeedsNewFrame()) {
        
        
        
        uint32_t size =
          len + mGIFStruct.bytes_to_consume + mGIFStruct.bytes_in_hold;
        if (size) {
          if (SetHold(q,
                      mGIFStruct.bytes_to_consume + mGIFStruct.bytes_in_hold,
                      buf, len)) {
            
            GETN(9, gif_image_header_continue);
            return;
          }
        }
        break;
      } else {
        
      }
    }

    case gif_image_header_continue: {
      
      
      
      memset(mImageData, 0, mImageDataLength);
      if (mColormap) {
        memset(mColormap, 0, mColormapSize);
      }

      if (!mGIFStruct.images_decoded) {
        
        
        
        if (mGIFStruct.y_offset > 0) {
          nsIntRect r(0, 0, mGIFStruct.screen_width, mGIFStruct.y_offset);
          PostInvalidation(r);
        }
      }

      if (q[8] & 0x40) {
        mGIFStruct.interlaced = true;
        mGIFStruct.ipass = 1;
      } else {
        mGIFStruct.interlaced = false;
        mGIFStruct.ipass = 0;
      }

      
      mGIFStruct.progressive_display = (mGIFStruct.images_decoded == 0);

      
      mGIFStruct.irow = 0;
      mGIFStruct.rows_remaining = mGIFStruct.height;
      mGIFStruct.rowp = mImageData;

      
      
      
      uint32_t depth = mGIFStruct.global_colormap_depth;
      if (q[8] & 0x80) {
        depth = (q[8]&0x07) + 1;
      }
      uint32_t realDepth = depth;
      while (mGIFStruct.tpixel >= (1 << realDepth) && (realDepth < 8)) {
        realDepth++;
      }
      
      if (q[8] & 0x80) {
        mGIFStruct.local_colormap_size = 1 << depth;
        if (!mGIFStruct.images_decoded) {
          
          
          mColormapSize = sizeof(uint32_t) << realDepth;
          if (!mGIFStruct.local_colormap) {
            mGIFStruct.local_colormap = (uint32_t*)moz_xmalloc(mColormapSize);
          }
          mColormap = mGIFStruct.local_colormap;
        }
        const uint32_t size = 3 << depth;
        if (mColormapSize > size) {
          
          memset(((uint8_t*)mColormap) + size, 0, mColormapSize - size);
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
      MOZ_ASSERT(!IsSizeDecode(), "Size decodes shouldn't reach gif_done");
      FinishInternal();
      goto done;

    case gif_error:
      PostDataError();
      return;

    
    default:
      break;
    }
  }

  
  if (mGIFStruct.state == gif_error) {
      PostDataError();
      return;
  }

  
  if (len) {
    
    if (mGIFStruct.state != gif_global_colormap &&
        mGIFStruct.state != gif_image_colormap) {
      if (!SetHold(buf, len)) {
        PostDataError();
        return;
      }
    } else {
      uint8_t* p = (mGIFStruct.state == gif_global_colormap) ?
                    (uint8_t*)mGIFStruct.global_colormap :
                    (uint8_t*)mColormap;
      memcpy(p, buf, len);
      mGIFStruct.bytes_in_hold = len;
    }

    mGIFStruct.bytes_to_consume -= len;
  }


done:
  if (!mGIFStruct.images_decoded) {
    FlushImageData();
    mLastFlushedRow = mCurrentRow;
    mLastFlushedPass = mCurrentPass;
  }

  return;
}

bool
nsGIFDecoder2::SetHold(const uint8_t* buf1, uint32_t count1,
                       const uint8_t* buf2 ,
                       uint32_t count2 )
{
  
  uint8_t* newHold = (uint8_t*) malloc(std::max(uint32_t(MIN_HOLD_SIZE),
                                       count1 + count2));
  if (!newHold) {
    mGIFStruct.state = gif_error;
    return false;
  }

  memcpy(newHold, buf1, count1);
  if (buf2) {
    memcpy(newHold + count1, buf2, count2);
  }

  free(mGIFStruct.hold);
  mGIFStruct.hold = newHold;
  mGIFStruct.bytes_in_hold = count1 + count2;
  return true;
}

Telemetry::ID
nsGIFDecoder2::SpeedHistogram()
{
  return Telemetry::IMAGE_DECODE_SPEED_GIF;
}


} 
} 
