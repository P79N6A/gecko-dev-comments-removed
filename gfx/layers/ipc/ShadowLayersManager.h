





#ifndef mozilla_layers_ShadowLayersManager_h
#define mozilla_layers_ShadowLayersManager_h

namespace mozilla {
namespace layers {

class TargetConfig;
class ShadowLayersParent;

class ShadowLayersManager
{
public:
    virtual void ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                     const TargetConfig& aTargetConfig,
                                     bool isFirstPaint) = 0;
};

} 
} 

#endif 
