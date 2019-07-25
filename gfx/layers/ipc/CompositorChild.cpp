






































#include "CompositorChild.h"
#include "mozilla/layers/ShadowLayersChild.h"

using mozilla::layers::ShadowLayersChild;

namespace mozilla {
namespace layers {

CompositorChild::CompositorChild()
{
    
    MOZ_COUNT_CTOR(CompositorChild);
	printf("Alloc CompositorChild\n");
}

CompositorChild::~CompositorChild()
{
    MOZ_COUNT_DTOR(CompositorChild);
}

PLayersChild*
CompositorChild::AllocPLayers(const LayersBackend &backend)
{
    return new ShadowLayersChild();
}

bool
CompositorChild::DeallocPLayers(PLayersChild* actor)
{
	delete actor;
    return true;
}

} 
} 

