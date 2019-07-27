




#include "LayerTreeInvalidation.h"
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "gfxColor.h"                   
#include "GraphicsFilter.h"             
#include "gfxPoint3D.h"                 
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDataHashtable.h"            
#include "nsDebug.h"                    
#include "nsHashKeys.h"                 
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsTArray.h"                   

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

struct LayerPropertiesBase;
LayerPropertiesBase* CloneLayerTreePropertiesInternal(Layer* aRoot);

static nsIntRect
TransformRect(const nsIntRect& aRect, const Matrix4x4& aTransform)
{
  if (aRect.IsEmpty()) {
    return nsIntRect();
  }

  Rect rect(aRect.x, aRect.y, aRect.width, aRect.height);
  rect = aTransform.TransformBounds(rect);
  rect.RoundOut();

  nsIntRect intRect;
  if (!gfxUtils::GfxRectToIntRect(ThebesRect(rect), &intRect)) {
    return nsIntRect();
  }

  return intRect;
}

static void
AddTransformedRegion(nsIntRegion& aDest, const nsIntRegion& aSource, const Matrix4x4& aTransform)
{
  nsIntRegionRectIterator iter(aSource);
  const nsIntRect *r;
  while ((r = iter.Next())) {
    aDest.Or(aDest, TransformRect(*r, aTransform));
  }
  aDest.SimplifyOutward(20);
}

static void
AddRegion(nsIntRegion& aDest, const nsIntRegion& aSource)
{
  aDest.Or(aDest, aSource);
  aDest.SimplifyOutward(20);
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
    , mInvalidRegion(aLayer->GetInvalidRegion())
    , mPostXScale(aLayer->GetPostXScale())
    , mPostYScale(aLayer->GetPostYScale())
    , mOpacity(aLayer->GetLocalOpacity())
    , mUseClipRect(!!aLayer->GetClipRect())
  {
    MOZ_COUNT_CTOR(LayerPropertiesBase);
    if (aLayer->GetMaskLayer()) {
      mMaskLayer = CloneLayerTreePropertiesInternal(aLayer->GetMaskLayer());
    }
    if (mUseClipRect) {
      mClipRect = *aLayer->GetClipRect();
    }
    mTransform = aLayer->GetTransform();
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
                                         NotifySubDocInvalidationFunc aCallback,
                                         bool* aGeometryChanged);

  virtual void MoveBy(const nsIntPoint& aOffset);

  nsIntRegion ComputeChange(NotifySubDocInvalidationFunc aCallback,
                            bool& aGeometryChanged)
  {
    bool transformChanged = !mTransform.FuzzyEqual(mLayer->GetTransform()) ||
                            mLayer->GetPostXScale() != mPostXScale ||
                            mLayer->GetPostYScale() != mPostYScale;
    Layer* otherMask = mLayer->GetMaskLayer();
    const nsIntRect* otherClip = mLayer->GetClipRect();
    nsIntRegion result;
    if ((mMaskLayer ? mMaskLayer->mLayer : nullptr) != otherMask ||
        (mUseClipRect != !!otherClip) ||
        mLayer->GetLocalOpacity() != mOpacity ||
        transformChanged) 
    {
      aGeometryChanged = true;
      result = OldTransformedBounds();
      AddRegion(result, NewTransformedBounds());

      
      
      
      if (!aCallback) {
        ClearInvalidations(mLayer);
        return result;
      }
    }

    AddRegion(result, ComputeChangeInternal(aCallback, aGeometryChanged));
    AddTransformedRegion(result, mLayer->GetInvalidRegion(), mTransform);

    if (mMaskLayer && otherMask) {
      AddTransformedRegion(result, mMaskLayer->ComputeChange(aCallback, aGeometryChanged),
                           mTransform);
    }

    if (mUseClipRect && otherClip) {
      if (!mClipRect.IsEqualInterior(*otherClip)) {
        aGeometryChanged = true;
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

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback,
                                            bool& aGeometryChanged)
  {
    return nsIntRect();
  }

  nsRefPtr<Layer> mLayer;
  nsAutoPtr<LayerPropertiesBase> mMaskLayer;
  nsIntRegion mVisibleRegion;
  nsIntRegion mInvalidRegion;
  Matrix4x4 mTransform;
  float mPostXScale;
  float mPostYScale;
  float mOpacity;
  nsIntRect mClipRect;
  bool mUseClipRect;
};

struct ContainerLayerProperties : public LayerPropertiesBase
{
  ContainerLayerProperties(ContainerLayer* aLayer)
    : LayerPropertiesBase(aLayer)
    , mPreXScale(aLayer->GetPreXScale())
    , mPreYScale(aLayer->GetPreYScale())
  {
    for (Layer* child = aLayer->GetFirstChild(); child; child = child->GetNextSibling()) {
      mChildren.AppendElement(CloneLayerTreePropertiesInternal(child));
    }
  }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback,
                                            bool& aGeometryChanged)
  {
    ContainerLayer* container = mLayer->AsContainerLayer();
    nsIntRegion result;

    if (mPreXScale != container->GetPreXScale() ||
        mPreYScale != container->GetPreYScale()) {
      aGeometryChanged = true;
      result = OldTransformedBounds();
      AddRegion(result, NewTransformedBounds());

      
      
      
      if (!aCallback) {
        ClearInvalidations(mLayer);
        return result;
      }
    }

    
    
    
    
    
    
    
    

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
            
            AddRegion(result, mChildren[childsOldIndex]->ComputeChange(aCallback, aGeometryChanged));
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
        aGeometryChanged = true;
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

    result.Transform(gfx::To3DMatrix(mLayer->GetTransform()));
    return result;
  }

  
  nsAutoTArray<nsAutoPtr<LayerPropertiesBase>,1> mChildren;
  float mPreXScale;
  float mPreYScale;
};

struct ColorLayerProperties : public LayerPropertiesBase
{
  ColorLayerProperties(ColorLayer *aLayer)
    : LayerPropertiesBase(aLayer)
    , mColor(aLayer->GetColor())
  { }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback,
                                            bool& aGeometryChanged)
  {
    ColorLayer* color = static_cast<ColorLayer*>(mLayer.get());

    if (mColor != color->GetColor()) {
      aGeometryChanged = true;
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
    , mContainer(aImage->GetContainer())
    , mFilter(aImage->GetFilter())
    , mScaleToSize(aImage->GetScaleToSize())
    , mScaleMode(aImage->GetScaleMode())
  {
  }

  virtual nsIntRegion ComputeChangeInternal(NotifySubDocInvalidationFunc aCallback,
                                            bool& aGeometryChanged)
  {
    ImageLayer* imageLayer = static_cast<ImageLayer*>(mLayer.get());
    
    if (!imageLayer->GetVisibleRegion().IsEqual(mVisibleRegion)) {
      aGeometryChanged = true;
      nsIntRect result = NewTransformedBounds();
      result = result.Union(OldTransformedBounds());
      return result;
    }

    if (mContainer != imageLayer->GetContainer() ||
        mFilter != imageLayer->GetFilter() ||
        mScaleToSize != imageLayer->GetScaleToSize() ||
        mScaleMode != imageLayer->GetScaleMode()) {
      aGeometryChanged = true;
      return NewTransformedBounds();
    }

    return nsIntRect();
  }

  nsRefPtr<ImageContainer> mContainer;
  GraphicsFilter mFilter;
  gfx::IntSize mScaleToSize;
  ScaleMode mScaleMode;
};

LayerPropertiesBase*
CloneLayerTreePropertiesInternal(Layer* aRoot)
{
  if (!aRoot) {
    return new LayerPropertiesBase();
  }

  switch (aRoot->GetType()) {
    case Layer::TYPE_CONTAINER:
    case Layer::TYPE_REF:
      return new ContainerLayerProperties(aRoot->AsContainerLayer());
    case Layer::TYPE_COLOR:
      return new ColorLayerProperties(static_cast<ColorLayer*>(aRoot));
    case Layer::TYPE_IMAGE:
      return new ImageLayerProperties(static_cast<ImageLayer*>(aRoot));
    default:
      return new LayerPropertiesBase(aRoot);
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
LayerPropertiesBase::ComputeDifferences(Layer* aRoot, NotifySubDocInvalidationFunc aCallback,
                                        bool* aGeometryChanged = nullptr)
{
  NS_ASSERTION(aRoot, "Must have a layer tree to compare against!");
  if (mLayer != aRoot) {
    if (aCallback) {
      NotifySubdocumentInvalidationRecursive(aRoot, aCallback);
    } else {
      ClearInvalidations(aRoot);
    }
    nsIntRect result = TransformRect(aRoot->GetVisibleRegion().GetBounds(),
                                     aRoot->GetTransform());
    result = result.Union(OldTransformedBounds());
    if (aGeometryChanged != nullptr) {
      *aGeometryChanged = true;
    }
    return result;
  } else {
    bool geometryChanged = (aGeometryChanged != nullptr) ? *aGeometryChanged : false;
    nsIntRegion invalid = ComputeChange(aCallback, geometryChanged);
    if (aGeometryChanged != nullptr) {
      *aGeometryChanged = geometryChanged;
    }
    return invalid;
  }
}

void
LayerPropertiesBase::MoveBy(const nsIntPoint& aOffset)
{
  mTransform.TranslatePost(aOffset.x, aOffset.y, 0);
}

} 
} 
