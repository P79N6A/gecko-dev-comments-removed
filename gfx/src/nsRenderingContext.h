




#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "gfxContext.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"

namespace mozilla {
namespace gfx {
class DrawTarget;
}
}

class MOZ_STACK_CLASS nsRenderingContext MOZ_FINAL
{
    typedef mozilla::gfx::DrawTarget DrawTarget;

public:
    nsRenderingContext() {}

    nsRenderingContext(gfxContext* aThebesContext)
      : mThebes(aThebesContext)
    {}

    nsRenderingContext(already_AddRefed<gfxContext>&& aThebesContext)
      : mThebes(aThebesContext)
    {}

    nsRenderingContext(DrawTarget* aDrawTarget) {
      Init(aDrawTarget);
    }

    void Init(gfxContext* aThebesContext);
    void Init(DrawTarget* aDrawTarget);

    
    gfxContext *ThebesContext() { return mThebes; }
    DrawTarget *GetDrawTarget() { return mThebes->GetDrawTarget(); }

private:
    nsRefPtr<gfxContext> mThebes;
};

#endif  
