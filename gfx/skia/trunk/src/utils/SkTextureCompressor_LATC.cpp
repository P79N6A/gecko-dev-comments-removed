






#include "SkTextureCompressor_LATC.h"

#include "SkEndian.h"









template <typename T> inline T abs_diff(const T &a, const T &b) {
    return (a > b) ? (a - b) : (b - a);
}

static bool is_extremal(uint8_t pixel) {
    return 0 == pixel || 255 == pixel;
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








static const int kLATCPaletteSize = 8;
static const int kLATCBlockSize = 4;
static const int kLATCPixelsPerBlock = kLATCBlockSize * kLATCBlockSize;






















static void generate_latc_palette(uint8_t palette[], uint8_t lum0, uint8_t lum1) {
    palette[0] = lum0;
    palette[1] = lum1;
    if (lum0 > lum1) {
        for (int i = 1; i < 7; i++) {
            palette[i+1] = ((7-i)*lum0 + i*lum1) / 7;
        }
    } else {
        for (int i = 1; i < 5; i++) {
            palette[i+1] = ((5-i)*lum0 + i*lum1) / 5;
        }
        palette[6] = 0;
        palette[7] = 255;
    }
}




static uint64_t compress_latc_block_bb(const uint8_t pixels[]) {
    uint8_t minVal = 255;
    uint8_t maxVal = 0;
    for (int i = 0; i < kLATCPixelsPerBlock; ++i) {
        minVal = SkTMin(pixels[i], minVal);
        maxVal = SkTMax(pixels[i], maxVal);
    }

    SkASSERT(!is_extremal(minVal));
    SkASSERT(!is_extremal(maxVal));

    uint8_t palette[kLATCPaletteSize];
    generate_latc_palette(palette, maxVal, minVal);

    uint64_t indices = 0;
    for (int i = kLATCPixelsPerBlock - 1; i >= 0; --i) {

        
        uint8_t bestError = abs_diff(pixels[i], palette[0]);
        uint8_t idx = 0;
        for (int j = 1; j < kLATCPaletteSize; ++j) {
            uint8_t error = abs_diff(pixels[i], palette[j]);
            if (error < bestError) {
                bestError = error;
                idx = j;
            }
        }

        indices <<= 3;
        indices |= idx;
    }

    return
        SkEndian_SwapLE64(
            static_cast<uint64_t>(maxVal) |
            (static_cast<uint64_t>(minVal) << 8) |
            (indices << 16));
}




static uint64_t compress_latc_block_bb_ignore_extremal(const uint8_t pixels[]) {
    uint8_t minVal = 255;
    uint8_t maxVal = 0;
    for (int i = 0; i < kLATCPixelsPerBlock; ++i) {
        if (is_extremal(pixels[i])) {
            continue;
        }

        minVal = SkTMin(pixels[i], minVal);
        maxVal = SkTMax(pixels[i], maxVal);
    }

    SkASSERT(!is_extremal(minVal));
    SkASSERT(!is_extremal(maxVal));

    uint8_t palette[kLATCPaletteSize];
    generate_latc_palette(palette, minVal, maxVal);

    uint64_t indices = 0;
    for (int i = kLATCPixelsPerBlock - 1; i >= 0; --i) {

        
        uint8_t idx = 0;
        if (is_extremal(pixels[i])) {
            if (0xFF == pixels[i]) {
                idx = 7;
            } else if (0 == pixels[i]) {
                idx = 6;
            } else {
                SkFAIL("Pixel is extremal but not really?!");
            }
        } else {
            uint8_t bestError = abs_diff(pixels[i], palette[0]);
            for (int j = 1; j < kLATCPaletteSize - 2; ++j) {
                uint8_t error = abs_diff(pixels[i], palette[j]);
                if (error < bestError) {
                    bestError = error;
                    idx = j;
                }
            }
        }

        indices <<= 3;
        indices |= idx;
    }

    return
        SkEndian_SwapLE64(
            static_cast<uint64_t>(minVal) |
            (static_cast<uint64_t>(maxVal) << 8) |
            (indices << 16));
}











static uint64_t compress_latc_block(const uint8_t pixels[]) {
    
    int nUniquePixels = 0;
    uint8_t uniquePixels[kLATCPixelsPerBlock];
    for (int i = 0; i < kLATCPixelsPerBlock; ++i) {
        bool foundPixel = false;
        for (int j = 0; j < nUniquePixels; ++j) {
            foundPixel = foundPixel || uniquePixels[j] == pixels[i];
        }

        if (!foundPixel) {
            uniquePixels[nUniquePixels] = pixels[i];
            ++nUniquePixels;
        }
    }

    
    if (1 == nUniquePixels) {
        return SkEndian_SwapLE64(pixels[0] | (pixels[0] << 8));

    
    
    
    } else if (2 == nUniquePixels) {
        uint64_t outBlock = 0;
        for (int i = kLATCPixelsPerBlock - 1; i >= 0; --i) {
            int idx = 0;
            if (pixels[i] == uniquePixels[1]) {
                idx = 1;
            }

            outBlock <<= 3;
            outBlock |= idx;
        }
        outBlock <<= 16;
        outBlock |= (uniquePixels[0] | (uniquePixels[1] << 8));
        return SkEndian_SwapLE64(outBlock);
    }

    
    int nonExtremalPixels = 0;
    for (int i = 0; i < nUniquePixels; ++i) {
        if (!is_extremal(uniquePixels[i])) {
            ++nonExtremalPixels;
        }
    }

    
    
    if (nonExtremalPixels == nUniquePixels) {
        
        
        
        
        return compress_latc_block_bb(pixels);
    } else {
        return compress_latc_block_bb_ignore_extremal(pixels);
    }
}



namespace SkTextureCompressor {

bool CompressA8ToLATC(uint8_t* dst, const uint8_t* src, int width, int height, int rowBytes) {
    return compress_4x4_a8_to_64bit(dst, src, width, height, rowBytes, compress_latc_block);
}

SkBlitter* CreateLATCBlitter(int width, int height, void* outputBuffer) {
    
    return NULL;
}

}  
