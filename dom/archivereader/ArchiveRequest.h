





#ifndef mozilla_dom_archivereader_domarchiverequest_h__
#define mozilla_dom_archivereader_domarchiverequest_h__

#include "mozilla/Attributes.h"
#include "ArchiveReader.h"
#include "DOMRequest.h"

#include "ArchiveReaderCommon.h"

namespace mozilla {
class EventChainPreVisitor;
} 

BEGIN_ARCHIVEREADER_NAMESPACE





class ArchiveRequest : public mozilla::dom::DOMRequest
{
public:
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  ArchiveReader* Reader() const;

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ArchiveRequest, DOMRequest)

  ArchiveRequest(nsPIDOMWindow* aWindow,
                 ArchiveReader* aReader);

  
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

public:
  
  void Run();

  
  void OpGetFilenames();
  void OpGetFile(const nsAString& aFilename);
  void OpGetFiles();

  nsresult ReaderReady(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
                       nsresult aStatus);

public: 
  static already_AddRefed<ArchiveRequest> Create(nsPIDOMWindow* aOwner,
                                                 ArchiveReader* aReader);

private:
  ~ArchiveRequest();

  nsresult GetFilenamesResult(JSContext* aCx,
                              JS::Value* aValue,
                              nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);
  nsresult GetFileResult(JSContext* aCx,
                         JS::MutableHandle<JS::Value> aValue,
                         nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList);
  nsresult GetFilesResult(JSContext* aCx,
                          JS::MutableHandle<JS::Value> aValue,
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

END_ARCHIVEREADER_NAMESPACE

#endif 
