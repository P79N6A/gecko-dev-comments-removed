





#ifndef mozilla_dom_file_filerequest_h__
#define mozilla_dom_file_filerequest_h__

#include "mozilla/Attributes.h"
#include "FileCommon.h"

#include "DOMRequest.h"

class nsIDOMLockedFile;

namespace mozilla {
class EventChainPreVisitor;
} 

BEGIN_FILE_NAMESPACE

class FileHelper;
class LockedFile;

class FileRequest : public mozilla::dom::DOMRequest
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
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
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

END_FILE_NAMESPACE

#endif 
