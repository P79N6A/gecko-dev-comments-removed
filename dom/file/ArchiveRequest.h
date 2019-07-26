





#ifndef mozilla_dom_file_domarchiverequest_h__
#define mozilla_dom_file_domarchiverequest_h__

#include "ArchiveReader.h"
#include "DOMRequest.h"

#include "FileCommon.h"


BEGIN_FILE_NAMESPACE





class ArchiveRequest : public mozilla::dom::DOMRequest
{
public:
  static bool PrefEnabled()
  {
    return ArchiveReader::PrefEnabled();
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  ArchiveReader* Reader() const;

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ArchiveRequest, DOMRequest)

  ArchiveRequest(nsIDOMWindow* aWindow,
                 ArchiveReader* aReader);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

public:
  
  void Run();

  
  void OpGetFilenames();
  void OpGetFile(const nsAString& aFilename);
  void OpGetFiles();

  nsresult ReaderReady(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
                       nsresult aStatus);

public: 
  static already_AddRefed<ArchiveRequest> Create(nsIDOMWindow* aOwner,
                                                 ArchiveReader* aReader);

private:
  ~ArchiveRequest();

  nsresult GetFilenamesResult(JSContext* aCx,
                              JS::Value* aValue,
                              nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);
  nsresult GetFileResult(JSContext* aCx,
                         JS::Value* aValue,
                         nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);
  nsresult GetFilesResult(JSContext* aCx,
                          JS::Value* aValue,
                          nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);

protected:
  
  nsRefPtr<ArchiveReader> mArchiveReader;

  
  enum {
    GetFilenames,
    GetFile,
    GetFiles
  } mOperation;

  
  nsString mFilename;
};

END_FILE_NAMESPACE

#endif 
