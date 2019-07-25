






#include "SkFontHost.h"
#include <math.h>






#ifndef USE_PREDEFINED_GAMMA_TABLES
    
    
    #define DUMP_GAMMA_TABLESx
#endif



#include "SkGraphics.h"


void skia_set_text_gamma(float blackGamma, float whiteGamma);

#ifdef USE_PREDEFINED_GAMMA_TABLES

#include "sk_predefined_gamma.h"

void skia_set_text_gamma(float blackGamma, float whiteGamma) {}

#else   

static void build_power_table(uint8_t table[], float ee) {

    for (int i = 0; i < 256; i++) {
        float x = i / 255.f;
        
        x = powf(x, ee);
        
        int xx = SkScalarRound(SkFloatToScalar(x * 255));
        
        table[i] = SkToU8(xx);
    }
}

static bool gGammaIsBuilt;
static uint8_t gBlackGamma[256], gWhiteGamma[256];

static float gBlackGammaCoeff = 1.4f;
static float gWhiteGammaCoeff = 1/1.4f;

void skia_set_text_gamma(float blackGamma, float whiteGamma) {
    gBlackGammaCoeff = blackGamma;
    gWhiteGammaCoeff = whiteGamma;
    gGammaIsBuilt = false;
    SkGraphics::SetFontCacheUsed(0);
    build_power_table(gBlackGamma, gBlackGammaCoeff);
    build_power_table(gWhiteGamma, gWhiteGammaCoeff);
}

#ifdef DUMP_GAMMA_TABLES

#include "SkString.h"

static void dump_a_table(const char name[], const uint8_t table[],
                         float gamma) {
    SkDebugf("\n");
    SkDebugf("\/\/ Gamma table for %g\n", gamma);
    SkDebugf("static const uint8_t %s[] = {\n", name);
    for (int y = 0; y < 16; y++) {
        SkString line, tmp;
        for (int x = 0; x < 16; x++) {
            tmp.printf("0x%02X, ", *table++);
            line.append(tmp);
        }
        SkDebugf("    %s\n", line.c_str());
    }
    SkDebugf("};\n");
}

#endif

#endif



void SkFontHost::GetGammaTables(const uint8_t* tables[2]) {
#ifndef USE_PREDEFINED_GAMMA_TABLES
    if (!gGammaIsBuilt) {
        build_power_table(gBlackGamma, gBlackGammaCoeff);
        build_power_table(gWhiteGamma, gWhiteGammaCoeff);
        gGammaIsBuilt = true;

#ifdef DUMP_GAMMA_TABLES
        dump_a_table("gBlackGamma", gBlackGamma, gBlackGammaCoeff);
        dump_a_table("gWhiteGamma", gWhiteGamma, gWhiteGammaCoeff);
#endif
    }
#endif
    tables[0] = gBlackGamma;
    tables[1] = gWhiteGamma;
}


#define BLACK_GAMMA_THRESHOLD   0x40


#define WHITE_GAMMA_THRESHOLD   0xC0

int SkFontHost::ComputeGammaFlag(const SkPaint& paint) {
    if (paint.getShader() == NULL) {
        SkColor c = paint.getColor();
        int r = SkColorGetR(c);
        int g = SkColorGetG(c);
        int b = SkColorGetB(c);
        int luminance = (r * 2 + g * 5 + b) >> 3;
        
        if (luminance <= BLACK_GAMMA_THRESHOLD) {
        
            return SkScalerContext::kGammaForBlack_Flag;
        }
        if (luminance >= WHITE_GAMMA_THRESHOLD) {
        
            return SkScalerContext::kGammaForWhite_Flag;
        }
    }
    return 0;
}

