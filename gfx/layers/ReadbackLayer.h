




#ifndef GFX_READBACKLAYER_H
#define GFX_READBACKLAYER_H

#include <stdint.h>                     
#include "Layers.h"                     
#include "gfxColor.h"                   
#include "gfxRect.h"                    
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "nscore.h"                     

class gfxContext;

namespace mozilla {
namespace layers {

class ReadbackProcessor;

namespace layerscope {
class LayersPacket;
}






class ReadbackSink {
public:
  ReadbackSink() {}
  virtual ~ReadbackSink() {}

  


  virtual void SetUnknown(uint64_t aSequenceNumber) = 0;
  















  virtual already_AddRefed<gfxContext>
      BeginUpdate(const nsIntRect& aRect, uint64_t aSequenceNumber) = 0;
  





  virtual void EndUpdate(gfxContext* aContext, const nsIntRect& aRect) = 0;
};













class ReadbackLayer : public Layer {
public:
  MOZ_LAYER_DECL_NAME("ReadbackLayer", TYPE_READBACK)

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
  {
    
    
    
    
    mEffectiveTransform =
        SnapTransform(GetLocalTransform(), gfxRect(0, 0, mSize.width, mSize.height),
                      nullptr)*
        SnapTransformTranslation(aTransformToSurface, nullptr);
  }

  









  void SetSink(ReadbackSink* aSink)
  {
    SetUnknown();
    mSink = aSink;
  }
  ReadbackSink* GetSink() { return mSink; }

  





  void SetSize(const nsIntSize& aSize)
  {
    NS_ASSERTION(!mSink, "Should have no sink while changing size!");
    mSize = aSize;
  }
  const nsIntSize& GetSize() { return mSize; }
  nsIntRect GetRect() { return nsIntRect(nsIntPoint(0, 0), mSize); }

  bool IsBackgroundKnown()
  {
    return mBackgroundLayer || mBackgroundColor.a == 1.0;
  }

  void NotifyRemoved() {
    SetUnknown();
    mSink = nullptr;
  }

  void NotifyThebesLayerRemoved(ThebesLayer* aLayer)
  {
    if (mBackgroundLayer == aLayer) {
      mBackgroundLayer = nullptr;
    }
  }

  const nsIntPoint& GetBackgroundLayerOffset() { return mBackgroundLayerOffset; }

  uint64_t AllocateSequenceNumber() { return ++mSequenceCounter; }

  void SetUnknown()
  {
    if (IsBackgroundKnown()) {
      if (mSink) {
        mSink->SetUnknown(AllocateSequenceNumber());
      }
      mBackgroundLayer = nullptr;
      mBackgroundColor = gfxRGBA(0,0,0,0);
    }
  }

protected:
  friend class ReadbackProcessor;

  ReadbackLayer(LayerManager* aManager, void* aImplData) :
    Layer(aManager, aImplData),
    mSequenceCounter(0),
    mSize(0,0),
    mBackgroundLayer(nullptr),
    mBackgroundLayerOffset(0, 0),
    mBackgroundColor(gfxRGBA(0,0,0,0))
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent);

  uint64_t mSequenceCounter;
  nsAutoPtr<ReadbackSink> mSink;
  nsIntSize mSize;

  
  
  
  
  
  
  
  ThebesLayer* mBackgroundLayer;
  
  
  
  nsIntPoint   mBackgroundLayerOffset;
  
  
  
  gfxRGBA      mBackgroundColor;
};

}
}
#endif 
