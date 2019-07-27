





#ifndef mozilla_dom_StructuredCloneUtils_h
#define mozilla_dom_StructuredCloneUtils_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "mozilla/dom/File.h"

#include "js/StructuredClone.h"

namespace mozilla {
namespace dom {

struct
StructuredCloneClosure
{
  nsTArray<nsRefPtr<File>> mBlobs;
};

struct
StructuredCloneData
{
  StructuredCloneData() : mData(nullptr), mDataLength(0) {}
  uint64_t* mData;
  size_t mDataLength;
  StructuredCloneClosure mClosure;
};

bool
ReadStructuredClone(JSContext* aCx, uint64_t* aData, size_t aDataLength,
                    const StructuredCloneClosure& aClosure,
                    JS::MutableHandle<JS::Value> aClone);

inline bool
ReadStructuredClone(JSContext* aCx, const StructuredCloneData& aData,
                    JS::MutableHandle<JS::Value> aClone)
{
  return ReadStructuredClone(aCx, aData.mData, aData.mDataLength,
                             aData.mClosure, aClone);
}

bool
WriteStructuredClone(JSContext* aCx, JS::Handle<JS::Value> aSource,
                     JSAutoStructuredCloneBuffer& aBuffer,
                     StructuredCloneClosure& aClosure);

} 
} 

#endif 
