







































#include "ShadowLayersParent.h"
#include "ShadowLayerParent.h"
#include "ShadowLayers.h"

#include "BasicLayers.h"

namespace mozilla {
namespace layers {

ShadowLayerParent::ShadowLayerParent() : mLayer(NULL)
{
}

ShadowLayerParent::~ShadowLayerParent()
{
}

void
ShadowLayerParent::Bind(Layer* layer)
{
  mLayer = layer;
}

ContainerLayer*
ShadowLayerParent::AsContainer() const
{
  return static_cast<ContainerLayer*>(AsLayer());
}

bool
ShadowLayerParent::Recv__delete__()
{
  mLayer = NULL;
  return true;
}

} 
} 
