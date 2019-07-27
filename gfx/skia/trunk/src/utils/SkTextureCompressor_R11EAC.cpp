






#include "SkTextureCompressor.h"

#include "SkEndian.h"



#define COMPRESS_R11_EAC_FASTEST 1
















#if COMPRESS_R11_EAC_SLOW

static const int kNumR11EACPalettes = 16;
static const int kR11EACPaletteSize = 8;
static const int kR11EACModifierPalettes[kNumR11EACPalettes][kR11EACPaletteSize] = {
    {-3, -6, -9, -15, 2, 5, 8, 14},
    {-3, -7, -10, -13, 2, 6, 9, 12},
    {-2, -5, -8, -13, 1, 4, 7, 12},
    {-2, -4, -6, -13, 1, 3, 5, 12},
    {-3, -6, -8, -12, 2, 5, 7, 11},
    {-3, -7, -9, -11, 2, 6, 8, 10},
    {-4, -7, -8, -11, 3, 6, 7, 10},
    {-3, -5, -8, -11, 2, 4, 7, 10},
    {-2, -6, -8, -10, 1, 5, 7, 9},
    {-2, -5, -8, -10, 1, 4, 7, 9},
    {-2, -4, -8, -10, 1, 3, 7, 9},
    {-2, -5, -7, -10, 1, 4, 6, 9},
    {-3, -4, -7, -10, 2, 3, 6, 9},
    {-1, -2, -3, -10, 0, 1, 2, 9},
    {-4, -6, -8, -9, 3, 5, 7, 8},
    {-3, -5, -7, -9, 2, 4, 6, 8}
};



static uint64_t pack_r11eac_block(uint16_t base_cw, uint16_t palette, uint16_t multiplier,
                                  uint64_t indices) {
    SkASSERT(palette < 16);
    SkASSERT(multiplier < 16);
    SkASSERT(indices < (static_cast<uint64_t>(1) << 48));

    const uint64_t b = static_cast<uint64_t>(base_cw) << 56;
    const uint64_t m = static_cast<uint64_t>(multiplier) << 52;
    const uint64_t p = static_cast<uint64_t>(palette) << 48;
    return SkEndian_SwapBE64(b | m | p | indices);
}



static uint16_t compute_r11eac_pixel(int base_cw, int modifier, int multiplier) {
    int ret = (base_cw * 8 + 4) + (modifier * multiplier * 8);
    return (ret > 2047)? 2047 : ((ret < 0)? 0 : ret);
}







static inline uint64_t compress_heterogeneous_r11eac_block(const uint8_t block[16]) {
    
    uint16_t bmin = block[0];
    uint16_t bmax = block[0];
    for (int i = 1; i < 16; ++i) {
        bmin = SkTMin<uint16_t>(bmin, block[i]);
        bmax = SkTMax<uint16_t>(bmax, block[i]);
    }

    uint16_t center = (bmax + bmin) >> 1;
    SkASSERT(center <= 255);

    
    
    uint16_t multiplier = (bmax - center) / 10;

    
    
    uint16_t cblock[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int srcIdx = i*4+j;
            int dstIdx = j*4+i;
            cblock[dstIdx] = (block[srcIdx] << 3) | (block[srcIdx] >> 5);
        }
    }

    
    uint32_t bestError = 0xFFFFFFFF;
    uint64_t bestIndices = 0;
    uint16_t bestPalette = 0;
    for (uint16_t paletteIdx = 0; paletteIdx < kNumR11EACPalettes; ++paletteIdx) {
        const int *palette = kR11EACModifierPalettes[paletteIdx];

        
        
        
        uint32_t error = 0;
        uint64_t indices = 0;
        for (int pixelIdx = 0; pixelIdx < 16; ++pixelIdx) {
            const uint16_t pixel = cblock[pixelIdx];

            
            
            uint16_t bestPixelError =
                abs_diff(pixel, compute_r11eac_pixel(center, palette[0], multiplier));
            int bestIndex = 0;
            for (int i = 1; i < kR11EACPaletteSize; ++i) {
                const uint16_t p = compute_r11eac_pixel(center, palette[i], multiplier);
                const uint16_t perror = abs_diff(pixel, p);

                
                if (perror < bestPixelError) {
                    bestIndex = i;
                    bestPixelError = perror;
                }
            }

            SkASSERT(bestIndex < 8);

            error += bestPixelError;
            indices <<= 3;
            indices |= bestIndex;
        }

        SkASSERT(indices < (static_cast<uint64_t>(1) << 48));

        
        if (error < bestError) {
            bestPalette = paletteIdx;
            bestIndices = indices;
            bestError = error;
        }
    }

    
    return pack_r11eac_block(center, bestPalette, multiplier, bestIndices);
}
#endif 

#if COMPRESS_R11_EAC_FAST






















static inline uint64_t compress_heterogeneous_r11eac_block(const uint8_t block[16]) {
    uint64_t retVal = static_cast<uint64_t>(0x8490) << 48;
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            const int shift = 45-3*(j*4+i);
            SkASSERT(shift <= 45);
            const uint64_t idx = block[i*4+j] >> 5;
            SkASSERT(idx < 8);

            
            switch(idx) {
                case 0:
                case 1:
                case 2:
                case 3:
                    retVal |= (3-idx) << shift;
                    break;
                default:
                    retVal |= idx << shift;
                    break;
            }
        }
    }

    return SkEndian_SwapBE64(retVal);
}
#endif 

#if (COMPRESS_R11_EAC_SLOW) || (COMPRESS_R11_EAC_FAST)
static uint64_t compress_r11eac_block(const uint8_t block[16]) {
    
    bool solid = true;
    for (int i = 1; i < 16; ++i) {
        if (block[i] != block[0]) {
            solid = false;
            break;
        }
    }

    if (solid) {
        switch(block[0]) {
            
            case 0:
                
                
                
                
                
                
                
                
                
                
                
                
                
                return 0x0020000000002000ULL;

            
            case 255:
            
                
                
                
                
                
                
                
                return 0xFFFFFFFFFFFFFFFFULL;

            default:
                
                
                
                
                
                
                break;
        }
    }

    return compress_heterogeneous_r11eac_block(block);
}





typedef uint64_t (*A84x4To64BitProc)(const uint8_t block[]);

static bool compress_4x4_a8_to_64bit(uint8_t* dst, const uint8_t* src,
                                     int width, int height, int rowBytes,
                                     A84x4To64BitProc proc) {
    
    if (0 == width || 0 == height || (width % 4) != 0 || (height % 4) != 0) {
        return false;
    }

    int blocksX = width >> 2;
    int blocksY = height >> 2;

    uint8_t block[16];
    uint64_t* encPtr = reinterpret_cast<uint64_t*>(dst);
    for (int y = 0; y < blocksY; ++y) {
        for (int x = 0; x < blocksX; ++x) {
            
            for (int k = 0; k < 4; ++k) {
                memcpy(block + k*4, src + k*rowBytes + 4*x, 4);
            }

            
            *encPtr = proc(block);
            ++encPtr;
        }
        src += 4 * rowBytes;
    }

    return true;
}
#endif  

#if COMPRESS_R11_EAC_FASTEST
template<unsigned shift>
static inline uint64_t swap_shift(uint64_t x, uint64_t mask) {
    const uint64_t t = (x ^ (x >> shift)) & mask;
    return x ^ t ^ (t << shift);
}

static inline uint64_t interleave6(uint64_t topRows, uint64_t bottomRows) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    uint64_t x = (static_cast<uint64_t>(topRows) << 32) | static_cast<uint64_t>(bottomRows);

    

    x = swap_shift<10>(x, 0x3FC0003FC00000ULL);

    

    x = (x | ((x << 52) & (0x3FULL << 52)) | ((x << 20) & (0x3FULL << 28))) >> 16;

    

    x = swap_shift<6>(x, 0xFC0000ULL);

#if defined (SK_CPU_BENDIAN)
    

    x = swap_shift<36>(x, 0x3FULL);

    

    x = swap_shift<12>(x, 0xFFF000000ULL);
#else
    
    
    

    x = swap_shift<36>(x, 0xFC0ULL);

    
    
    x = (x & (0xFFFULL << 36)) | ((x & 0xFFFFFFULL) << 12) | ((x >> 24) & 0xFFFULL);
#endif

    
    return x;
}















static inline uint32_t convert_indices(uint32_t x) {
    
    x = (x & 0xE0E0E0E0) >> 5;

    
    x = ~((0x80808080 - x) ^ 0x7F7F7F7F);

    
    const uint32_t s = (x & 0x7F7F7F7F) + 0x03030303;
    x = ((x ^ 0x03030303) & 0x80808080) ^ s;

    
    const uint32_t a = x & 0x80808080;
    const uint32_t b = a >> 7;

    
    const uint32_t m = (a >> 6) | b;

    
    x = (x ^ ((a - b) | a)) + b;

    
    return x + m;
}




static uint64_t compress_r11eac_block_fast(const uint8_t* src, int rowBytes) {
    
    const uint32_t alphaRow1 = *(reinterpret_cast<const uint32_t*>(src));
    const uint32_t alphaRow2 = *(reinterpret_cast<const uint32_t*>(src + rowBytes));
    const uint32_t alphaRow3 = *(reinterpret_cast<const uint32_t*>(src + 2*rowBytes));
    const uint32_t alphaRow4 = *(reinterpret_cast<const uint32_t*>(src + 3*rowBytes));

    
    
    if (alphaRow1 == alphaRow2 && alphaRow1 == alphaRow3 && alphaRow1 == alphaRow4) {
        if (0 == alphaRow1) {
            
            return 0x0020000000002000ULL;
        } else if (0xFFFFFFFF == alphaRow1) {
            
            return 0xFFFFFFFFFFFFFFFFULL;
        }
    }

    
    const uint32_t indexRow1 = convert_indices(alphaRow1);
    const uint32_t indexRow2 = convert_indices(alphaRow2);
    const uint32_t indexRow3 = convert_indices(alphaRow3);
    const uint32_t indexRow4 = convert_indices(alphaRow4);

    
    
    
    
    
    
    
    const uint32_t r1r2 = (indexRow1 << 3) | indexRow2;
    const uint32_t r3r4 = (indexRow3 << 3) | indexRow4;
    const uint64_t indices = interleave6(r1r2, r3r4);

    
    return SkEndian_SwapBE64(0x8490000000000000ULL | indices);
}

static bool compress_a8_to_r11eac_fast(uint8_t* dst, const uint8_t* src,
                                       int width, int height, int rowBytes) {
    
    if (0 == width || 0 == height || (width % 4) != 0 || (height % 4) != 0) {
        return false;
    }

    const int blocksX = width >> 2;
    const int blocksY = height >> 2;

    uint64_t* encPtr = reinterpret_cast<uint64_t*>(dst);
    for (int y = 0; y < blocksY; ++y) {
        for (int x = 0; x < blocksX; ++x) {
            
            *encPtr = compress_r11eac_block_fast(src + 4*x, rowBytes);
            ++encPtr;
        }
        src += 4 * rowBytes;
    }
    return true;
}
#endif 













static inline uint32_t pack_indices_vertical(uint32_t x) {
#if defined (SK_CPU_BENDIAN)
    return 
        (x & 7) |
        ((x >> 5) & (7 << 3)) |
        ((x >> 10) & (7 << 6)) |
        ((x >> 15) & (7 << 9));
#else
    return 
        ((x >> 24) & 7) |
        ((x >> 13) & (7 << 3)) |
        ((x >> 2) & (7 << 6)) |
        ((x << 9) & (7 << 9));
#endif
}





static inline uint64_t compress_block_vertical(const uint32_t alphaColumn0,
                                               const uint32_t alphaColumn1,
                                               const uint32_t alphaColumn2,
                                               const uint32_t alphaColumn3) {

    if (alphaColumn0 == alphaColumn1 &&
        alphaColumn2 == alphaColumn3 &&
        alphaColumn0 == alphaColumn2) {

        if (0 == alphaColumn0) {
            
            return 0x0020000000002000ULL;
        }
        else if (0xFFFFFFFF == alphaColumn0) {
            
            return 0xFFFFFFFFFFFFFFFFULL;
        }
    }

    const uint32_t indexColumn0 = convert_indices(alphaColumn0);
    const uint32_t indexColumn1 = convert_indices(alphaColumn1);
    const uint32_t indexColumn2 = convert_indices(alphaColumn2);
    const uint32_t indexColumn3 = convert_indices(alphaColumn3);

    const uint32_t packedIndexColumn0 = pack_indices_vertical(indexColumn0);
    const uint32_t packedIndexColumn1 = pack_indices_vertical(indexColumn1);
    const uint32_t packedIndexColumn2 = pack_indices_vertical(indexColumn2);
    const uint32_t packedIndexColumn3 = pack_indices_vertical(indexColumn3);

    return SkEndian_SwapBE64(0x8490000000000000ULL |
                             (static_cast<uint64_t>(packedIndexColumn0) << 36) |
                             (static_cast<uint64_t>(packedIndexColumn1) << 24) |
                             static_cast<uint64_t>(packedIndexColumn2 << 12) |
                             static_cast<uint64_t>(packedIndexColumn3));
        
}




static inline void update_block_columns(uint32_t* block, const int col,
                                        const int colsLeft, const uint32_t curAlphai) {
    SkASSERT(NULL != block);
    SkASSERT(col + colsLeft <= 4);

    for (int i = col; i < (col + colsLeft); ++i) {
        block[i] = curAlphai;
    }
}



namespace SkTextureCompressor {

bool CompressA8ToR11EAC(uint8_t* dst, const uint8_t* src, int width, int height, int rowBytes) {

#if (COMPRESS_R11_EAC_SLOW) || (COMPRESS_R11_EAC_FAST)

    return compress_4x4_a8_to_64bit(dst, src, width, height, rowBytes, compress_r11eac_block);

#elif COMPRESS_R11_EAC_FASTEST

    return compress_a8_to_r11eac_fast(dst, src, width, height, rowBytes);

#else
#error "Must choose R11 EAC algorithm"
#endif
}





class R11_EACBlitter : public SkBlitter {
public:
    R11_EACBlitter(int width, int height, void *compressedBuffer);
    virtual ~R11_EACBlitter() { this->flushRuns(); }

    
    virtual void blitH(int x, int y, int width) SK_OVERRIDE {
        
        
        
        SkFAIL("Not implemented!");
    }
    
    
    
    virtual void blitAntiH(int x, int y,
                           const SkAlpha antialias[],
                           const int16_t runs[]) SK_OVERRIDE;
    
    
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE {
        
        
        
        
        
        
        
        
        
        
        
        
        SkFAIL("Not implemented!");
    }

    
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE {
        
        
        
        SkFAIL("Not implemented!");
    }

    
    
    
    virtual void blitAntiRect(int x, int y, int width, int height,
                              SkAlpha leftAlpha, SkAlpha rightAlpha) SK_OVERRIDE {
        
        
        
        
        
        
        
        
        
        
        
        
        
        SkFAIL("Not implemented!");
    }

    
    
    virtual void blitMask(const SkMask&, const SkIRect& clip) SK_OVERRIDE {
        
        
        
        
        
        
        
        SkFAIL("Not implemented!");
    }

    
    
    
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) SK_OVERRIDE {
        return NULL;
    }

    





    virtual int requestRowsPreserved() const { return kR11_EACBlockSz; }

protected:
    virtual void onNotifyFinished() { this->flushRuns(); }

private:
    static const int kR11_EACBlockSz = 4;
    static const int kPixelsPerBlock = kR11_EACBlockSz * kR11_EACBlockSz;

    
    
    
    
    
    const int16_t kLongestRun;

    
    
    const SkAlpha kZeroAlpha;

    
    
    struct BufferedRun {
        const SkAlpha* fAlphas;
        const int16_t* fRuns;
        int fX, fY;
    } fBufferedRuns[kR11_EACBlockSz];

    
    
    int fNextRun;

    
    const int fWidth;
    const int fHeight;

    
    
    uint64_t* const fBuffer;

    
    int blocksWide() const { return fWidth / kR11_EACBlockSz; }
    int blocksTall() const { return fHeight / kR11_EACBlockSz; }
    int totalBlocks() const { return (fWidth * fHeight) / kPixelsPerBlock; }

    
    
    int getBlockOffset(int x, int y) const {
        SkASSERT(x < fWidth);
        SkASSERT(y < fHeight);
        const int blockCol = x / kR11_EACBlockSz;
        const int blockRow = y / kR11_EACBlockSz;
        return blockRow * this->blocksWide() + blockCol;
    }

    
    uint64_t *getBlock(int x, int y) const {
        return fBuffer + this->getBlockOffset(x, y);
    }

    
    
    
    void flushRuns();
};


R11_EACBlitter::R11_EACBlitter(int width, int height, void *latcBuffer)
    
    
    
    : kLongestRun(0x7FFE), kZeroAlpha(0)
    , fNextRun(0)
    , fWidth(width)
    , fHeight(height)
    , fBuffer(reinterpret_cast<uint64_t*const>(latcBuffer))
{
    SkASSERT((width % kR11_EACBlockSz) == 0);
    SkASSERT((height % kR11_EACBlockSz) == 0);
}

void R11_EACBlitter::blitAntiH(int x, int y,
                               const SkAlpha* antialias,
                               const int16_t* runs) {
    
    
    
    
    
    if (fNextRun > 0 &&
        ((x != fBufferedRuns[fNextRun-1].fX) ||
         (y-1 != fBufferedRuns[fNextRun-1].fY))) {
        this->flushRuns();
    }

    
    
    
    
    const int row = y & ~3;
    while ((row + fNextRun) < y) {
        fBufferedRuns[fNextRun].fAlphas = &kZeroAlpha;
        fBufferedRuns[fNextRun].fRuns = &kLongestRun;
        fBufferedRuns[fNextRun].fX = 0;
        fBufferedRuns[fNextRun].fY = row + fNextRun;
        ++fNextRun;
    }

    
    SkASSERT(fNextRun == (y & 3));
    SkASSERT(fNextRun == 0 || fBufferedRuns[fNextRun - 1].fY < y);

    
    fBufferedRuns[fNextRun].fAlphas = antialias;
    fBufferedRuns[fNextRun].fRuns = runs;
    fBufferedRuns[fNextRun].fX = x;
    fBufferedRuns[fNextRun].fY = y;

    
    
    if (4 == ++fNextRun) {
        this->flushRuns();
    }
}

void R11_EACBlitter::flushRuns() {

    
    if (0 == fNextRun) {
        return;
    }

#ifndef NDEBUG
    
    for (int i = 1; i < fNextRun; ++i) {
        SkASSERT(fBufferedRuns[i].fY == fBufferedRuns[i-1].fY + 1);
        SkASSERT(fBufferedRuns[i].fX == fBufferedRuns[i-1].fX);
    }
#endif

    
    
    for (int i = fNextRun; i < kR11_EACBlockSz; ++i) {
        fBufferedRuns[i].fY = fBufferedRuns[0].fY + i;
        fBufferedRuns[i].fX = fBufferedRuns[0].fX;
        fBufferedRuns[i].fAlphas = &kZeroAlpha;
        fBufferedRuns[i].fRuns = &kLongestRun;
    }

    
    SkASSERT(fNextRun > 0 && fNextRun <= 4);
    SkASSERT((fBufferedRuns[0].fY & 3) == 0);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    uint32_t c[4] = { 0, 0, 0, 0 };
    uint32_t curAlphaColumn = 0;
    SkAlpha *curAlpha = reinterpret_cast<SkAlpha*>(&curAlphaColumn);

    int nextX[kR11_EACBlockSz];
    for (int i = 0; i < kR11_EACBlockSz; ++i) {
        nextX[i] = 0x7FFFFF;
    }

    uint64_t* outPtr = this->getBlock(fBufferedRuns[0].fX, fBufferedRuns[0].fY);

    
    
    int curX = 0;
    int finalX = 0xFFFFF;
    for (int i = 0; i < kR11_EACBlockSz; ++i) {
        nextX[i] = *(fBufferedRuns[i].fRuns);
        curAlpha[i] = *(fBufferedRuns[i].fAlphas);

        finalX = SkMin32(nextX[i], finalX);
    }

    
    SkASSERT(finalX < 0xFFFFF);

    
    while (curX != finalX) {
        SkASSERT(finalX >= curX);

        
        if ((finalX - (curX & ~3)) >= kR11_EACBlockSz) {
            const int col = curX & 3;
            const int colsLeft = 4 - col;
            SkASSERT(curX + colsLeft <= finalX);

            update_block_columns(c, col, colsLeft, curAlphaColumn);

            
            *outPtr = compress_block_vertical(c[0], c[1], c[2], c[3]);
            ++outPtr;
            curX += colsLeft;
        }

        
        if ((finalX - curX) >= kR11_EACBlockSz) {
            SkASSERT((curX & 3) == 0);

            const int col = 0;
            const int colsLeft = kR11_EACBlockSz;

            update_block_columns(c, col, colsLeft, curAlphaColumn);

            
            uint64_t lastBlock = compress_block_vertical(c[0], c[1], c[2], c[3]);
            while((finalX - curX) >= kR11_EACBlockSz) {
                *outPtr = lastBlock;
                ++outPtr;
                curX += kR11_EACBlockSz;
            }
        }

        
        if (curX < finalX) {
            const int col = curX & 3;
            const int colsLeft = finalX - curX;

            update_block_columns(c, col, colsLeft, curAlphaColumn);

            curX += colsLeft;
        }

        SkASSERT(curX == finalX);

        
        for (int i = 0; i < kR11_EACBlockSz; ++i) {
            if (nextX[i] == finalX) {
                const int16_t run = *(fBufferedRuns[i].fRuns);
                fBufferedRuns[i].fRuns += run;
                fBufferedRuns[i].fAlphas += run;
                curAlpha[i] = *(fBufferedRuns[i].fAlphas);
                nextX[i] += *(fBufferedRuns[i].fRuns);
            }
        }

        finalX = 0xFFFFF;
        for (int i = 0; i < kR11_EACBlockSz; ++i) {
            finalX = SkMin32(nextX[i], finalX);
        }
    }

    
    if ((curX & 3) > 1) {
        *outPtr = compress_block_vertical(c[0], c[1], c[2], c[3]);
    }

    fNextRun = 0;
}

SkBlitter* CreateR11EACBlitter(int width, int height, void* outputBuffer) {
    return new R11_EACBlitter(width, height, outputBuffer);
}

}  
