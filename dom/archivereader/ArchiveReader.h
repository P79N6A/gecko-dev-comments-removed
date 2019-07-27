





#ifndef mozilla_dom_archivereader_domarchivereader_h__
#define mozilla_dom_archivereader_domarchivereader_h__

#include "nsWrapperCache.h"

#include "ArchiveReaderCommon.h"

#include "nsCOMArray.h"
#include "nsIChannel.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
struct ArchiveReaderOptions;
class Blob;
class BlobImpl;
class File;
class GlobalObject;
} 
} 

BEGIN_ARCHIVEREADER_NAMESPACE

class ArchiveRequest;




class ArchiveReader final : public nsISupports,
                            public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ArchiveReader)

  static already_AddRefed<ArchiveReader>
  Constructor(const GlobalObject& aGlobal, Blob& aBlob,
              const ArchiveReaderOptions& aOptions, ErrorResult& aError);

  ArchiveReader(Blob& aBlob, nsPIDOMWindow* aWindow,
                const nsACString& aEncoding);

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<ArchiveRequest> GetFilenames();
  already_AddRefed<ArchiveRequest> GetFile(const nsAString& filename);
  already_AddRefed<ArchiveRequest> GetFiles();

  nsresult GetInputStream(nsIInputStream** aInputStream);
  nsresult GetSize(uint64_t* aSize);

public: 
  nsresult RegisterRequest(ArchiveRequest* aRequest);

public: 
  BlobImpl* GetBlobImpl() const
  {
    return mBlobImpl;
  }

  void Ready(nsTArray<nsRefPtr<File>>& aFileList, nsresult aStatus);

private:
  ~ArchiveReader();

  already_AddRefed<ArchiveRequest> GenerateArchiveRequest();

  nsresult OpenArchive();

  void RequestReady(ArchiveRequest* aRequest);

protected:
  
  nsRefPtr<BlobImpl> mBlobImpl;

  
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
    nsTArray<nsRefPtr<File>> fileList;
    nsresult status;
  } mData;

  nsCString mEncoding;
};

END_ARCHIVEREADER_NAMESPACE

#endif 
