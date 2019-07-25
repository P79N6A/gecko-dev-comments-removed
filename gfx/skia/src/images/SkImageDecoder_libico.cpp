








#include "SkImageDecoder.h"
#include "SkStream.h"
#include "SkColorPriv.h"
#include "SkTypes.h"

class SkICOImageDecoder : public SkImageDecoder {
public:
    SkICOImageDecoder();
    
    virtual Format getFormat() const {
        return kICO_Format;
    }

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode);
};

SkImageDecoder* SkCreateICOImageDecoder();
SkImageDecoder* SkCreateICOImageDecoder() {
    return new SkICOImageDecoder;
}






#define readByte(buffer,begin) buffer[begin]
#define read2Bytes(buffer,begin) buffer[begin]+(buffer[begin+1]<<8)
#define read4Bytes(buffer,begin) buffer[begin]+(buffer[begin+1]<<8)+(buffer[begin+2]<<16)+(buffer[begin+3]<<24)



SkICOImageDecoder::SkICOImageDecoder()
{
}


static void editPixelBit1(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors);
static void editPixelBit4(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors);
static void editPixelBit8(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors);
static void editPixelBit24(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors);
static void editPixelBit32(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors);
            
            
static int calculateRowBytesFor8888(int w, int bitCount)
{
    
    
    
    
    if (4 == bitCount && (w & 0x1)) {
        return (w + 1) << 2;
    }
    
    return 0;
}

bool SkICOImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode)
{
    size_t length = stream->read(NULL, 0);
    SkAutoMalloc autoMal(length);
    unsigned char* buf = (unsigned char*)autoMal.get();
    if (stream->read((void*)buf, length) != length) {
        return false;
    }
    
    
    
    int reserved = read2Bytes(buf, 0);    
    int type = read2Bytes(buf, 2);        
    if (reserved != 0 || type != 1)
        return false;
    int count = read2Bytes(buf, 4);
    
    
    if (length < (size_t)(6 + count*16))
        return false;
        
    int choice;
    Chooser* chooser = this->getChooser();
    
    
    if (NULL == chooser) {
        choice = 0;
    } else {
        chooser->begin(count);
        for (int i = 0; i < count; i++)
        {
            
            int width = readByte(buf, 6 + i*16);
            int height = readByte(buf, 7 + i*16);
            int offset = read4Bytes(buf, 18 + i*16);
            int bitCount = read2Bytes(buf, offset+14);
            SkBitmap::Config c;
            
            
            switch (bitCount)
            {
                case 1:
                case 4:
                    
                    
                    
                    c = SkBitmap::kIndex8_Config;
                    break;
                case 8:
                case 24:
                case 32:
                    c = SkBitmap::kARGB_8888_Config;
                    break;
                default:
                    SkDEBUGF(("Image with %ibpp not supported\n", bitCount));
                    continue;
            }
            chooser->inspect(i, c, width, height);
        }
        choice = chooser->choose();
    }
    
    
    if (choice >= count || choice < 0)       
        return false;
    
    
    
    
    int w = readByte(buf, 6 + choice*16);
    int h = readByte(buf, 7 + choice*16);
    int colorCount = readByte(buf, 8 + choice*16);
    
    
    
    int size = read4Bytes(buf, 14 + choice*16);           
    int offset = read4Bytes(buf, 18 + choice*16);
    if ((size_t)(offset + size) > length)
        return false;
    
    
    
    
    int bitCount = read2Bytes(buf, offset+14);
    
    void (*placePixel)(const int pixelNo, const unsigned char* buf, 
        const int xorOffset, int& x, int y, const int w, 
        SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors) = NULL;
    switch (bitCount)
    {
        case 1:
            placePixel = &editPixelBit1;
            colorCount = 2;
            break;
        case 4:
            placePixel = &editPixelBit4;
            colorCount = 16;
            break;
        case 8:
            placePixel = &editPixelBit8;
            colorCount = 256;
            break;
        case 24:
            placePixel = &editPixelBit24;
            colorCount = 0;
            break;
        case 32:
            placePixel = &editPixelBit32;
            colorCount = 0;
            break;
        default:
            SkDEBUGF(("Decoding %ibpp is unimplemented\n", bitCount));
            return false;
    }
        
    
    
    
    
    
    
    
        
    int begin = offset + 40;
    
    
    SkPMColor* colors = NULL;
    int blue, green, red;
    if (colorCount) 
    {
        colors = new SkPMColor[colorCount];
        for (int j = 0; j < colorCount; j++)
        {
            
            blue = readByte(buf, begin + 4*j);
            green = readByte(buf, begin + 4*j + 1);
            red = readByte(buf, begin + 4*j + 2);
            colors[j] = SkPackARGB32(0xFF, red & 0xFF, green & 0xFF, blue & 0xFF);
        }
    }
    int bitWidth = w*bitCount;
    int test = bitWidth & 0x1F;
    int mask = -(((test >> 4) | (test >> 3) | (test >> 2) | (test >> 1) | test) & 0x1);    
    int lineBitWidth = (bitWidth & 0xFFFFFFE0) + (0x20 & mask);
    int lineWidth = lineBitWidth/bitCount;
    
    int xorOffset = begin + colorCount*4;   
                                            
    int andOffset = xorOffset + ((lineWidth*h*bitCount) >> 3);
    
    test = w & 0x1F;   
    mask = -(((test >> 4) | (test >> 3) | (test >> 2) | (test >> 1) | test) & 0x1);    
    int andLineWidth = (w & 0xFFFFFFE0) + (0x20 & mask);
    
    
    
    
    

    bm->setConfig(SkBitmap::kARGB_8888_Config, w, h, calculateRowBytesFor8888(w, bitCount));
    
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        delete[] colors;
        return true;
    }

    if (!this->allocPixelRef(bm, NULL))
    {
        delete[] colors;
        return false;
    }
    
    SkAutoLockPixels alp(*bm);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            
            
            
            int andPixelNo = andLineWidth*(h-y-1)+x;
            
            
            
            int alphaByte = readByte(buf, andOffset + (andPixelNo >> 3));
            int shift = 7 - (andPixelNo & 0x7);
            int m = 1 << shift;
            
            int pixelNo = lineWidth*(h-y-1)+x;
            placePixel(pixelNo, buf, xorOffset, x, y, w, bm, alphaByte, m, shift, colors);

        }
    }

    delete [] colors;
    
    
    
    return true;
}   


static void editPixelBit1(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors)
{
    
    SkPMColor* address = bm->getAddr32(x,y);
    int byte = readByte(buf, xorOffset + (pixelNo >> 3));
    int colorBit;
    int alphaBit;
    
    int i = x + 8;
    
    
    i = i > w ? w : i;
    
    while (x < i)
    {
        
        colorBit = (byte & m) >> shift;
        alphaBit = (alphaByte & m) >> shift;
        *address = (alphaBit-1)&(colors[colorBit]);
        x++;
        
        address = address + 1;
        m = m >> 1;
        shift -= 1;
    }
    x--;
}
static void editPixelBit4(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors)
{
    SkPMColor* address = bm->getAddr32(x, y);
    int byte = readByte(buf, xorOffset + (pixelNo >> 1));
    int pixel = (byte >> 4) & 0xF;
    int alphaBit = (alphaByte & m) >> shift;
    *address = (alphaBit-1)&(colors[pixel]);
    x++;
    
    
    address = address + 1;
    pixel = byte & 0xF;
    m = m >> 1;
    alphaBit = (alphaByte & m) >> (shift-1);
    
    *address = (alphaBit-1)&(colors[pixel]);
}

static void editPixelBit8(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors)
{
    SkPMColor* address = bm->getAddr32(x, y);
    int pixel = readByte(buf, xorOffset + pixelNo);
    int alphaBit = (alphaByte & m) >> shift;
    *address = (alphaBit-1)&(colors[pixel]);
}            

static void editPixelBit24(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors)
{
    SkPMColor* address = bm->getAddr32(x, y);
    int blue = readByte(buf, xorOffset + 3*pixelNo);
    int green = readByte(buf, xorOffset + 3*pixelNo + 1);
    int red = readByte(buf, xorOffset + 3*pixelNo + 2);
    int alphaBit = (alphaByte & m) >> shift;
    
    int alpha = (alphaBit-1) & 0xFF;
    *address = SkPreMultiplyARGB(alpha, red, green, blue);    
}

static void editPixelBit32(const int pixelNo, const unsigned char* buf, 
            const int xorOffset, int& x, int y, const int w, 
            SkBitmap* bm, int alphaByte, int m, int shift, SkPMColor* colors)
{
    SkPMColor* address = bm->getAddr32(x, y);
    int blue = readByte(buf, xorOffset + 4*pixelNo);
    int green = readByte(buf, xorOffset + 4*pixelNo + 1);
    int red = readByte(buf, xorOffset + 4*pixelNo + 2);
    int alphaBit = (alphaByte & m) >> shift;
#if 1 
    alphaBit = 0;
#endif
    int alpha = readByte(buf, xorOffset + 4*pixelNo + 3) & ((alphaBit-1)&0xFF);
    *address = SkPreMultiplyARGB(alpha, red, green, blue);
}



#include "SkTRegistry.h"

static SkImageDecoder* Factory(SkStream* stream) {
    
    
    SkAutoMalloc autoMal(4);
    unsigned char* buf = (unsigned char*)autoMal.get();
    stream->read((void*)buf, 4);
    int reserved = read2Bytes(buf, 0);
    int type = read2Bytes(buf, 2);
    if (reserved != 0 || type != 1) {
        
        return NULL;
    }
    return SkNEW(SkICOImageDecoder);
}

static SkTRegistry<SkImageDecoder*, SkStream*> gReg(Factory);

