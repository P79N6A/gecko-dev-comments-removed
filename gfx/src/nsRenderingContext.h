




#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "gfxContext.h"
#include "mozilla/Attributes.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"

namespace mozilla {
namespace gfx {
class DrawTarget;
}
}

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
