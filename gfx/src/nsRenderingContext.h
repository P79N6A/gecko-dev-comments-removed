




#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxContext.h"                 
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"
#include "nsAutoPtr.h"                  
#include "nsBoundingMetrics.h"          
#include "nsColor.h"                    
#include "nsCoord.h"                    
#include "nsFontMetrics.h"              
#include "nsISupports.h"                
#include "nsString.h"               
#include "nscore.h"                     

class nsIntRegion;
struct nsPoint;
struct nsRect;

class nsRenderingContext MOZ_FINAL
{
    typedef mozilla::gfx::DrawTarget DrawTarget;

public:
    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    void Init(gfxContext* aThebesContext);
    void Init(DrawTarget* aDrawTarget);

    
    gfxContext *ThebesContext() { return mThebes; }
    DrawTarget *GetDrawTarget() { return mThebes->GetDrawTarget(); }

private:
    
    ~nsRenderingContext() {}

    nsRefPtr<gfxContext> mThebes;
};

#endif  
