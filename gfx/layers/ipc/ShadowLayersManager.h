





#ifndef mozilla_layers_ShadowLayersManager_h
#define mozilla_layers_ShadowLayersManager_h

namespace mozilla {
namespace layers {

class TargetConfig;
class LayerTransactionParent;
class AsyncCompositionManager;
class APZTestData;

class ShadowLayersManager
{
public:
    virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                     const uint64_t& aTransactionId,
                                     const TargetConfig& aTargetConfig,
                                     const InfallibleTArray<PluginWindowData>& aPlugins,
                                     bool aIsFirstPaint,
                                     bool aScheduleComposite,
                                     uint32_t aPaintSequenceNumber,
                                     bool aIsRepeatTransaction) = 0;

    virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aLayerTree) { return nullptr; }

    virtual void NotifyClearCachedResources(LayerTransactionParent* aLayerTree) { }

    virtual void ForceComposite(LayerTransactionParent* aLayerTree) { }
    virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                   const TimeStamp& aTime) { return true; }
    virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) { }
    virtual void ApplyAsyncProperties(LayerTransactionParent* aLayerTree) { }
    virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                                APZTestData* aOutData) { }
    virtual void SetConfirmedTargetAPZC(const LayerTransactionParent* aLayerTree,
                                        const uint64_t& aInputBlockId,
                                        const nsTArray<ScrollableLayerGuid>& aTargets) = 0;
};

} 
} 

#endif 
