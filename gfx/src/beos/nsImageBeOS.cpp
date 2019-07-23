





































#include "nsImageBeOS.h"
#include "nsRenderingContextBeOS.h"
#include "nspr.h"
#include <Looper.h>
#include <Bitmap.h>
#include <View.h>

NS_IMPL_ISUPPORTS1(nsImageBeOS, nsIImage)

nsImageBeOS::nsImageBeOS()
  : mImage(nsnull)
  , mImageBits(nsnull)
  , mWidth(0)
  , mHeight(0)
  , mDepth(0)
  , mRowBytes(0)
  , mSizeImage(0)
  , mDecodedX1(PR_INT32_MAX)
  , mDecodedY1(PR_INT32_MAX)
  , mDecodedX2(0)
  , mDecodedY2(0)
  , mAlphaBits(nsnull)
  , mAlphaRowBytes(0)
  , mAlphaDepth(0)
  , mFlags(0)
  , mNumBytesPixel(0)
  , mImageCurrent(PR_FALSE)
  , mOptimized(PR_FALSE)
  , mTileBitmap(nsnull)
{
}

nsImageBeOS::~nsImageBeOS() 
{
	if (nsnull != mImage) 
	{
		delete mImage;
		mImage = nsnull;
	}
			
	if (mTileBitmap) 
	{
		delete mTileBitmap;
		mTileBitmap = nsnull;
	}

	if (nsnull != mImageBits) 
	{
		delete [] mImageBits;
		mImageBits = nsnull;
	}
	if (nsnull != mAlphaBits) 
	{
		delete [] mAlphaBits;
		mAlphaBits = nsnull;
	}
}

nsresult nsImageBeOS::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
							nsMaskRequirements aMaskRequirements) 
{
	
	
	if (24 == aDepth) 
	{
		mNumBytesPixel = 3;
	}
	else 
	{
		NS_ASSERTION(PR_FALSE, "unexpected image depth");
		return NS_ERROR_UNEXPECTED;
	}
	
	mWidth = aWidth;
	mHeight = aHeight;
	mDepth = aDepth;
	mRowBytes = (mWidth * mDepth) >> 5;
	if (((PRUint32)mWidth * mDepth) & 0x1F) 
		mRowBytes++;
	mRowBytes <<= 2;
	mSizeImage = mRowBytes * mHeight;

	mImageBits = new PRUint8[mSizeImage];

	switch (aMaskRequirements) 
	{
		case nsMaskRequirements_kNeeds1Bit:
			mAlphaRowBytes = (aWidth + 7) / 8;
			mAlphaDepth = 1;
			
			mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;
			mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
			memset(mAlphaBits, 255, mAlphaRowBytes * aHeight);
			break;
		case nsMaskRequirements_kNeeds8Bit:
			mAlphaRowBytes = aWidth;
			mAlphaDepth = 8;
			
			mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;
			mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
			break;
	}
	
	return NS_OK;
}






void nsImageBeOS::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect) 
{
	
	mFlags = aFlags;
	mImageCurrent = PR_FALSE;

	mDecodedX1 = PR_MIN(mDecodedX1, aUpdateRect->x);
	mDecodedY1 = PR_MIN(mDecodedY1, aUpdateRect->y);

	if (aUpdateRect->YMost() > mDecodedY2)
		mDecodedY2 = aUpdateRect->YMost();
	if (aUpdateRect->XMost() > mDecodedX2)
		mDecodedX2 = aUpdateRect->XMost();
} 




PRBool nsImageBeOS::GetIsImageComplete() {
  return mDecodedX1 == 0 &&
         mDecodedY1 == 0 &&
         mDecodedX2 == mWidth &&
         mDecodedY2 == mHeight;
}


NS_IMETHODIMP nsImageBeOS::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
	PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
	PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight) 
{
	
	if (!aSWidth || !aSHeight || !aDWidth || !aDHeight)
		return NS_OK;
	if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
		return NS_OK;

	
	float srcX = aSX, srcY = aSY, srcMostX = aSX + aSWidth, srcMostY = aSY + aSHeight;
	float dstX = aDX, dstY = aDY, dstMostX = aDX + aDWidth, dstMostY = aDY + aDHeight;
	float  scaleX = float(aDWidth)/float(aSWidth), scaleY = float(aDHeight)/float(aSHeight);

	if (!mImageCurrent || (nsnull == mImage)) 
		BuildImage(aSurface);
	if (nsnull == mImage || mImage->BitsLength() == 0) 
		return NS_ERROR_FAILURE;

	
	
	if ((mDecodedY1 > 0)) 
	{
		srcY = float(PR_MAX(mDecodedY1, aSY));
	}
	if ((mDecodedX1 > 0))
	{
		srcX = float(PR_MAX(mDecodedX1, aSX));
	}
	
	if ((mDecodedY2 < mHeight)) 
		srcMostY = float(PR_MIN(mDecodedY2, aSY + aSHeight));

	if ((mDecodedX2 < mWidth)) 
		srcMostX = float(PR_MIN(mDecodedX2, aSX + aSWidth));

	dstX = float(srcX - aSX)*scaleX + float(aDX);
	dstY = float(srcY - aSY)*scaleY + float(aDY);
	dstMostX = dstMostX - (float(aSWidth + aSX) - srcMostX)*scaleX;
	dstMostY =	dstMostY - (float(aSHeight + aSY) - srcMostY)*scaleY;

	nsDrawingSurfaceBeOS *beosdrawing = (nsDrawingSurfaceBeOS *)aSurface;
	BView *view;

	
	if (((nsRenderingContextBeOS&)aContext).LockAndUpdateView()) 
	{
		beosdrawing->AcquireView(&view);
		if (view) 
		{
			
			
			

			
			if (0 != mAlphaDepth) 
			{
				view->SetDrawingMode(B_OP_ALPHA);
				view->DrawBitmap(mImage, BRect(srcX, srcY, srcMostX - 1, srcMostY - 1),
					BRect(dstX, dstY, dstMostX - 1, dstMostY - 1));
				view->SetDrawingMode(B_OP_COPY);
			}
			else 
			{
				view->DrawBitmap(mImage, BRect(srcX, srcY, srcMostX - 1, srcMostY - 1),
					BRect(dstX, dstY, dstMostX - 1, dstMostY - 1));
			}
			
		}
		((nsRenderingContextBeOS&)aContext).UnlockView();
		beosdrawing->ReleaseView();
	}
	
	mFlags = 0;
	return NS_OK;
}


NS_IMETHODIMP nsImageBeOS::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
	PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) 
{
	
	if (!aWidth || !aHeight)
		return NS_OK;
	if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
		return NS_OK;

	if (!mImageCurrent || (nsnull == mImage)) 
		BuildImage(aSurface);
	if (nsnull == mImage) 
		return NS_ERROR_FAILURE;

	PRInt32 validX = 0, validY = 0, validMostX = mWidth, validMostY = mHeight;

	
	
	aWidth = PR_MIN(aWidth, mWidth);
	aHeight = PR_MIN(aHeight, mHeight);

	if ((mDecodedY2 < aHeight)) 
		validMostY = mDecodedY2;

	if ((mDecodedX2 < aWidth)) 
		validMostX = mDecodedX2;

	if ((mDecodedY1 > 0)) 
		validY = mDecodedY1;
	if ((mDecodedX1 > 0)) 
		validX = mDecodedX1;
			
	nsDrawingSurfaceBeOS *beosdrawing = (nsDrawingSurfaceBeOS *)aSurface;
	BView *view;

	if (((nsRenderingContextBeOS&)aContext).LockAndUpdateView()) 
	{
		beosdrawing->AcquireView(&view);
		if (view) 
		{
			
			
			
			
			
			if (0 != mAlphaDepth) 
			{
				view->SetDrawingMode(B_OP_ALPHA);
				view->DrawBitmap(mImage, BRect(validX, validY, validMostX - 1, validMostY - 1), 
								BRect(aX + validX, aY + validY, aX + validMostX - 1, aY + validMostY - 1));
				view->SetDrawingMode(B_OP_COPY);
			} 
			else 
			{
				view->DrawBitmap(mImage, BRect(validX, validY, validMostX - 1, validMostY - 1), 
								BRect(aX + validX, aY + validY, aX + validMostX - 1, aY + validMostY - 1));
			}
		}
		((nsRenderingContextBeOS&)aContext).UnlockView();
		beosdrawing->ReleaseView();
	}

	mFlags = 0;
	return NS_OK;
}

NS_IMETHODIMP nsImageBeOS::DrawTile(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
	PRInt32 aSXOffset, PRInt32 aSYOffset, PRInt32 aPadX, PRInt32 aPadY, const nsRect &aTileRect) 
{
	
	if (!aTileRect.width || !aTileRect.height)
		return NS_OK;
	if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
		return NS_OK;

	if (!mImageCurrent || (nsnull == mImage)) 
		BuildImage(aSurface);
	if (nsnull == mImage || mImage->BitsLength() == 0) 
		return NS_ERROR_FAILURE;

	PRInt32 validX = 0, validY = 0, validMostX = mWidth, validMostY = mHeight;

	
	
	if ((mDecodedY2 < mHeight)) 
		validMostY = mDecodedY2;

	if ((mDecodedX2 < mWidth)) 
		validMostX = mDecodedX2;

	if ((mDecodedY1 > 0)) 
		validY = mDecodedY1;

	if ((mDecodedX1 > 0)) 
		validX = mDecodedX1;

	nsDrawingSurfaceBeOS *beosdrawing = (nsDrawingSurfaceBeOS *)aSurface;
	BView *view = 0;
	if (((nsRenderingContextBeOS&)aContext).LockAndUpdateView()) 
	{
		beosdrawing->AcquireView(&view);
		if (view) 
		{
	        BRegion rgn(BRect(aTileRect.x, aTileRect.y,
	        			aTileRect.x + aTileRect.width - 1, aTileRect.y + aTileRect.height - 1));
    	    view->ConstrainClippingRegion(&rgn);

			
			if (0 != mAlphaDepth || aPadX || aPadY) 
				view->SetDrawingMode(B_OP_ALPHA);
			
			
			if (!mTileBitmap || mTileBitmap->Bounds().IntegerWidth() + 1 != aTileRect.width || mTileBitmap->Bounds().IntegerHeight() + 1 != aTileRect.height)
			{
				if (mTileBitmap)
				{
					delete mTileBitmap;
					mTileBitmap = nsnull;
				}
				mTileBitmap = new BBitmap(BRect(0, 0, aTileRect.width - 1, aTileRect.height -1), mImage->ColorSpace(), false);
			}
			
			int32 tmpbitlength = mTileBitmap->BitsLength();

			if (!mTileBitmap || tmpbitlength == 0)
			{
				
				((nsRenderingContextBeOS&)aContext).UnlockView();
				if (mTileBitmap)
				{
					delete mTileBitmap;
					mTileBitmap = nsnull;
				}
				beosdrawing->ReleaseView();
				return NS_ERROR_FAILURE;
			}

			uint32 *dst0 = (uint32 *)mTileBitmap->Bits();
			uint32 *src0 = (uint32 *)mImage->Bits();
			uint32 *dst = dst0;
			uint32 dstRowLength = mTileBitmap->BytesPerRow()/4;
			uint32 dstColHeight = tmpbitlength/mTileBitmap->BytesPerRow();

			
			uint32 filllength = tmpbitlength/4;
			if (0 != mAlphaDepth  || aPadX || aPadY) 
			{
				for (uint32 i=0, *dst = dst0; i < filllength; ++i)
					*(dst++) = B_TRANSPARENT_MAGIC_RGBA32;
			}

			
			uint32 *src = src0; dst = dst0;
			for (uint32 y = 0, yy = aSYOffset; y < dstColHeight; ++y) 
			{					
				src = src0 + yy*mWidth;
				dst = dst0 + y*dstRowLength;
				
				if (yy >= validY && yy <= validMostY)
				{
					for (uint32 x = 0, xx = aSXOffset; x < dstRowLength; ++x) 
					{
						
						if (xx >= validX && xx <= validMostX)
							dst[x] = src[xx];
						if (++xx == mWidth)
						{
							
							xx = 0;
							x += aPadX;
						}
					}
				}
				if (++yy == mHeight)
				{
					
					yy = 0;
					y += aPadY;
				}
			}
			
			view->DrawBitmap(mTileBitmap, BPoint(aTileRect.x , aTileRect.y ));
			view->SetDrawingMode(B_OP_COPY);
			view->Sync();
		}
		((nsRenderingContextBeOS&)aContext).UnlockView();
		beosdrawing->ReleaseView();
	}
	mFlags = 0;
	return NS_OK;
}




nsresult nsImageBeOS::Optimize(nsIDeviceContext *aContext) 
{
	if (!mOptimized) 
	{
		
		CreateImage(NULL);
		
		
		if (nsnull != mImageBits) 
		{
			delete [] mImageBits;
			mImageBits = nsnull;
		}
		if (nsnull != mAlphaBits) 
		{
			delete [] mAlphaBits;
			mAlphaBits = nsnull;
		}
		
		mOptimized = PR_TRUE;
	}
	return NS_OK;
}



NS_IMETHODIMP nsImageBeOS::LockImagePixels(PRBool aMaskPixels) 
{
	
	return NS_OK;
}


NS_IMETHODIMP nsImageBeOS::UnlockImagePixels(PRBool aMaskPixels) 
{
	return NS_OK;
}

nsresult nsImageBeOS::BuildImage(nsIDrawingSurface* aDrawingSurface) 
{
	CreateImage(aDrawingSurface);
	return NS_OK;
}





void nsImageBeOS::CreateImage(nsIDrawingSurface* aSurface) 
{
	PRInt32 validX = 0, validY = 0, validMostX = mWidth, validMostY = mHeight;

	if (mImageBits) 
	{
		if (24 != mDepth) 
		{
			NS_ASSERTION(PR_FALSE, "unexpected image depth");
			return;
		}
		
		
		const color_space cs = B_RGBA32;
		if (nsnull != mImage) 
		{
			BRect bounds = mImage->Bounds();
			if (bounds.IntegerWidth() < validMostX - 1 || bounds.IntegerHeight() < validMostY - 1 ||
				mImage->ColorSpace() != cs) 
			{
				
				delete mImage;
				mImage = new BBitmap(BRect(0, 0, validMostX - 1, validMostY - 1), cs, false);
			} 
			else 
			{
				
				if (mImageCurrent) return;
			}
		} 
		else 
		{
			
			mImage = new BBitmap(BRect(0, 0, mWidth - 1, mHeight - 1), cs, false);
		}
		
		
		
		
		if ((mDecodedY2 < mHeight)) 
			validMostY = mDecodedY2;

		if ((mDecodedX2 < mWidth)) 
			validMostX = mDecodedX2;

		if ((mDecodedY1 > 0)) 
			validY = mDecodedY1;
		if ((mDecodedX1 > 0)) 
			validX = mDecodedX1;
		
		if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
			return;

		
		
		
		
		
		
		if (mImage && mImage->IsValid()) 
		{
			uint32 *dest, *dst0 = (uint32 *)mImage->Bits() + validX;
			uint8 *src, *src0 = mImageBits + 3*validX; 
			if (mAlphaBits) 
			{
				uint8 a, *alpha = mAlphaBits + validY*mAlphaRowBytes;;
				for (int y = validY; y < validMostY; ++y) 
				{
					dest = dst0 + y*mWidth;
					src = src0 + y*mRowBytes;
					for (int x = validX; x < validMostX; ++x) 
					{
						if(1 == mAlphaDepth)
							a = (alpha[x / 8] & (1 << (7 - (x % 8)))) ? 255 : 0;
						else
							a = alpha[x];
						*dest++ = (a << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
						src += 3;
					}
					alpha += mAlphaRowBytes;
				}
			} 
			else 
			{
				
				for (int y = validY; y < validMostY; ++y) 
				{
					dest = dst0 + y*mWidth;
					src = src0 + y*mRowBytes;
					for (int x = validX; x < validMostX; ++x) 
					{
						*dest++ = 0xff000000 | (src[2] << 16) | (src[1] << 8) | src[0];
						src += 3;
					}
				}
			}
			
			
			
			mImageCurrent = PR_TRUE;
		}
	}
}





NS_IMETHODIMP nsImageBeOS::DrawToImage(nsIImage* aDstImage, 
										nscoord aDX, nscoord aDY, 
										nscoord aDWidth, nscoord aDHeight)
{
	nsImageBeOS *dest = static_cast<nsImageBeOS *>(aDstImage);

	if (!dest)
		return NS_ERROR_FAILURE;

	if (aDX >= dest->mWidth || aDY >= dest->mHeight)
		return NS_OK;

	PRUint8 *rgbPtr=0, *alphaPtr=0;
	PRUint32 rgbStride, alphaStride;

	rgbPtr = mImageBits;
	rgbStride = mRowBytes;
	alphaPtr = mAlphaBits;
	alphaStride = mAlphaRowBytes;

	PRInt32 y;
	PRInt32 ValidWidth = ( aDWidth < ( dest->mWidth - aDX ) ) ? aDWidth : ( dest->mWidth - aDX ); 
	PRInt32 ValidHeight = ( aDHeight < ( dest->mHeight - aDY ) ) ? aDHeight : ( dest->mHeight - aDY );

	
	switch (mAlphaDepth)
	{
		case 1:
		{
			PRUint8 *dst = dest->mImageBits + aDY*dest->mRowBytes + 3*aDX;
			PRUint8 *dstAlpha = dest->mAlphaBits + aDY*dest->mAlphaRowBytes;
			PRUint8 *src = rgbPtr;
			PRUint8 *alpha = alphaPtr;
			PRUint8 offset = aDX & 0x7; 
			PRUint8 offset_8U = 8U - offset;
			int iterations = (ValidWidth+7)/8; 
			PRUint32  dst_it_stride = dest->mRowBytes - 3*8*iterations;
			PRUint32  src_it_stride = rgbStride - 3*8*iterations;
			PRUint32  alpha_it_stride = alphaStride - iterations;

			for (y=0; y < ValidHeight; ++y)
			{
				for (int x=0; x < ValidWidth; x += 8, dst += 24, src += 24)
				{
					PRUint8 alphaPixels = *alpha++;
					PRInt32  VW_x = ValidWidth-x;
					if (alphaPixels == 0)
						continue; 

					
					
					if (x+7 >= ValidWidth)
					{
						alphaPixels &= 0xff << (8 - VW_x); 
						if (alphaPixels == 0)
							continue;  
					}
					if (offset == 0)
					{
						dstAlpha[(aDX+x)>>3] |= alphaPixels; 
					}
					else
					{
						dstAlpha[(aDX+x)>>3] |= alphaPixels >> offset;
						
						
						if (alphaPixels << offset_8U)
							dstAlpha[((aDX+x)>>3) + 1] |= alphaPixels << offset_8U;
					}
          
					if (alphaPixels == 0xff)
					{
						
						
						memcpy(dst,src,24);
						continue;
					}
					else
					{
						
						
						PRUint8 *d = dst, *s = src;
						for (PRUint8 aMask = 1<<7, j = 0; aMask && j < VW_x; aMask >>= 1, ++j)
						{
							
							if (alphaPixels & aMask)
							{
								
								d[0] = s[0];
								d[1] = s[1];
								d[2] = s[2];
								
							}
							d += 3;
							s += 3;
						}
					}
				}
				
				dst = dst + dst_it_stride;
				src = src + src_it_stride;
				alpha = alpha + alpha_it_stride;
				dstAlpha += dest->mAlphaRowBytes;
			}
		}
		break;
		case 0:
		default:
			for (y=0; y < ValidHeight; ++y)
				memcpy(dest->mImageBits + (y+aDY)*dest->mRowBytes + 3*aDX, 
						rgbPtr + y*rgbStride, 3*ValidWidth);
	}
	
	
	
	nsRect rect(aDX, aDY, ValidWidth, ValidHeight);
	dest->ImageUpdated(nsnull, 0, &rect);
	mImageCurrent = PR_TRUE;

	return NS_OK;
}
