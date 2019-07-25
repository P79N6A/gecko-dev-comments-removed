




































#ifndef MOZILLA_IMAGELIB_BMPHEADERS_H_
#define MOZILLA_IMAGELIB_BMPHEADERS_H_

namespace mozilla {
  namespace imagelib {

    struct BMPFILEHEADER {
      char signature[2]; 
      PRUint32 filesize;
      PRInt32 reserved; 
      PRUint32 dataoffset; 

      PRUint32 bihsize;
    };


#define BFH_LENGTH 14 


#define BFH_INTERNAL_LENGTH 18

#define OS2_INTERNAL_BIH_LENGTH 8
#define WIN_INTERNAL_BIH_LENGTH 36

#define OS2_BIH_LENGTH 12 // This is the real BIH size (as contained in the bihsize field of BMPFILEHEADER)
#define WIN_BIH_LENGTH 40 // This is the real BIH size (as contained in the bihsize field of BMPFILEHEADER)

#define OS2_HEADER_LENGTH (BFH_INTERNAL_LENGTH + OS2_INTERNAL_BIH_LENGTH)
#define WIN_HEADER_LENGTH (BFH_INTERNAL_LENGTH + WIN_INTERNAL_BIH_LENGTH)

    struct BMPINFOHEADER {
      PRInt32 width; 
      PRInt32 height; 
      PRUint16 planes; 
      PRUint16 bpp; 
      
      PRUint32 compression; 
      PRUint32 image_size; 
      PRUint32 xppm; 
      PRUint32 yppm; 
      PRUint32 colors; 
      PRUint32 important_colors; 
    };

    struct colorTable {
      PRUint8 red;
      PRUint8 green;
      PRUint8 blue;
    };

    struct bitFields {
      PRUint32 red;
      PRUint32 green;
      PRUint32 blue;
      PRUint8 redLeftShift;
      PRUint8 redRightShift;
      PRUint8 greenLeftShift;
      PRUint8 greenRightShift;
      PRUint8 blueLeftShift;
      PRUint8 blueRightShift;
    };

  } 
} 

#define BITFIELD_LENGTH 12 // Length of the bitfields structure in the bmp file
#define USE_RGB


#ifndef BI_RGB
#define BI_RGB 0
#endif
#ifndef BI_RLE8
#define BI_RLE8 1
#endif
#ifndef BI_RLE4
#define BI_RLE4 2
#endif
#ifndef BI_BITFIELDS
#define BI_BITFIELDS 3
#endif


#ifndef BI_ALPHABITFIELDS
#define BI_ALPHABITFIELDS 4
#endif


#define RLE_ESCAPE       0
#define RLE_ESCAPE_EOL   0
#define RLE_ESCAPE_EOF   1
#define RLE_ESCAPE_DELTA 2


enum ERLEState {
  eRLEStateInitial,
  eRLEStateNeedSecondEscapeByte,
  eRLEStateNeedXDelta,
  eRLEStateNeedYDelta,    
  eRLEStateAbsoluteMode,  
  eRLEStateAbsoluteModePadded 
};

#endif