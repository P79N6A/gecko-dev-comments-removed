





#ifndef mozilla_dom_SourceBufferList_h_
#define mozilla_dom_SourceBufferList_h_

#include "SourceBuffer.h"
#include "js/RootingAPI.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsCycleCollectionNoteChild.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupports.h"
#include "nsTArray.h"

struct JSContext;
class JSObject;

namespace mozilla {

template <typename T> class AsyncEventRunner;

namespace dom {

class MediaSource;

class SourceBufferList final : public DOMEventTargetHelper
{
public:
  
  SourceBuffer* IndexedGetter(uint32_t aIndex, bool& aFound);

  uint32_t Length();
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SourceBufferList,
                                           DOMEventTargetHelper)

  explicit SourceBufferList(MediaSource* aMediaSource);

  MediaSource* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  void Append(SourceBuffer* aSourceBuffer);

  
  void Remove(SourceBuffer* aSourceBuffer);

  
  bool Contains(SourceBuffer* aSourceBuffer);

  
  void Clear();

  
  bool IsEmpty();

  
  bool AnyUpdating();

  
  void RangeRemoval(double aStart, double aEnd);

  
  void Ended();

  
  void Evict(double aStart, double aEnd);

  
  double GetHighestBufferedEndTime();

  
  void AppendSimple(SourceBuffer* aSourceBuffer);

  
  
  void ClearSimple();

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  ~SourceBufferList();

  friend class AsyncEventRunner<SourceBufferList>;
  void DispatchSimpleEvent(const char* aName);
  void QueueAsyncSimpleEvent(const char* aName);

  nsRefPtr<MediaSource> mMediaSource;
  nsTArray<nsRefPtr<SourceBuffer> > mSourceBuffers;
};

} 

} 
#endif 
