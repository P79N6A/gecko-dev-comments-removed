





#ifndef mozilla_dom_FileRequest_h
#define mozilla_dom_FileRequest_h

#include "DOMRequest.h"
#include "js/TypeDecls.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"

class nsPIDOMWindow;

namespace mozilla {

class EventChainPreVisitor;

namespace dom {

class FileHelper;
class LockedFile;

class FileRequest : public DOMRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileRequest, DOMRequest)

  static already_AddRefed<FileRequest>
  Create(nsPIDOMWindow* aOwner, LockedFile* aLockedFile,
         bool aWrapAsDOMRequest);

  
  virtual nsresult
  PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

  void
  OnProgress(uint64_t aProgress, uint64_t aProgressMax)
  {
    FireProgressEvent(aProgress, aProgressMax);
  }

  nsresult
  NotifyHelperCompleted(FileHelper* aFileHelper);

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  LockedFile*
  GetLockedFile() const;

  IMPL_EVENT_HANDLER(progress)

protected:
  FileRequest(nsPIDOMWindow* aWindow);
  ~FileRequest();

  void
  FireProgressEvent(uint64_t aLoaded, uint64_t aTotal);

  nsRefPtr<LockedFile> mLockedFile;

  bool mWrapAsDOMRequest;
};

} 
} 

#endif 
