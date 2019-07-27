






#ifndef mozilla_CompositorLRU_h
#define mozilla_CompositorLRU_h

#include "mozilla/StaticPtr.h"

#include "nsISupportsImpl.h"
#include "nsTArray.h"

#include <utility>

namespace mozilla {
namespace layers {

class PCompositorParent;

class CompositorLRU final
{
  typedef std::pair<PCompositorParent*, uint64_t> CompositorLayerPair;
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorLRU)

  static void Init();
  static CompositorLRU* GetSingleton();

  




  void Add(PCompositorParent* aCompositor, const uint64_t& id);

  


  void Remove(PCompositorParent* aCompositor, const uint64_t& id);

  


  void Remove(PCompositorParent* aCompositor);

private:
  static StaticRefPtr<CompositorLRU> sSingleton;

  CompositorLRU();
  ~CompositorLRU();
  uint32_t mLRUSize;
  nsTArray<CompositorLayerPair> mLRU;
};

} 
} 

#endif 
