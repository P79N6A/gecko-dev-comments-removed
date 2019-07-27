





#include "HitTestingTreeNode.h"

#include "AsyncPanZoomController.h"                     
#include "LayersLogging.h"                              
#include "mozilla/gfx/Point.h"                          
#include "mozilla/layers/APZThreadUtils.h"              
#include "mozilla/layers/AsyncCompositionManager.h"     
#include "nsPrintfCString.h"                            
#include "UnitTransforms.h"                             

namespace mozilla {
namespace layers {

HitTestingTreeNode::HitTestingTreeNode(AsyncPanZoomController* aApzc,
                                       bool aIsPrimaryHolder)
  : mApzc(aApzc)
  , mIsPrimaryApzcHolder(aIsPrimaryHolder)
{
  if (mIsPrimaryApzcHolder) {
    MOZ_ASSERT(mApzc);
  }
}

void
HitTestingTreeNode::RecycleWith(AsyncPanZoomController* aApzc)
{
  MOZ_ASSERT(!mIsPrimaryApzcHolder);
  Destroy(); 
  mApzc = aApzc;
  
  
}

HitTestingTreeNode::~HitTestingTreeNode()
{
}

void
HitTestingTreeNode::Destroy()
{
  APZThreadUtils::AssertOnCompositorThread();

  mPrevSibling = nullptr;
  mLastChild = nullptr;
  mParent = nullptr;

  if (mApzc) {
    if (mIsPrimaryApzcHolder) {
      mApzc->Destroy();
    }
    mApzc = nullptr;
  }
}

void
HitTestingTreeNode::SetLastChild(HitTestingTreeNode* aChild)
{
  mLastChild = aChild;
  if (aChild) {
    aChild->mParent = this;

    if (aChild->GetApzc()) {
      AsyncPanZoomController* parent = GetNearestContainingApzc();
      
      
      
      
      MOZ_ASSERT(aChild->GetApzc() != parent);
      aChild->SetApzcParent(parent);
    }
  }
}

void
HitTestingTreeNode::SetPrevSibling(HitTestingTreeNode* aSibling)
{
  mPrevSibling = aSibling;
  if (aSibling) {
    aSibling->mParent = mParent;

    if (aSibling->GetApzc()) {
      AsyncPanZoomController* parent = mParent ? mParent->GetNearestContainingApzc() : nullptr;
      aSibling->SetApzcParent(parent);
    }
  }
}

void
HitTestingTreeNode::MakeRoot()
{
  mParent = nullptr;

  if (GetApzc()) {
    SetApzcParent(nullptr);
  }
}

HitTestingTreeNode*
HitTestingTreeNode::GetFirstChild() const
{
  HitTestingTreeNode* child = GetLastChild();
  while (child && child->GetPrevSibling()) {
    child = child->GetPrevSibling();
  }
  return child;
}

HitTestingTreeNode*
HitTestingTreeNode::GetLastChild() const
{
  return mLastChild;
}

HitTestingTreeNode*
HitTestingTreeNode::GetPrevSibling() const
{
  return mPrevSibling;
}

HitTestingTreeNode*
HitTestingTreeNode::GetParent() const
{
  return mParent;
}

AsyncPanZoomController*
HitTestingTreeNode::GetApzc() const
{
  return mApzc;
}

AsyncPanZoomController*
HitTestingTreeNode::GetNearestContainingApzc() const
{
  for (const HitTestingTreeNode* n = this; n; n = n->GetParent()) {
    if (n->GetApzc()) {
      return n->GetApzc();
    }
  }
  return nullptr;
}

bool
HitTestingTreeNode::IsPrimaryHolder() const
{
  return mIsPrimaryApzcHolder;
}

void
HitTestingTreeNode::SetHitTestData(const EventRegions& aRegions,
                                   const gfx::Matrix4x4& aTransform,
                                   const Maybe<nsIntRegion>& aClipRegion)
{
  mEventRegions = aRegions;
  mTransform = aTransform;
  mClipRegion = aClipRegion;
}

bool
HitTestingTreeNode::IsOutsideClip(const ParentLayerPoint& aPoint) const
{
  
  return (mClipRegion.isSome() && !mClipRegion->Contains(aPoint.x, aPoint.y));
}

Maybe<LayerPoint>
HitTestingTreeNode::Untransform(const ParentLayerPoint& aPoint) const
{
  
  gfx::Matrix4x4 localTransform = mTransform;
  if (mApzc) {
    localTransform = localTransform * mApzc->GetCurrentAsyncTransformWithOverscroll();
  }
  gfx::Point4D point = localTransform.Inverse().ProjectPoint(aPoint.ToUnknownPoint());
  return point.HasPositiveWCoord()
        ? Some(ViewAs<LayerPixel>(point.As2DPoint()))
        : Nothing();
}

HitTestResult
HitTestingTreeNode::HitTest(const ParentLayerPoint& aPoint) const
{
  
  
  MOZ_ASSERT(!IsOutsideClip(aPoint));

  
  
  
  
  
  if (!gfxPrefs::LayoutEventRegionsEnabled() && GetApzc()) {
    return HitTestResult::HitLayer;
  }

  
  Maybe<LayerPoint> pointInLayerPixels = Untransform(aPoint);
  if (!pointInLayerPixels) {
    return HitTestResult::HitNothing;
  }
  LayerIntPoint point = RoundedToInt(pointInLayerPixels.ref());

  
  if (!mEventRegions.mHitRegion.Contains(point.x, point.y)) {
    return HitTestResult::HitNothing;
  }
  if (mEventRegions.mDispatchToContentHitRegion.Contains(point.x, point.y)) {
    return HitTestResult::HitDispatchToContentRegion;
  }
  return HitTestResult::HitLayer;
}

void
HitTestingTreeNode::Dump(const char* aPrefix) const
{
  if (mPrevSibling) {
    mPrevSibling->Dump(aPrefix);
  }
  printf_stderr("%sHitTestingTreeNode (%p) APZC (%p) g=(%s) r=(%s) t=(%s) c=(%s)\n",
    aPrefix, this, mApzc.get(), mApzc ? Stringify(mApzc->GetGuid()).c_str() : "",
    Stringify(mEventRegions).c_str(), Stringify(mTransform).c_str(),
    mClipRegion ? Stringify(mClipRegion.ref()).c_str() : "none");
  if (mLastChild) {
    mLastChild->Dump(nsPrintfCString("%s  ", aPrefix).get());
  }
}

void
HitTestingTreeNode::SetApzcParent(AsyncPanZoomController* aParent)
{
  
  MOZ_ASSERT(GetApzc() != nullptr);
  if (IsPrimaryHolder()) {
    GetApzc()->SetParent(aParent);
  } else {
    MOZ_ASSERT(GetApzc()->GetParent() == aParent);
  }
}

} 
} 
