






#ifndef SkTextureCompressor_DEFINED
#define SkTextureCompressor_DEFINED

#include "SkImageInfo.h"
#include "SkBlitter.h"

class SkBitmap;
class SkData;

namespace SkTextureCompressor {
    
    enum Format {
        
        kLATC_Format,       
        kR11_EAC_Format,    
        kASTC_12x12_Format, 

        kLast_Format = kASTC_12x12_Format
    };
    static const int kFormatCnt = kLast_Format + 1;

    
    
    
    int GetCompressedDataSize(Format fmt, int width, int height);

    
    
    
    
    SkData* CompressBitmapToFormat(const SkBitmap& bitmap, Format format);

    
    
    
    bool CompressBufferToFormat(uint8_t* dst, const uint8_t* src, SkColorType srcColorType,
                                int width, int height, int rowBytes, Format format,
                                bool opt = true );

    
    
    
    typedef bool (*CompressionProc)(uint8_t* dst, const uint8_t* src,
                                    int width, int height, int rowBytes);

    
    
    
    SkBlitter* CreateBlitterForFormat(int width, int height, void* compressedBuffer,
                                      Format format);
}

#endif
