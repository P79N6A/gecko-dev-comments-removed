






































#include "prmem.h"

#include "nsGIFDecoder2.h"
#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsRecyclingAllocator.h"

#include "imgIContainerObserver.h"

#include "imgILoad.h"

#include "imgContainer.h"









const int kGifAllocatorNBucket = 3;
static nsRecyclingAllocator *gGifAllocator = nsnull;

void nsGifShutdown()
{
  
  delete gGifAllocator;
  gGifAllocator = nsnull;
}






NS_IMPL_ISUPPORTS1(nsGIFDecoder2, imgIDecoder)

nsGIFDecoder2::nsGIFDecoder2()
  : mCurrentRow(-1)
  , mLastFlushedRow(-1)
  , mGIFStruct(nsnull)
  , mRGBLine(nsnull)
  , mRGBLineMaxSize(0)
  , mBackgroundRGBIndex(0)
  , mCurrentPass(0)
  , mLastFlushedPass(0)
  , mGIFOpen(PR_FALSE)
{
}

nsGIFDecoder2::~nsGIFDecoder2(void)
{
  Close();
}







NS_IMETHODIMP nsGIFDecoder2::Init(imgILoad *aLoad)
{
  mObserver = do_QueryInterface(aLoad);

  mImageContainer = do_CreateInstance("@mozilla.org/image/container;1");
  aLoad->SetImage(mImageContainer);
  
  if (!gGifAllocator) {
    gGifAllocator = new nsRecyclingAllocator(kGifAllocatorNBucket,
                                             NS_DEFAULT_RECYCLE_TIMEOUT, "gif");
    if (!gGifAllocator)
      return NS_ERROR_FAILURE;
  }
  mGIFStruct = (gif_struct *)gGifAllocator->Malloc(sizeof(gif_struct));
  NS_ASSERTION(mGIFStruct, "gif_create failed");
  if (!mGIFStruct)
    return NS_ERROR_FAILURE;

  
  GIFInit(mGIFStruct, this);

  return NS_OK;
}








NS_IMETHODIMP nsGIFDecoder2::Close()
{
  if (mGIFStruct) {
    nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, mGIFStruct->clientptr);
    if (decoder->mImageFrame)
      EndImageFrame(mGIFStruct->clientptr, mGIFStruct->images_decoded + 1, 
                    mGIFStruct->delay_time);
    if (decoder->mGIFOpen)
      EndGIF(mGIFStruct->clientptr, mGIFStruct->loop_count);

    gif_destroy(mGIFStruct);
    if (gGifAllocator)
      gGifAllocator->Free(mGIFStruct);
    mGIFStruct = nsnull;
  }
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




NS_METHOD
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

  return NS_OK;
}


nsresult nsGIFDecoder2::ProcessData(unsigned char *data, PRUint32 count, PRUint32 *_retval)
{
  
  
  PRStatus result = gif_write(mGIFStruct, data, count);
  if (result != PR_SUCCESS)
    return NS_ERROR_FAILURE;

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

  


  if (NS_SUCCEEDED(rv) && mGIFStruct && mGIFStruct->state == gif_error) {
    PRUint32 numFrames = 0;
    if (mImageContainer)
      mImageContainer->GetNumFrames(&numFrames);
    if (numFrames <= 0)
      return NS_ERROR_FAILURE;
  }

  return rv;
}







int nsGIFDecoder2::BeginGIF(
  void*    aClientData,
  PRUint32 aLogicalScreenWidth, 
  PRUint32 aLogicalScreenHeight,
  PRUint8  aBackgroundRGBIndex)
{
  
  
  if(aLogicalScreenWidth == 0 || aLogicalScreenHeight == 0)
    return 0;
    
  
  nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);

  decoder->mBackgroundRGBIndex = aBackgroundRGBIndex;

  if (decoder->mObserver)
    decoder->mObserver->OnStartDecode(nsnull);

  decoder->mImageContainer->Init(aLogicalScreenWidth, aLogicalScreenHeight, decoder->mObserver);

  if (decoder->mObserver)
    decoder->mObserver->OnStartContainer(nsnull, decoder->mImageContainer);

  decoder->mGIFOpen = PR_TRUE;
  return 0;
}


int nsGIFDecoder2::EndGIF(
    void*    aClientData,
    int      aAnimationLoopCount)
{
  nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);

  if (!decoder->mGIFOpen)
    return 0;

  if (decoder->mObserver) {
    decoder->mObserver->OnStopContainer(nsnull, decoder->mImageContainer);
    decoder->mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }
  
  decoder->mImageContainer->SetLoopCount(aAnimationLoopCount);
  decoder->mImageContainer->DecodingComplete();

  decoder->mGIFOpen = PR_FALSE;
  return 0;
}


int nsGIFDecoder2::BeginImageFrame(
  void*    aClientData,
  PRUint32 aFrameNumber,   
  PRUint32 aFrameXOffset,  
  PRUint32 aFrameYOffset,  
  PRUint32 aFrameWidth,    
  PRUint32 aFrameHeight)
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  
  decoder->mImageFrame = nsnull; 
  decoder->mGIFStruct->x_offset = aFrameXOffset;
  decoder->mGIFStruct->y_offset = aFrameYOffset;
  decoder->mGIFStruct->width = aFrameWidth;
  decoder->mGIFStruct->height = aFrameHeight;

  if (aFrameNumber == 1) {
    
    
    
    PRInt32 imgWidth;
    decoder->mImageContainer->GetWidth(&imgWidth);
    if (aFrameYOffset > 0) {
      nsIntRect r(0, 0, imgWidth, aFrameYOffset);
      decoder->mObserver->OnDataAvailable(nsnull, decoder->mImageFrame, &r);
    }
  }

  return 0;
}


int nsGIFDecoder2::EndImageFrame(
  void*    aClientData, 
  PRUint32 aFrameNumber,
  PRUint32 aDelayTimeout)  



{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  
  
  
  
  if (!decoder->mImageFrame) {
    HaveDecodedRow(aClientData,nsnull,0,0,0);
  } else {
    
    
    
    
    decoder->mImageFrame->SetTimeout(aDelayTimeout);
  }
  decoder->mImageContainer->EndFrameDecode(aFrameNumber, aDelayTimeout);

  if (decoder->mObserver && decoder->mImageFrame) {
    decoder->FlushImageData();

    if (aFrameNumber == 1) {
      
      
      
      PRInt32 imgHeight;
      PRInt32 realFrameHeight = decoder->mGIFStruct->height + decoder->mGIFStruct->y_offset;

      decoder->mImageContainer->GetHeight(&imgHeight);
      if (imgHeight > realFrameHeight) {
        PRInt32 imgWidth;
        decoder->mImageContainer->GetWidth(&imgWidth);

        nsIntRect r(0, realFrameHeight, imgWidth, imgHeight - realFrameHeight);
        decoder->mObserver->OnDataAvailable(nsnull, decoder->mImageFrame, &r);
      }
    }

    decoder->mCurrentRow = decoder->mLastFlushedRow = -1;
    decoder->mCurrentPass = decoder->mLastFlushedPass = 0;

    decoder->mObserver->OnStopFrame(nsnull, decoder->mImageFrame);
  }

  decoder->mImageFrame = nsnull;
  decoder->mGIFStruct->is_transparent = PR_FALSE;
  return 0;
}
  


int nsGIFDecoder2::HaveDecodedRow(
  void* aClientData,
  PRUint8* aRowBufPtr,   
  int aRowNumber,        
  int aDuplicateCount,   
  int aInterlacePass)    
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  PRUint32 bpr;
  
  
  
  
  
  if(! decoder->mImageFrame) {
    gfx_format format = gfxIFormats::RGB;
    if (decoder->mGIFStruct->is_transparent) {
      format = gfxIFormats::RGB_A1;  
    }

    
    decoder->mImageFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
    if (!decoder->mImageFrame || NS_FAILED(decoder->mImageFrame->Init(
          decoder->mGIFStruct->x_offset, decoder->mGIFStruct->y_offset, 
          decoder->mGIFStruct->width, decoder->mGIFStruct->height, format, 24))) {
      decoder->mImageFrame = 0;
      return 0;
    }

    decoder->mImageFrame->SetFrameDisposalMethod(decoder->mGIFStruct->disposal_method);
    decoder->mImageContainer->AppendFrame(decoder->mImageFrame);

    if (decoder->mObserver)
      decoder->mObserver->OnStartFrame(nsnull, decoder->mImageFrame);

    decoder->mImageFrame->GetImageBytesPerRow(&bpr);

    if (bpr > decoder->mRGBLineMaxSize) {
      decoder->mRGBLine = (PRUint8 *)PR_REALLOC(decoder->mRGBLine, bpr);
      decoder->mRGBLineMaxSize = bpr;
    }
  } else {
    decoder->mImageFrame->GetImageBytesPerRow(&bpr);
  }
  
  if (aRowBufPtr) {
    PRInt32 width;
    decoder->mImageFrame->GetWidth(&width);

    gfx_format format;
    decoder->mImageFrame->GetFormat(&format);

    
    int cmapsize;
    PRUint8* cmap;
    cmapsize = decoder->mGIFStruct->global_colormap_size;
    cmap = decoder->mGIFStruct->global_colormap;

    if(decoder->mGIFStruct->global_colormap &&
       decoder->mGIFStruct->screen_bgcolor < cmapsize) {
      gfx_color bgColor = 0;
      PRUint32 bgIndex = decoder->mGIFStruct->screen_bgcolor * 3;
      bgColor |= cmap[bgIndex];
      bgColor |= cmap[bgIndex + 1] << 8;
      bgColor |= cmap[bgIndex + 2] << 16;
      decoder->mImageFrame->SetBackgroundColor(bgColor);
    }
    if (decoder->mGIFStruct->is_local_colormap_defined) {
      cmapsize = decoder->mGIFStruct->local_colormap_size;
      cmap = decoder->mGIFStruct->local_colormap;
    }

    if (!cmap) { 
      for (int i = 0; i < aDuplicateCount; ++i) {
        imgContainer::BlackenFrame(decoder->mImageFrame, 0, aRowNumber+i, width, 1);
      }
    } else {
      PRUint8* rowBufIndex = aRowBufPtr;
      PRUint32 *rgbRowIndex = (PRUint32*)decoder->mRGBLine;

      PRInt32 tpixel =
        decoder->mGIFStruct->is_transparent ? decoder->mGIFStruct->tpixel : -1;

      while (rowBufIndex != decoder->mGIFStruct->rowend) {
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
        decoder->mImageFrame->SetImageData(decoder->mRGBLine, bpr, (aRowNumber+i)*bpr);
    }

    decoder->mCurrentRow = aRowNumber + aDuplicateCount - 1;
    decoder->mCurrentPass = aInterlacePass;
    if (aInterlacePass == 1)
      decoder->mLastFlushedPass = aInterlacePass;   
  }

  return 0;
}
