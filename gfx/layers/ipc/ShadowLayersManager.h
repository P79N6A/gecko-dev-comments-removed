





#ifndef mozilla_layers_ShadowLayersManager_h
#define mozilla_layers_ShadowLayersManager_h

namespace mozilla {
namespace layers {

class ShadowLayersParent;

class ShadowLayersManager
{
public:
    virtual void ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                     
                                     bool isFirstPaint) = 0;
};

} 
} 

#endif 
