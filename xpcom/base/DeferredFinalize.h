





#ifndef mozilla_DeferredFinalize_h
#define mozilla_DeferredFinalize_h

class nsISupports;

namespace mozilla {




typedef void* (*DeferredFinalizeAppendFunction)(void* aPointers, void* aThing);





typedef bool (*DeferredFinalizeFunction)(uint32_t aSlice, void* aData);

void DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                      DeferredFinalizeFunction aFunc,
                      void* aThing);

void DeferredFinalize(nsISupports* aSupports);

} 

#endif 
