





#ifndef mozilla_dom_file_domarchiverequest_h__
#define mozilla_dom_file_domarchiverequest_h__

#include "nsIDOMArchiveRequest.h"
#include "ArchiveReader.h"
#include "DOMRequest.h"

#include "FileCommon.h"


BEGIN_FILE_NAMESPACE

class ArchiveRequest : public mozilla::dom::DOMRequest,
                       public nsIDOMArchiveRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMARCHIVEREQUEST

  NS_FORWARD_NSIDOMDOMREQUEST(DOMRequest::)
  NS_FORWARD_NSIDOMEVENTTARGET_NOPREHANDLEEVENT(DOMRequest::)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ArchiveRequest, DOMRequest)

  ArchiveRequest(nsIDOMWindow* aWindow,
                 ArchiveReader* aReader);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

public:
  
  void Run();

  
  void OpGetFilenames();
  void OpGetFile(const nsAString& aFilename);

  nsresult ReaderReady(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
                       nsresult aStatus);

public: 
  static already_AddRefed<ArchiveRequest> Create(nsIDOMWindow* aOwner,
                                                 ArchiveReader* aReader);

private:
  ~ArchiveRequest();

  nsresult GetFilenamesResult(JSContext* aCx,
                              jsval* aValue,
                              nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);
  nsresult GetFileResult(JSContext* aCx,
                         jsval* aValue,
                         nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);

protected:
  
  nsRefPtr<ArchiveReader> mArchiveReader;

  
  enum {
    GetFilenames,
    GetFile
  } mOperation;

  
  nsString mFilename;
};

END_FILE_NAMESPACE

#endif 
