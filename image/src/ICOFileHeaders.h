





































#ifndef MOZILLA_IMAGELIB_ICOHEADERS_H_
#define MOZILLA_IMAGELIB_ICOHEADERS_H_

namespace mozilla {
  namespace imagelib {

    #define ICONFILEHEADERSIZE 6
    #define ICODIRENTRYSIZE 16
    #define PNGSIGNATURESIZE 8
    #define BMPFILEHEADERSIZE 14

    struct IconFileHeader
    {
      PRUint16   mReserved;
      PRUint16   mType;
      PRUint16   mCount;
    };

    struct IconDirEntry
    {
      PRUint8   mWidth;
      PRUint8   mHeight;
      PRUint8   mColorCount;
      PRUint8   mReserved;
      union {
        PRUint16 mPlanes;   
        PRUint16 mXHotspot; 
      };
      union {
        PRUint16 mBitCount; 
        PRUint16 mYHotspot; 
      };
      PRUint32  mBytesInRes;
      PRUint32  mImageOffset;
    };


  } 
} 

#endif
