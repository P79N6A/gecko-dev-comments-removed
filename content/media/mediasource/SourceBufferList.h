





#ifndef mozilla_dom_SourceBufferList_h_
#define mozilla_dom_SourceBufferList_h_

#include "AsyncEventRunner.h"
#include "MediaSource.h"
#include "SourceBuffer.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsWrapperCache.h"
#include "nscore.h"

namespace mozilla {

class ErrorResult;
template <typename T> class AsyncEventRunner;

namespace dom {

class MediaSource;

class SourceBufferList MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  
  SourceBuffer* IndexedGetter(uint32_t aIndex, bool& aFound);

  uint32_t Length();
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SourceBufferList, nsDOMEventTargetHelper)

  explicit SourceBufferList(MediaSource* aMediaSource);

  MediaSource* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  void Append(SourceBuffer* aSourceBuffer);

  
  void Remove(SourceBuffer* aSourceBuffer);

  
  bool Contains(SourceBuffer* aSourceBuffer);

  
  void Clear();

  
  bool IsEmpty();

  
  void DetachAndClear();

  
  bool AnyUpdating();

  
  
  void Remove(double aStart, double aEnd, ErrorResult& aRv);

private:
  friend class AsyncEventRunner<SourceBufferList>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  nsRefPtr<MediaSource> mMediaSource;
  nsTArray<nsRefPtr<SourceBuffer> > mSourceBuffers;
};

} 
} 
#endif 
