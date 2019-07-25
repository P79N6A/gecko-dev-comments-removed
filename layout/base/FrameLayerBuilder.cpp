




































#include "FrameLayerBuilder.h"

#include "nsDisplayList.h"
#include "nsPresContext.h"
#include "nsLayoutUtils.h"

using namespace mozilla::layers;

namespace mozilla {

namespace {







class ClippedItemIterator {
public:
  ClippedItemIterator(const nsDisplayList* aList)
  {
    DescendIntoList(aList, nsnull, nsnull);
    AdvanceToItem();
  }
  PRBool IsDone()
  {
    return mStack.IsEmpty();
  }
  void Next()
  {
    State* top = StackTop();
    top->mItem = top->mItem->GetAbove();
    AdvanceToItem();
  }
  
  
  const gfxRect* GetEffectiveClipRect()
  {
    State* top = StackTop();
    return top->mHasClipRect ? &top->mEffectiveClipRect : nsnull;
  }
  nsDisplayItem* Item()
  {
    return StackTop()->mItem;
  }

private:
  
  
  struct State {
    
    nsDisplayItem* mItem;
    
    gfxRect mEffectiveClipRect;
    PRPackedBool mHasClipRect;
  };

  State* StackTop()
  {
    return &mStack[mStack.Length() - 1];
  }
  void DescendIntoList(const nsDisplayList* aList,
                       nsPresContext* aPresContext,
                       const nsRect* aClipRect)
  {
    State* state = mStack.AppendElement();
    if (!state)
      return;
    if (mStack.Length() >= 2) {
      *state = mStack[mStack.Length() - 2];
    } else {
      state->mHasClipRect = PR_FALSE;
    }
    state->mItem = aList->GetBottom();
    if (aClipRect) {
      gfxRect r(aClipRect->x, aClipRect->y, aClipRect->width, aClipRect->height);
      r.ScaleInverse(aPresContext->AppUnitsPerDevPixel());
      if (state->mHasClipRect) {
        state->mEffectiveClipRect = state->mEffectiveClipRect.Intersect(r);
      } else {
        state->mEffectiveClipRect = r;
        state->mHasClipRect = PR_TRUE;
      }
    }
  }
  
  void AdvanceToItem()
  {
    while (!mStack.IsEmpty()) {
      State* top = StackTop();
      if (!top->mItem) {
        mStack.SetLength(mStack.Length() - 1);
        if (!mStack.IsEmpty()) {
          top = StackTop();
          top->mItem = top->mItem->GetAbove();
        }
        continue;
      }
      if (top->mItem->GetType() != nsDisplayItem::TYPE_CLIP)
        return;
      nsDisplayClip* clipItem = static_cast<nsDisplayClip*>(top->mItem);
      nsRect clip = clipItem->GetClipRect();
      DescendIntoList(clipItem->GetList(),
                      clipItem->GetClippingFrame()->PresContext(),
                      &clip);
    }
  }

  nsAutoTArray<State,10> mStack;
};














struct ItemGroup {
  
  nsDisplayItem* mStartItem;
  nsDisplayItem* mEndItem;
  ItemGroup* mNextItemsForLayer;
  
  gfxRect mClipRect;
  PRPackedBool mHasClipRect;

  ItemGroup() : mStartItem(nsnull), mEndItem(nsnull),
    mNextItemsForLayer(nsnull), mHasClipRect(PR_FALSE) {}

  void* operator new(size_t aSize,
                     nsDisplayListBuilder* aBuilder) CPP_THROW_NEW {
    return aBuilder->Allocate(aSize);
  }
};





struct LayerItems {
  nsRefPtr<Layer> mLayer;
  
  ThebesLayer* mThebesLayer;
  ItemGroup* mItems;
  
  nsIntRect mVisibleRect;

  LayerItems(ItemGroup* aItems) :
    mThebesLayer(nsnull), mItems(aItems)
  {
  }

  void* operator new(size_t aSize,
                     nsDisplayListBuilder* aBuilder) CPP_THROW_NEW {
    return aBuilder->Allocate(aSize);
  }
};









static ItemGroup*
AddToItemGroup(nsDisplayListBuilder* aBuilder,
               ItemGroup* aGroup, nsDisplayItem* aItem,
               const gfxRect* aClipRect)
{
  NS_ASSERTION(!aGroup->mNextItemsForLayer,
               "aGroup must be the last group in the chain");

  if (!aGroup->mStartItem) {
    aGroup->mStartItem = aItem;
    aGroup->mEndItem = aItem->GetAbove();
    aGroup->mHasClipRect = aClipRect != nsnull;
    if (aClipRect) {
      aGroup->mClipRect = *aClipRect;
    }
    return aGroup;
  }

  if (aGroup->mEndItem == aItem &&
      (aGroup->mHasClipRect
       ? (aClipRect && aGroup->mClipRect == *aClipRect)
       : !aClipRect))  {
    aGroup->mEndItem = aItem->GetAbove();
    return aGroup;
  }

  ItemGroup* itemGroup = new (aBuilder) ItemGroup();
  if (!itemGroup)
    return aGroup;
  aGroup->mNextItemsForLayer = itemGroup;
  return AddToItemGroup(aBuilder, itemGroup, aItem, aClipRect);
}





static ItemGroup*
CreateEmptyThebesLayer(nsDisplayListBuilder* aBuilder,
                       LayerManager* aManager,
                       nsTArray<LayerItems*>* aLayers)
{
  ItemGroup* itemGroup = new (aBuilder) ItemGroup();
  if (!itemGroup)
    return nsnull;
  nsRefPtr<ThebesLayer> thebesLayer = aManager->CreateThebesLayer();
  if (!thebesLayer)
    return nsnull;
  LayerItems* layerItems = new (aBuilder) LayerItems(itemGroup);
  aLayers->AppendElement(layerItems);
  thebesLayer->SetUserData(layerItems);
  layerItems->mThebesLayer = thebesLayer;
  layerItems->mLayer = thebesLayer.forget();
  return itemGroup;
}

static PRBool
IsAllUniform(nsDisplayListBuilder* aBuilder, ItemGroup* aGroup,
             nscolor* aColor)
{
  nsRect visibleRect = aGroup->mStartItem->GetVisibleRect();
  nscolor finalColor = NS_RGBA(0,0,0,0);
  for (ItemGroup* group = aGroup; group;
       group = group->mNextItemsForLayer) {
    for (nsDisplayItem* item = group->mStartItem; item != group->mEndItem;
         item = item->GetAbove()) {
      nscolor color;
      if (visibleRect != item->GetVisibleRect())
        return PR_FALSE;
      if (!item->IsUniform(aBuilder, &color))
        return PR_FALSE;
      finalColor = NS_ComposeColors(finalColor, color);
    }
  }
  *aColor = finalColor;
  return PR_TRUE;
}










static void BuildLayers(nsDisplayListBuilder* aBuilder,
                        const nsDisplayList& aList,
                        LayerManager* aManager,
                        nsTArray<LayerItems*>* aLayers)
{
  NS_ASSERTION(aLayers->IsEmpty(), "aLayers must be initially empty");

  
  
  
  
  
  
  ItemGroup* firstThebesLayerItems =
    CreateEmptyThebesLayer(aBuilder, aManager, aLayers);
  if (!firstThebesLayerItems)
    return;
  
  
  
  ItemGroup* lastThebesLayerItems = firstThebesLayerItems;
  
  
  nsRegion areaAboveFirstThebesLayer;

  for (ClippedItemIterator iter(&aList); !iter.IsDone(); iter.Next()) {
    nsDisplayItem* item = iter.Item();
    const gfxRect* clipRect = iter.GetEffectiveClipRect();
    
    nsRefPtr<Layer> layer = item->BuildLayer(aBuilder, aManager);
    nsRect bounds = item->GetBounds(aBuilder);
    
    
    LayerItems* layerItems = nsnull;
    if (layer) {
      
      
      ItemGroup* itemGroup = new (aBuilder) ItemGroup();
      if (itemGroup) {
        AddToItemGroup(aBuilder, itemGroup, item, clipRect);
        layerItems = new (aBuilder) LayerItems(itemGroup);
        aLayers->AppendElement(layerItems);
        if (layerItems) {
          if (itemGroup->mHasClipRect) {
            gfxRect r = itemGroup->mClipRect;
            r.Round();
            nsIntRect intRect(r.X(), r.Y(), r.Width(), r.Height());
            layer->IntersectClipRect(intRect);
          }
          layerItems->mLayer = layer.forget();
        }
      }
      
      areaAboveFirstThebesLayer.Or(areaAboveFirstThebesLayer, bounds);
      lastThebesLayerItems = nsnull;
    } else {
      
      
      
      
      if (!areaAboveFirstThebesLayer.Intersects(bounds)) {
        firstThebesLayerItems =
          AddToItemGroup(aBuilder, firstThebesLayerItems, item, clipRect);
        layerItems = aLayers->ElementAt(0);
      } else if (lastThebesLayerItems) {
        
        lastThebesLayerItems =
          AddToItemGroup(aBuilder, lastThebesLayerItems, item, clipRect);
        
        areaAboveFirstThebesLayer.Or(areaAboveFirstThebesLayer, bounds);
        layerItems = aLayers->ElementAt(aLayers->Length() - 1);
      } else {
        
        ItemGroup* itemGroup =
          CreateEmptyThebesLayer(aBuilder, aManager, aLayers);
        if (itemGroup) {
          lastThebesLayerItems =
            AddToItemGroup(aBuilder, itemGroup, item, clipRect);
          NS_ASSERTION(lastThebesLayerItems == itemGroup,
                       "AddToItemGroup shouldn't create a new group if the "
                       "initial group is empty");
          
          areaAboveFirstThebesLayer.Or(areaAboveFirstThebesLayer, bounds);
          layerItems = aLayers->ElementAt(aLayers->Length() - 1);
        }
      }
    }

    if (layerItems) {
      
      
      nscoord appUnitsPerDevPixel =
        item->GetUnderlyingFrame()->PresContext()->AppUnitsPerDevPixel();
      layerItems->mVisibleRect.UnionRect(layerItems->mVisibleRect,
        item->GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel));
    }
  }

  if (!firstThebesLayerItems->mStartItem) {
    
    
    aLayers->ElementAt(0)->mLayer = nsnull;
    aLayers->RemoveElementAt(0);
  }

  for (PRUint32 i = 0; i < aLayers->Length(); ++i) {
    LayerItems* layerItems = aLayers->ElementAt(i);

    nscolor color;
    
    
    if (layerItems->mThebesLayer &&
        IsAllUniform(aBuilder, layerItems->mItems, &color) &&
        layerItems->mLayer->GetTransform().IsIdentity()) {
      nsRefPtr<ColorLayer> layer = aManager->CreateColorLayer();
      layer->SetClipRect(layerItems->mThebesLayer->GetClipRect());
      
      layer->IntersectClipRect(layerItems->mVisibleRect);
      layer->SetColor(gfxRGBA(color));
      layerItems->mLayer = layer.forget();
      layerItems->mThebesLayer = nsnull;
    }

    gfxMatrix transform;
    nsIntRect visibleRect = layerItems->mVisibleRect;
    if (layerItems->mLayer->GetTransform().Is2D(&transform)) {
      
      
      transform.Invert();
      gfxRect layerVisible = transform.TransformBounds(
          gfxRect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height));
      layerVisible.RoundOut();
      if (NS_FAILED(nsLayoutUtils::GfxRectToIntRect(layerVisible, &visibleRect))) {
        NS_ERROR("Visible rect transformed out of bounds");
      }
    } else {
      NS_ERROR("Only 2D transformations currently supported");
    }
    layerItems->mLayer->SetVisibleRegion(nsIntRegion(visibleRect));
  }
}

} 

already_AddRefed<Layer>
FrameLayerBuilder::GetContainerLayerFor(nsDisplayListBuilder* aBuilder,
                                        LayerManager* aManager,
                                        nsDisplayItem* aContainer,
                                        const nsDisplayList& aChildren)
{
  
  
  
  
  nsRefPtr<ContainerLayer> container = aManager->CreateContainerLayer();
  if (!container)
    return nsnull;

  nsAutoTArray<LayerItems*,10> layerItems;
  BuildLayers(aBuilder, aChildren, aManager, &layerItems);

  Layer* lastChild = nsnull;
  for (PRUint32 i = 0; i < layerItems.Length(); ++i) {
    Layer* child = layerItems[i]->mLayer;
    container->InsertAfter(child, lastChild);
    lastChild = child;
    
    
    layerItems[i]->mLayer = nsnull;
  }
  container->SetIsOpaqueContent(aChildren.IsOpaque());
  nsRefPtr<Layer> layer = container.forget();
  return layer.forget();
}

Layer*
FrameLayerBuilder::GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   nsDisplayItem* aItem)
{
  
  return nsnull;
}

 void
FrameLayerBuilder::InvalidateThebesLayerContents(nsIFrame* aFrame,
                                                 const nsRect& aRect)
{
  
}

 void
FrameLayerBuilder::DrawThebesLayer(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw,
                                   const nsIntRegion& aRegionToInvalidate,
                                   void* aCallbackData)
{
  
  

  LayerItems* layerItems = static_cast<LayerItems*>(aLayer->GetUserData());
  nsDisplayListBuilder* builder =
    static_cast<nsDisplayListBuilder*>(aCallbackData);

  
  
  
  
  

  
  
  
  
  
  nsRefPtr<nsIRenderingContext> rc;
  nsPresContext* lastPresContext = nsnull;
  gfxRect currentClip;
  PRBool setClipRect = PR_FALSE;
  NS_ASSERTION(layerItems->mItems, "No empty layers allowed");
  for (ItemGroup* group = layerItems->mItems; group;
       group = group->mNextItemsForLayer) {
    
    
    if (setClipRect != group->mHasClipRect ||
        (group->mHasClipRect && group->mClipRect != currentClip)) {
      if (setClipRect) {
        aContext->Restore();
      }
      setClipRect = group->mHasClipRect;
      if (setClipRect) {
        aContext->Save();
        aContext->NewPath();
        aContext->Rectangle(group->mClipRect, PR_TRUE);
        aContext->Clip();
        currentClip = group->mClipRect;
      }
    }
    NS_ASSERTION(group->mStartItem, "No empty groups allowed");
    for (nsDisplayItem* item = group->mStartItem; item != group->mEndItem;
         item = item->GetAbove()) {
      nsPresContext* presContext = item->GetUnderlyingFrame()->PresContext();
      if (presContext != lastPresContext) {
        
        
        nsresult rv =
          presContext->DeviceContext()->CreateRenderingContextInstance(*getter_AddRefs(rc));
        if (NS_FAILED(rv))
          break;
        rc->Init(presContext->DeviceContext(), aContext);
        lastPresContext = presContext;
      }
      item->Paint(builder, rc);
    }
  }
  if (setClipRect) {
    aContext->Restore();
  }
}

} 
