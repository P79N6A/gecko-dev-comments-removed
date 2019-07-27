






#ifndef dom_plugins_PluginBackgroundDestroyer
#define dom_plugins_PluginBackgroundDestroyer

#include "mozilla/plugins/PPluginBackgroundDestroyerChild.h"
#include "mozilla/plugins/PPluginBackgroundDestroyerParent.h"

#include "gfxSharedImageSurface.h"

class gfxASurface;

namespace mozilla {
namespace plugins {






class PluginBackgroundDestroyerParent : public PPluginBackgroundDestroyerParent {
public:
    explicit PluginBackgroundDestroyerParent(gfxASurface* aDyingBackground);

    virtual ~PluginBackgroundDestroyerParent();

private:
    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    nsRefPtr<gfxASurface> mDyingBackground;
};





class PluginBackgroundDestroyerChild : public PPluginBackgroundDestroyerChild {
public:
    PluginBackgroundDestroyerChild() { }
    virtual ~PluginBackgroundDestroyerChild() { }

private:
    
    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
    { }
};

} 
} 

#endif  
