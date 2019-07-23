





































#include "SharedDIBWin.h"
#include "nsMathUtils.h"
#include "nsDebug.h"

namespace mozilla {
namespace gfx {

SharedDIBWin::SharedDIBWin() :
    mSharedHdc(nsnull)
  , mSharedBmp(nsnull)
  , mOldObj(nsnull)
{
}

SharedDIBWin::~SharedDIBWin()
{
  Close();
}

nsresult
SharedDIBWin::Close()
{
  if (mSharedHdc && mOldObj)
    ::SelectObject(mSharedHdc, mOldObj);

  if (mSharedHdc)
    ::DeleteObject(mSharedHdc);

  if (mSharedBmp)
    ::DeleteObject(mSharedBmp);

  mSharedHdc = NULL;
  mOldObj = mSharedBmp = NULL;

  SharedDIB::Close();

  return NS_OK;
}

nsresult
SharedDIBWin::Create(HDC aHdc, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth)
{
  Close();

  
  BITMAPINFOHEADER bmih;
  PRUint32 size = SetupBitmapHeader(aWidth, aHeight, aDepth, &bmih);

  nsresult rv = SharedDIB::Create(size);
  if (NS_FAILED(rv))
    return rv;

  if (NS_FAILED(SetupSurface(aHdc, &bmih))) {
    Close();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
SharedDIBWin::Attach(Handle aHandle, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth)
{
  Close();

  BITMAPINFOHEADER bmih;
  SetupBitmapHeader(aWidth, aHeight, aDepth, &bmih);

  nsresult rv = SharedDIB::Attach(aHandle, 0);
  if (NS_FAILED(rv))
    return rv;

  if (NS_FAILED(SetupSurface(NULL, &bmih))) {
    Close();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

PRUint32
SharedDIBWin::SetupBitmapHeader(PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth, BITMAPINFOHEADER *aHeader)
{
  NS_ASSERTION(aDepth == 32, "Invalid SharedDIBWin depth");

  memset((void*)aHeader, 0, sizeof(BITMAPINFOHEADER));
  aHeader->biSize        = sizeof(BITMAPINFOHEADER);
  aHeader->biWidth       = aWidth;
  aHeader->biHeight      = aHeight;
  aHeader->biPlanes      = 1;
  aHeader->biBitCount    = aDepth;
  aHeader->biCompression = BI_RGB;

  
  return (sizeof(BITMAPINFOHEADER) + (aHeader->biHeight * aHeader->biWidth * (PRUint32)NS_ceil(aDepth/8)));
}

nsresult
SharedDIBWin::SetupSurface(HDC aHdc, BITMAPINFOHEADER *aHdr)
{
  mSharedHdc = ::CreateCompatibleDC(aHdc);

  if (!mSharedHdc)
    return NS_ERROR_FAILURE;

  void* ppvBits = nsnull;
  mSharedBmp = ::CreateDIBSection(mSharedHdc,
                                  (BITMAPINFO*)aHdr,
                                  DIB_RGB_COLORS,
                                  (void**)&ppvBits,
                                  mShMem->handle(),
                                  (unsigned long)sizeof(BITMAPINFOHEADER));
  if (!mSharedBmp)
    return NS_ERROR_FAILURE;

  mOldObj = SelectObject(mSharedHdc, mSharedBmp);

  return NS_OK;
}


} 
} 
