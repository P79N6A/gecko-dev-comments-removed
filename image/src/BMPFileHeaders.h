



#ifndef mozilla_image_src_BMPFileHeaders_h
#define mozilla_image_src_BMPFileHeaders_h

namespace mozilla {
namespace image {

struct BMPFILEHEADER {
  char signature[2];   
  uint32_t filesize;
  int32_t reserved;    
  uint32_t dataoffset; 

  uint32_t bihsize;
};


#define BFH_LENGTH 14


#define BFH_INTERNAL_LENGTH 18

#define OS2_INTERNAL_BIH_LENGTH 8
#define WIN_V3_INTERNAL_BIH_LENGTH 36
#define WIN_V5_INTERNAL_BIH_LENGTH 120

#define OS2_BIH_LENGTH 12     // This is the real BIH size (as contained in the
                              
#define WIN_V3_BIH_LENGTH 40  // This is the real BIH size (as contained in the
                              
#define WIN_V5_BIH_LENGTH 124 // This is the real BIH size (as contained in the
                              

#define OS2_HEADER_LENGTH (BFH_INTERNAL_LENGTH + OS2_INTERNAL_BIH_LENGTH)
#define WIN_V3_HEADER_LENGTH (BFH_INTERNAL_LENGTH + WIN_V3_INTERNAL_BIH_LENGTH)
#define WIN_V5_HEADER_LENGTH (BFH_INTERNAL_LENGTH + WIN_V5_INTERNAL_BIH_LENGTH)

#ifndef LCS_sRGB
#define LCS_sRGB 0x73524742
#endif

struct xyz {
  int32_t x, y, z;
};

struct xyzTriple {
  xyz r, g, b;
};

struct BITMAPV5HEADER {
  int32_t width;             
  int32_t height;            
  uint16_t planes;           
  uint16_t bpp;              
  
  uint32_t compression;      
  uint32_t image_size;       
                             
  uint32_t xppm;             
  uint32_t yppm;             
  uint32_t colors;           
  uint32_t important_colors; 
  uint32_t red_mask;         
  uint32_t green_mask;       
  uint32_t blue_mask;        
  uint32_t alpha_mask;       
  uint32_t color_space;      
  
  xyzTriple white_point;     
  uint32_t gamma_red;        
  uint32_t gamma_green;      
  uint32_t gamma_blue;       
  uint32_t intent;           
  
  uint32_t profile_offset;   
  uint32_t profile_size;     
  uint32_t reserved;         
};

struct colorTable {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct bitFields {
  uint32_t red;
  uint32_t green;
  uint32_t blue;
  uint8_t redLeftShift;
  uint8_t redRightShift;
  uint8_t greenLeftShift;
  uint8_t greenRightShift;
  uint8_t blueLeftShift;
  uint8_t blueRightShift;
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
