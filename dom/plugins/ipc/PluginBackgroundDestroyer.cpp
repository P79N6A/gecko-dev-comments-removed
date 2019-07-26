






#include "PluginBackgroundDestroyer.h"
#include "gfxSharedImageSurface.h"

using namespace mozilla;
using namespace plugins;

PluginBackgroundDestroyerParent::PluginBackgroundDestroyerParent(gfxASurface* aDyingBackground)
  : mDyingBackground(aDyingBackground)
{
}

PluginBackgroundDestroyerParent::~PluginBackgroundDestroyerParent()
{
}

void
PluginBackgroundDestroyerParent::ActorDestroy(ActorDestroyReason why)
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
