




#include "nsRenderingContext.h"

void
nsRenderingContext::Init(gfxContext *aThebesContext)
{
    mThebes = aThebesContext;
    mThebes->SetLineWidth(1.0);
}

void
nsRenderingContext::Init(DrawTarget *aDrawTarget)
{
    Init(new gfxContext(aDrawTarget));
}

void
nsRenderingContext::SetTextRunRTL(bool aIsRTL)
{
    mFontMetrics->SetTextRunRTL(aIsRTL);
}

void
nsRenderingContext::SetFont(nsFontMetrics *aFontMetrics)
{
    mFontMetrics = aFontMetrics;
}
