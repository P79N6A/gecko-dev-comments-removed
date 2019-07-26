





#ifndef mozilla_dom_file_domarchivereader_h__
#define mozilla_dom_file_domarchivereader_h__

#include "nsIDOMArchiveReader.h"
#include "nsIJSNativeInitializer.h"

#include "FileCommon.h"

#include "nsCOMArray.h"
#include "nsIChannel.h"
#include "nsIDOMFile.h"
#include "mozilla/Attributes.h"
#include "DictionaryHelpers.h"

BEGIN_FILE_NAMESPACE

class ArchiveRequest;




class ArchiveReader MOZ_FINAL : public nsIDOMArchiveReader,
                                public nsIJSNativeInitializer
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMARCHIVEREADER

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ArchiveReader,
                                           nsIDOMArchiveReader)

  ArchiveReader();

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        uint32_t aArgc,
                        JS::Value* aArgv);

  nsresult GetInputStream(nsIInputStream** aInputStream);
  nsresult GetSize(uint64_t* aSize);

  static bool PrefEnabled();

public: 
  nsresult RegisterRequest(ArchiveRequest* aRequest);

public: 
  void Ready(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
             nsresult aStatus);

private:
  ~ArchiveReader();

  already_AddRefed<ArchiveRequest> GenerateArchiveRequest();

  nsresult OpenArchive();

  void RequestReady(ArchiveRequest* aRequest);

protected:
  
  nsCOMPtr<nsIDOMBlob> mBlob;

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  enum {
    NOT_STARTED = 0,
    WORKING,
    READY
  } mStatus;

  
  enum {
    Header, 
    Name,   
    Data,   
    Search  
  } mReadStatus;

  
  nsTArray<nsRefPtr<ArchiveRequest> > mRequests;

  
  struct {
    nsTArray<nsCOMPtr<nsIDOMFile> > fileList;
    nsresult status;
  } mData;

  nsString mEncoding;
};

END_FILE_NAMESPACE

#endif 
