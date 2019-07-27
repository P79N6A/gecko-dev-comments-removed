





#ifndef mozilla_layers_HitTestingTreeNode_h
#define mozilla_layers_HitTestingTreeNode_h

#include "APZUtils.h"                       
#include "FrameMetrics.h"                   
#include "mozilla/gfx/Matrix.h"             
#include "mozilla/layers/LayersTypes.h"     
#include "nsRefPtr.h"                       

namespace mozilla {
namespace layers {

class AsyncPanZoomController;




























class HitTestingTreeNode {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(HitTestingTreeNode);

private:
  ~HitTestingTreeNode();
public:
  HitTestingTreeNode(AsyncPanZoomController* aApzc, bool aIsPrimaryHolder);
  void Destroy();

  
  void SetLastChild(HitTestingTreeNode* aChild);
  void SetPrevSibling(HitTestingTreeNode* aSibling);
  void MakeRoot();

  

  HitTestingTreeNode* GetFirstChild() const;
  HitTestingTreeNode* GetLastChild() const;
  HitTestingTreeNode* GetPrevSibling() const;
  HitTestingTreeNode* GetParent() const;

  
  AsyncPanZoomController* Apzc() const;
  bool IsPrimaryHolder() const;

  
  void SetHitTestData(const EventRegions& aRegions,
                      const gfx::Matrix4x4& aTransform,
                      const nsIntRegion& aClipRegion);
  HitTestResult HitTest(const ParentLayerPoint& aPoint) const;

  
  void Dump(const char* aPrefix = "") const;

private:
  nsRefPtr<HitTestingTreeNode> mLastChild;
  nsRefPtr<HitTestingTreeNode> mPrevSibling;
  nsRefPtr<HitTestingTreeNode> mParent;

  nsRefPtr<AsyncPanZoomController> mApzc;
  bool mIsPrimaryApzcHolder;

  











  EventRegions mEventRegions;

  



  gfx::Matrix4x4 mTransform;

  



  nsIntRegion mClipRegion;
};

}
}

#endif 
