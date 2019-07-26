





#ifndef mozilla_dom_archivereader_domarchivereader_h__
#define mozilla_dom_archivereader_domarchivereader_h__

#include "nsWrapperCache.h"

#include "ArchiveReaderCommon.h"

#include "nsCOMArray.h"
#include "nsIChannel.h"
#include "nsIDOMFile.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
class ArchiveReaderOptions;
class GlobalObject;
} 
} 

BEGIN_ARCHIVEREADER_NAMESPACE

class ArchiveRequest;




class ArchiveReader MOZ_FINAL : public nsISupports,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ArchiveReader)

  static already_AddRefed<ArchiveReader>
  Constructor(const GlobalObject& aGlobal, nsIDOMBlob* aBlob,
              const ArchiveReaderOptions& aOptions, ErrorResult& aError);

  ArchiveReader(nsIDOMBlob* aBlob, nsPIDOMWindow* aWindow,
                const nsACString& aEncoding);

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  already_AddRefed<ArchiveRequest> GetFilenames();
  already_AddRefed<ArchiveRequest> GetFile(const nsAString& filename);
  already_AddRefed<ArchiveRequest> GetFiles();

  nsresult GetInputStream(nsIInputStream** aInputStream);
  nsresult GetSize(uint64_t* aSize);

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

  
  nsCOMPtr<nsPIDOMWindow> mWindow;

  
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

  nsCString mEncoding;
};

END_ARCHIVEREADER_NAMESPACE

#endif 
