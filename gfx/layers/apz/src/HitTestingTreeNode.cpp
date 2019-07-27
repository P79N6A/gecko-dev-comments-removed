





#include "HitTestingTreeNode.h"

#include "AsyncPanZoomController.h"                     
#include "LayersLogging.h"                              
#include "mozilla/gfx/Point.h"                          
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

HitTestingTreeNode::~HitTestingTreeNode()
{
}

void
HitTestingTreeNode::Destroy()
{
  AsyncPanZoomController::AssertOnCompositorThread();

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
                                   const nsIntRegion& aClipRegion)
{
  mEventRegions = aRegions;
  mTransform = aTransform;
  mClipRegion = aClipRegion;
}

HitTestResult
HitTestingTreeNode::HitTest(const ParentLayerPoint& aPoint) const
{
  
  
  
  if (!gfxPrefs::LayoutEventRegionsEnabled()) {
    MOZ_ASSERT(mEventRegions == EventRegions());
    MOZ_ASSERT(mTransform == gfx::Matrix4x4());
    return mClipRegion.Contains(aPoint.x, aPoint.y)
        ? HitTestResult::ApzcHitRegion
        : HitTestResult::NoApzcHit;
  }

  
  if (!mClipRegion.Contains(aPoint.x, aPoint.y)) {
    return HitTestResult::NoApzcHit;
  }

  
  gfx::Matrix4x4 localTransform = mTransform;
  if (mApzc) {
    localTransform = localTransform * gfx::Matrix4x4(mApzc->GetCurrentAsyncTransform());
  }
  gfx::Point4D pointInLayerPixels = localTransform.Inverse().ProjectPoint(aPoint.ToUnknownPoint());
  if (!pointInLayerPixels.HasPositiveWCoord()) {
    return HitTestResult::NoApzcHit;
  }
  LayerIntPoint point = RoundedToInt(ViewAs<LayerPixel>(pointInLayerPixels.As2DPoint()));

  
  if (!mEventRegions.mHitRegion.Contains(point.x, point.y)) {
    return HitTestResult::NoApzcHit;
  }
  if (mEventRegions.mDispatchToContentHitRegion.Contains(point.x, point.y)) {
    return HitTestResult::ApzcContentRegion;
  }
  return HitTestResult::ApzcHitRegion;
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
    Stringify(mClipRegion).c_str());
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
