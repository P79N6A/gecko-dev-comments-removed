





#ifndef mozilla_dom_indexeddb_idbfilerequest_h__
#define mozilla_dom_indexeddb_idbfilerequest_h__

#include "DOMRequest.h"
#include "js/TypeDecls.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/FileRequest.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"

class nsPIDOMWindow;

namespace mozilla {

class EventChainPreVisitor;

namespace dom {
namespace indexedDB {

class IDBFileHandle;

class IDBFileRequest MOZ_FINAL : public DOMRequest,
                                 public FileRequestBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBFileRequest, DOMRequest)

  static already_AddRefed<IDBFileRequest>
  Create(nsPIDOMWindow* aOwner, IDBFileHandle* aFileHandle,
         bool aWrapAsDOMRequest);

  
  virtual nsresult
  PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

  
  virtual void
  OnProgress(uint64_t aProgress, uint64_t aProgressMax) MOZ_OVERRIDE;

  virtual nsresult
  NotifyHelperCompleted(FileHelper* aFileHelper) MOZ_OVERRIDE;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  IDBFileHandle*
  GetFileHandle() const;

  IDBFileHandle*
  GetLockedFile() const
  {
    return GetFileHandle();
  }

  IMPL_EVENT_HANDLER(progress)

private:
  explicit IDBFileRequest(nsPIDOMWindow* aWindow);
  ~IDBFileRequest();

  void
  FireProgressEvent(uint64_t aLoaded, uint64_t aTotal);

  nsRefPtr<IDBFileHandle> mFileHandle;

  bool mWrapAsDOMRequest;
};

} 
} 
} 

#endif 
