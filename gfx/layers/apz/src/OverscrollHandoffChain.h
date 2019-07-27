





#ifndef mozilla_layers_OverscrollHandoffChain_h
#define mozilla_layers_OverscrollHandoffChain_h

#include <vector>
#include "nsAutoPtr.h"
#include "nsISupportsImpl.h"  

namespace mozilla {
namespace layers {

class AsyncPanZoomController;











#define NS_INLINE_DECL_THREADSAFE_MUTABLE_REFCOUNTING(_class)                 \
public:                                                                       \
  NS_METHOD_(MozExternalRefCountType) AddRef(void) const {                    \
    MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    nsrefcnt count = ++mRefCnt;                                               \
    NS_LOG_ADDREF(const_cast<_class*>(this), count, #_class, sizeof(*this));  \
    return (nsrefcnt) count;                                                  \
  }                                                                           \
  NS_METHOD_(MozExternalRefCountType) Release(void) const {                   \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    nsrefcnt count = --mRefCnt;                                               \
    NS_LOG_RELEASE(const_cast<_class*>(this), count, #_class);                \
    if (count == 0) {                                                         \
      delete (this);                                                          \
      return 0;                                                               \
    }                                                                         \
    return count;                                                             \
  }                                                                           \
protected:                                                                    \
  mutable ::mozilla::ThreadSafeAutoRefCnt mRefCnt;                            \
public:








class OverscrollHandoffChain
{
protected:
  
  ~OverscrollHandoffChain();
public:
  
  
  
  
  
  NS_INLINE_DECL_THREADSAFE_MUTABLE_REFCOUNTING(OverscrollHandoffChain)

  



  void Add(AsyncPanZoomController* aApzc);
  void SortByScrollPriority();

  


  uint32_t Length() const { return mChain.size(); }
  const nsRefPtr<AsyncPanZoomController>& GetApzcAtIndex(uint32_t aIndex) const;

  



  
  void FlushRepaints() const;

  
  void CancelAnimations() const;

  
  void SnapBackOverscrolledApzc() const;

  
  
  bool CanBePanned(const AsyncPanZoomController* aApzc) const;
private:
  std::vector<nsRefPtr<AsyncPanZoomController>> mChain;
};


#undef NS_INLINE_DECL_THREADSAFE_MUTABLE_REFCOUNTING

}
}

#endif 
