





#ifndef mozilla_dom_file_filerequest_h__
#define mozilla_dom_file_filerequest_h__

#include "FileCommon.h"

#include "nsIDOMFileRequest.h"

#include "DOMRequest.h"

BEGIN_FILE_NAMESPACE

class FileHelper;
class LockedFile;

class FileRequest : public mozilla::dom::DOMRequest,
                    public nsIDOMFileRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMFILEREQUEST
  NS_FORWARD_NSIDOMDOMREQUEST(DOMRequest::)
  NS_FORWARD_NSIDOMEVENTTARGET_NOPREHANDLEEVENT(DOMRequest::)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileRequest, DOMRequest)

  static already_AddRefed<FileRequest>
  Create(nsIDOMWindow* aOwner, LockedFile* aLockedFile);

  
  virtual nsresult
  PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  void
  OnProgress(PRUint64 aProgress, PRUint64 aProgressMax)
  {
    FireProgressEvent(aProgress, aProgressMax);
  }

  nsresult
  NotifyHelperCompleted(FileHelper* aFileHelper);

private:
  FileRequest(nsIDOMWindow* aWindow);
  ~FileRequest();

  void
  FireProgressEvent(PRUint64 aLoaded, PRUint64 aTotal);

  virtual void
  RootResultVal();

  nsRefPtr<LockedFile> mLockedFile;

  NS_DECL_EVENT_HANDLER(progress)
};

END_FILE_NAMESPACE

#endif
