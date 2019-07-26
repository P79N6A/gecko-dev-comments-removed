





#include "SpeechRecognitionResultList.h"

#include "nsContentUtils.h"

#include "mozilla/dom/SpeechRecognitionResultListBinding.h"

#include "SpeechRecognition.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(SpeechRecognitionResultList, mParent, mItems)
NS_IMPL_CYCLE_COLLECTING_ADDREF(SpeechRecognitionResultList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SpeechRecognitionResultList)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SpeechRecognitionResultList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

SpeechRecognitionResultList::SpeechRecognitionResultList(SpeechRecognition* aParent)
  : mParent(aParent)
{
  SetIsDOMBinding();
}

SpeechRecognitionResultList::~SpeechRecognitionResultList()
{
}

nsISupports*
SpeechRecognitionResultList::GetParentObject() const
{
  return static_cast<nsDOMEventTargetHelper*>(mParent.get());
}

JSObject*
SpeechRecognitionResultList::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return SpeechRecognitionResultListBinding::Wrap(aCx, aScope, this);
}

already_AddRefed<SpeechRecognitionResult>
SpeechRecognitionResultList::IndexedGetter(uint32_t aIndex, bool& aPresent)
{
  if (aIndex >= Length()) {
    aPresent = false;
    return nullptr;
  }

  aPresent = true;
  return Item(aIndex);
}

uint32_t
SpeechRecognitionResultList::Length() const
{
  return mItems.Length();
}

already_AddRefed<SpeechRecognitionResult>
SpeechRecognitionResultList::Item(uint32_t aIndex)
{
  nsRefPtr<SpeechRecognitionResult> result = mItems.ElementAt(aIndex);
  return result.forget();
}
} 
} 
