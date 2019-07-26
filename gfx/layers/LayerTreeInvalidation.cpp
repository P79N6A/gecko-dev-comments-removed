




#include "LayerTreeInvalidation.h"
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "gfx3DMatrix.h"                
#include "gfxColor.h"                   
#include "GraphicsFilter.h"             
#include "gfxPoint.h"                   
#include "gfxPoint3D.h"                 
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDataHashtable.h"            
#include "nsDebug.h"                    
#include "nsHashKeys.h"                 
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              

namespace mozilla {
namespace layers {

struct LayerPropertiesBase;
LayerPropertiesBase* CloneLayerTreePropertiesInternal(Layer* aRoot);

static nsIntRect
TransformRect(const nsIntRect& aRect, const gfx3DMatrix& aTransform)
{
  if (aRect.IsEmpty()) {
    return nsIntRect();
  }

  gfxRect rect(aRect.x, aRect.y, aRect.width, aRect.height);
  rect = aTransform.TransformBounds(rect);
  rect.RoundOut();

  nsIntRect intRect;
  if (!gfxUtils::GfxRectToIntRect(rect, &intRect)) {
    return nsIntRect();
  }

  return intRect;
}

static void
AddTransformedRegion(nsIntRegion& aDest, const nsIntRegion& aSource, const gfx3DMatrix& aTransform)
{
  nsIntRegionRectIterator iter(aSource);
  const nsIntRect *r;
  while ((r = iter.Next())) {
    aDest.Or(aDest, TransformRect(*r, aTransform));
  }
  aDest.SimplifyOutward(4);
}

static void
AddRegion(nsIntRegion& aDest, const nsIntRegion& aSource)
{
  aDest.Or(aDest, aSource);
  aDest.SimplifyOutward(4);
}

static nsIntRegion
TransformRegion(const nsIntRegion& aRegion, const gfx3DMatrix& aTransform)
{
  nsIntRegion result;
  AddTransformedRegion(result, aRegion, aTransform);
  return result;
}






static void
NotifySubdocumentInvalidationRecursive(Layer* aLayer, NotifySubDocInvalidationFunc aCallback)
{
  aLayer->ClearInvalidRect();
  ContainerLayer* container = aLayer->AsContainerLayer();

  if (aLayer->GetMaskLayer()) {
    NotifySubdocumentInvalidationRecursive(aLayer->GetMaskLayer(), aCallback);
  }

  if (!container) {
    return;
  }

  for (Layer* child = container->GetFirstChild(); child; child = child->GetNextSibling()) {
    NotifySubdocumentInvalidationRecursive(child, aCallback);
  }

  aCallback(container, container->GetVisibleRegion());
}

struct LayerPropertiesBase : public LayerProperties
{
  LayerPropertiesBase(Layer* aLayer)
    : mLayer(aLayer)
    , mMaskLayer(nullptr)
    , mVisibleRegion(aLayer->GetVisibleRegion())
    , mTransform(aLayer->GetTransform())
    , mOpacity(aLayer->GetOpacity())
    , mUseClipRect(!!aLayer->GetClipRect())
  {
    MOZ_COUNT_CTOR(LayerPropertiesBase);
    if (aLayer->GetMaskLayer()) {
      mMaskLayer = CloneLayerTreePropertiesInternal(aLayer->GetMaskLayer());
    }
    if (mUseClipRect) {
      mClipRect = *aLayer->GetClipRect();
    }
  }
  LayerPropertiesBase()
    : mLayer(nullptr)
    , mMaskLayer(nullptr)
  {
    MOZ_COUNT_CTOR(LayerPropertiesBase);
  }
  ~LayerPropertiesBase()
  {
    MOZ_COUNT_DTOR(LayerPropertiesBase);
  }
  
  virtual nsIntRegion ComputeDifferences(Layer* aRoot, 
                                         NotifySubDocInvalidationFunc aCallback);

  virtual void MoveBy(const nsIntPoint& aOffset);

  nsIntRegion ComputeChange(NotifySubDocInvalidationFunc aCallback)
  {
    bool transformChanged = !mTransform.FuzzyEqual(mLayer->GetTransform());
    Layer* otherMask = mLayer->GetMaskLayer();
    const nsIntRect* otherClip = mLayer->GetClipRect();
    nsIntRegion result;
    if ((mMaskLayer ? mMaskLayer->mLayer : nullptr) != otherMask ||
        (mUseClipRect != !!otherClip) ||
        mLayer->GetOpacity() != mOpacity ||
        transformChanged) 
    {
      result = OldTransformedBounds();
      if (transformChanged) {
        AddRegion(result, NewTransformedBounds());
      }

      
      
      
      if (!aCallback) {
        ClearInvalidations(mLayer);
        return result;
      }
    }

    nsIntRegion visible;
    visible.Xor(mVisibleRegion, mLayer->GetVisibleRegion());
    AddTransformedRegion(result, visible, mTransform);

    AddRegion(result, ComputeChangeInternal(aCallback));
    AddTransformedRegion(result, mLayer->GetInvalidRegion(), mTransform);

    if (mMaskLayer && otherMask) {
      AddTransformedRegion(result, mMaskLayer->ComputeChange(aCallback), mTransform);
    }

    if (mUseClipRect && otherClip) {
      if (!mClipRect.IsEqualInterior(*otherClip)) {
        nsIntRegion tmp; 
        tmp.Xor(mClipRect, *otherClip); 
        AddRegion(result, tmp);
      }
    }

    mLayer->ClearInvalidRect();
    return result;
  }

  nsIntRect NewTransformedBounds()
  {
    return TransformRect(mLayer->GetVisibleRegion().GetBounds(), mLayer->GetTransform());
  }

  nsIntRect OldTransformedBounds()
  {
    return TransformRect(mVisibleRegion.GetBounds(), mTransform);
  }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback) { return nsIntRect(); }

  nsRefPtr<Layer> mLayer;
  nsAutoPtr<LayerPropertiesBase> mMaskLayer;
  nsIntRegion mVisibleRegion;
  gfx3DMatrix mTransform;
  float mOpacity;
  nsIntRect mClipRect;
  bool mUseClipRect;
};

struct ContainerLayerProperties : public LayerPropertiesBase
{
  ContainerLayerProperties(ContainerLayer* aLayer)
    : LayerPropertiesBase(aLayer)
  {
    for (Layer* child = aLayer->GetFirstChild(); child; child = child->GetNextSibling()) {
      mChildren.AppendElement(CloneLayerTreePropertiesInternal(child));
    }
  }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback)
  {
    ContainerLayer* container = mLayer->AsContainerLayer();
    nsIntRegion result;

    
    
    
    
    
    
    
    

    nsDataHashtable<nsPtrHashKey<Layer>, uint32_t> oldIndexMap(mChildren.Length());
    for (uint32_t i = 0; i < mChildren.Length(); ++i) {
      oldIndexMap.Put(mChildren[i]->mLayer, i);
    }

    uint32_t i = 0; 
    for (Layer* child = container->GetFirstChild(); child; child = child->GetNextSibling()) {
      bool invalidateChildsCurrentArea = false;
      if (i < mChildren.Length()) {
        uint32_t childsOldIndex;
        if (oldIndexMap.Get(child, &childsOldIndex)) {
          if (childsOldIndex >= i) {
            
            
            
            
            
            for (uint32_t j = i; j < childsOldIndex; ++j) {
              AddRegion(result, mChildren[j]->OldTransformedBounds());
            }
            
            AddRegion(result, mChildren[childsOldIndex]->ComputeChange(aCallback));
            i = childsOldIndex + 1;
          } else {
            
            
            
            invalidateChildsCurrentArea = true;
          }
        } else {
          
          invalidateChildsCurrentArea = true;
        }
      } else {
        
        invalidateChildsCurrentArea = true;
      }
      if (invalidateChildsCurrentArea) {
        AddTransformedRegion(result, child->GetVisibleRegion(), child->GetTransform());
        if (aCallback) {
          NotifySubdocumentInvalidationRecursive(child, aCallback);
        } else {
          ClearInvalidations(child);
        }
      }
    }

    
    while (i < mChildren.Length()) {
      AddRegion(result, mChildren[i]->OldTransformedBounds());
      i++;
    }

    if (aCallback) {
      aCallback(container, result);
    }

    return TransformRegion(result, mLayer->GetTransform());
  }

  
  nsAutoTArray<nsAutoPtr<LayerPropertiesBase>,1> mChildren;
};

struct ColorLayerProperties : public LayerPropertiesBase
{
  ColorLayerProperties(ColorLayer *aLayer)
    : LayerPropertiesBase(aLayer)
    , mColor(aLayer->GetColor())
  { }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback)
  {
    ColorLayer* color = static_cast<ColorLayer*>(mLayer.get());

    if (mColor != color->GetColor()) {
      return NewTransformedBounds();
    }

    return nsIntRegion();
  }

  gfxRGBA mColor;
};

struct ImageLayerProperties : public LayerPropertiesBase
{
  ImageLayerProperties(ImageLayer* aImage)
    : LayerPropertiesBase(aImage)
    , mVisibleRegion(aImage->GetVisibleRegion())
    , mContainer(aImage->GetContainer())
    , mFilter(aImage->GetFilter())
    , mScaleToSize(aImage->GetScaleToSize())
    , mScaleMode(aImage->GetScaleMode())
  {
  }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback)
  {
    ImageLayer* imageLayer = static_cast<ImageLayer*>(mLayer.get());
    
    if (!imageLayer->GetVisibleRegion().IsEqual(mVisibleRegion)) {
      nsIntRect result = NewTransformedBounds();
      result = result.Union(OldTransformedBounds());
      return result;
    }

    if (mContainer != imageLayer->GetContainer() ||
        mFilter != imageLayer->GetFilter() ||
        mScaleToSize != imageLayer->GetScaleToSize() ||
        mScaleMode != imageLayer->GetScaleMode()) {
      return NewTransformedBounds();
    }

    return nsIntRect();
  }

  nsIntRegion mVisibleRegion;
  nsRefPtr<ImageContainer> mContainer;
  GraphicsFilter mFilter;
  gfxIntSize mScaleToSize;
  ScaleMode mScaleMode;
};

LayerPropertiesBase*
CloneLayerTreePropertiesInternal(Layer* aRoot)
{
  if (!aRoot) {
    return new LayerPropertiesBase();
  }

  switch (aRoot->GetType()) {
    case Layer::TYPE_CONTAINER:  return new ContainerLayerProperties(aRoot->AsContainerLayer());
    case Layer::TYPE_COLOR:  return new ColorLayerProperties(static_cast<ColorLayer*>(aRoot));
    case Layer::TYPE_IMAGE:  return new ImageLayerProperties(static_cast<ImageLayer*>(aRoot));
    default: return new LayerPropertiesBase(aRoot);
  }

  return nullptr;
}

 LayerProperties*
LayerProperties::CloneFrom(Layer* aRoot)
{
  return CloneLayerTreePropertiesInternal(aRoot);
}

 void 
LayerProperties::ClearInvalidations(Layer *aLayer)
{
  aLayer->ClearInvalidRect();
  if (aLayer->GetMaskLayer()) {
    ClearInvalidations(aLayer->GetMaskLayer());
  }

  ContainerLayer* container = aLayer->AsContainerLayer();
  if (!container) {
    return;
  }

  for (Layer* child = container->GetFirstChild(); child; child = child->GetNextSibling()) {
    ClearInvalidations(child);
  }
}

nsIntRegion
LayerPropertiesBase::ComputeDifferences(Layer* aRoot, NotifySubDocInvalidationFunc aCallback)
{
  NS_ASSERTION(aRoot, "Must have a layer tree to compare against!");
  if (mLayer != aRoot) {
    if (aCallback) {
      NotifySubdocumentInvalidationRecursive(aRoot, aCallback);
    } else {
      ClearInvalidations(aRoot);
    }
    nsIntRect result = TransformRect(aRoot->GetVisibleRegion().GetBounds(), aRoot->GetTransform());
    result = result.Union(OldTransformedBounds());
    return result;
  } else {
    return ComputeChange(aCallback);
  }
}
  
void 
LayerPropertiesBase::MoveBy(const nsIntPoint& aOffset)
{
  mTransform.TranslatePost(gfxPoint3D(aOffset.x, aOffset.y, 0)); 
}

} 
} 
