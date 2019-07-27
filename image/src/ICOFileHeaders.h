




#ifndef mozilla_image_src_ICOFileHeaders_h
#define mozilla_image_src_ICOFileHeaders_h

namespace mozilla {
namespace image {

#define ICONFILEHEADERSIZE 6
#define ICODIRENTRYSIZE 16
#define PNGSIGNATURESIZE 8
#define BMPFILEHEADERSIZE 14





struct IconFileHeader
{
  


  uint16_t   mReserved;
  



  uint16_t   mType;
  


  uint16_t   mCount;
};







struct IconDirEntry
{
  uint8_t   mWidth;
  uint8_t   mHeight;
  



  uint8_t   mColorCount;
  


  uint8_t   mReserved;
  union {
    uint16_t mPlanes;   
    uint16_t mXHotspot; 
  };
  union {
    uint16_t mBitCount; 
    uint16_t mYHotspot; 
  };
  



  uint32_t  mBytesInRes;
  



  uint32_t  mImageOffset;
};


} 
} 

#endif 
