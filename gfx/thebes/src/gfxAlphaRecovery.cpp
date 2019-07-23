





































#include "gfxAlphaRecovery.h"

#include "gfxImageSurface.h"


struct gfxAlphaRecoveryResult {
    gfxAlphaRecoveryResult()
        : uniformColor(PR_FALSE),
          uniformAlpha(PR_FALSE)
    { }
    PRBool uniformColor;
    PRBool uniformAlpha;
    gfxFloat alpha;
    gfxFloat r, g, b;
};

static void _compute_alpha_values (unsigned int *black_data,
                                   unsigned int *white_data,
                                   gfxIntSize dimensions,
                                   gfxAlphaRecoveryResult *result);

already_AddRefed<gfxImageSurface>
gfxAlphaRecovery::RecoverAlpha (gfxImageSurface *blackSurf,
                                gfxImageSurface *whiteSurf,
                                gfxIntSize dimensions)
{

    nsRefPtr<gfxImageSurface> resultSurf;
    resultSurf = new gfxImageSurface(dimensions, gfxASurface::ImageFormatARGB32);

    
    nsRefPtr<gfxContext> ctx = new gfxContext(resultSurf);
    ctx->SetSource(blackSurf);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->Paint();
    ctx = nsnull;

    gfxAlphaRecoveryResult result;
    _compute_alpha_values ((unsigned int*) resultSurf->Data(),
                           (unsigned int*) whiteSurf->Data(),
                           dimensions,
                           &result);

    

    NS_ADDREF(resultSurf.get());
    return resultSurf.get();
}




















#define SET_ALPHA(v, a) (((v) & ~(0xFF << 24)) | ((a) << 24))
#define GREEN_OF(v) (((v) >> 8) & 0xFF)

static void
_compute_alpha_values (unsigned int *black_data,
                       unsigned int *white_data,
                       gfxIntSize dimensions,
                       gfxAlphaRecoveryResult *result)
{
    int num_pixels = dimensions.width * dimensions.height;
    int i;
    unsigned int first;
    unsigned int deltas = 0;
    unsigned char first_alpha;
  
    if (num_pixels == 0) {
        if (result) {
            result->uniformAlpha = PR_TRUE;
            result->uniformColor = PR_TRUE;
            
            result->alpha = 1.0;
            result->r = result->g = result->b = 0.0;
        }
        return;
    }
  
    first_alpha = 255 - (GREEN_OF(*white_data) - GREEN_OF(*black_data));
    
    first = SET_ALPHA(*black_data, first_alpha);
  
    for (i = 0; i < num_pixels; ++i) {
        unsigned int black = *black_data;
        unsigned int white = *white_data;
        unsigned char pixel_alpha = 255 - (GREEN_OF(white) - GREEN_OF(black));
        
        black = SET_ALPHA(black, pixel_alpha);
        *black_data = black;
        deltas |= (first ^ black);
        
        black_data++;
        white_data++;
    }
    
    if (result) {
        result->uniformAlpha = (deltas >> 24) == 0;
        if (result->uniformAlpha) {
            result->alpha = first_alpha/255.0;
            



            result->uniformColor = (deltas & ~(0xFF << 24)) == 0;
            if (result->uniformColor) {
                if (first_alpha == 0) {
                    
                    result->r = result->g = result->b = 0.0;
                } else {
                    double d_first_alpha = first_alpha;
                    result->r = (first & 0xFF)/d_first_alpha;
                    result->g = ((first >> 8) & 0xFF)/d_first_alpha;
                    result->b = ((first >> 16) & 0xFF)/d_first_alpha;
                }
            }
        }
    }
}
