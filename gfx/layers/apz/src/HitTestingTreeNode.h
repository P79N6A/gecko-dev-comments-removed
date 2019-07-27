





#ifndef mozilla_layers_HitTestingTreeNode_h
#define mozilla_layers_HitTestingTreeNode_h

#include "APZUtils.h"                       
#include "FrameMetrics.h"                   
#include "mozilla/gfx/Matrix.h"             
#include "mozilla/layers/LayersTypes.h"     
#include "mozilla/Maybe.h"                  
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
  void RecycleWith(AsyncPanZoomController* aApzc);
  void Destroy();

  

  void SetLastChild(HitTestingTreeNode* aChild);
  void SetPrevSibling(HitTestingTreeNode* aSibling);
  void MakeRoot();

  


  HitTestingTreeNode* GetFirstChild() const;
  HitTestingTreeNode* GetLastChild() const;
  HitTestingTreeNode* GetPrevSibling() const;
  HitTestingTreeNode* GetParent() const;

  

  AsyncPanZoomController* GetApzc() const;
  AsyncPanZoomController* GetNearestContainingApzc() const;
  bool IsPrimaryHolder() const;

  

  void SetHitTestData(const EventRegions& aRegions,
                      const gfx::Matrix4x4& aTransform,
                      const Maybe<nsIntRegion>& aClipRegion,
                      bool aForceDispatchToContent);
  bool IsOutsideClip(const ParentLayerPoint& aPoint) const;
  

  Maybe<LayerPoint> Untransform(const ParentLayerPoint& aPoint) const;
  

  HitTestResult HitTest(const ParentLayerPoint& aPoint) const;
  
  bool GetForceDispatchToContent() const;

  
  void Dump(const char* aPrefix = "") const;

private:
  void SetApzcParent(AsyncPanZoomController* aApzc);

  nsRefPtr<HitTestingTreeNode> mLastChild;
  nsRefPtr<HitTestingTreeNode> mPrevSibling;
  nsRefPtr<HitTestingTreeNode> mParent;

  nsRefPtr<AsyncPanZoomController> mApzc;
  bool mIsPrimaryApzcHolder;

  






  EventRegions mEventRegions;

  

  gfx::Matrix4x4 mTransform;

  




  Maybe<nsIntRegion> mClipRegion;

  


  bool mForceDispatchToContent;
};

}
}

#endif 
