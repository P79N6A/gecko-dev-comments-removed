







































#ifndef dom_plugins_PluginBackgroundDestroyer
#define dom_plugins_PluginBackgroundDestroyer

#include "mozilla/plugins/PPluginBackgroundDestroyerChild.h"
#include "mozilla/plugins/PPluginBackgroundDestroyerParent.h"

#include "gfxASurface.h"
#include "gfxSharedImageSurface.h"

namespace mozilla {
namespace plugins {






class PluginBackgroundDestroyerParent : public PPluginBackgroundDestroyerParent {
public:
    PluginBackgroundDestroyerParent(gfxASurface* aDyingBackground)
      : mDyingBackground(aDyingBackground)
    { }

    virtual ~PluginBackgroundDestroyerParent() { }

private:
    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why)
    {
        switch(why) {
        case Deletion:
        case AncestorDeletion:
            if (gfxSharedImageSurface::IsSharedImage(mDyingBackground)) {
                gfxSharedImageSurface* s =
                    static_cast<gfxSharedImageSurface*>(mDyingBackground.get());
                DeallocShmem(s->GetShmem());
            }
            break;
        default:
            
            
            break;
        }
    }

    nsRefPtr<gfxASurface> mDyingBackground;
};





class PluginBackgroundDestroyerChild : public PPluginBackgroundDestroyerChild {
public:
    PluginBackgroundDestroyerChild() { }
    virtual ~PluginBackgroundDestroyerChild() { }

private:
    
    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why)
    { }
};

} 
} 

#endif  
