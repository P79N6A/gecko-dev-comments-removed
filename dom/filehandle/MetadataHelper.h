





#ifndef mozilla_dom_MetadataHelper_h
#define mozilla_dom_MetadataHelper_h

#include "AsyncHelper.h"
#include "FileHelper.h"
#include "js/TypeDecls.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class MetadataHelper;

class MetadataParameters MOZ_FINAL
{
  friend class MetadataHelper;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MetadataParameters)

  MetadataParameters(bool aSizeRequested, bool aLastModifiedRequested)
    : mSizeRequested(aSizeRequested)
    , mLastModifiedRequested(aLastModifiedRequested)
  {
  }

  bool
  IsConfigured() const
  {
    return mSizeRequested || mLastModifiedRequested;
  }

  bool
  SizeRequested() const
  {
    return mSizeRequested;
  }

  bool
  LastModifiedRequested() const
  {
    return mLastModifiedRequested;
  }

  uint64_t
  Size() const
  {
    return mSize;
  }

  int64_t
  LastModified() const
  {
    return mLastModified;
  }

private:
  
  ~MetadataParameters()
  {
  }

  uint64_t mSize;
  int64_t mLastModified;
  bool mSizeRequested;
  bool mLastModifiedRequested;
};

class MetadataHelper : public FileHelper
{
public:
  MetadataHelper(LockedFile* aLockedFile,
                 FileRequest* aFileRequest,
                 MetadataParameters* aParams)
  : FileHelper(aLockedFile, aFileRequest),
    mParams(aParams)
  { }

  nsresult
  DoAsyncRun(nsISupports* aStream) MOZ_OVERRIDE;

  nsresult
  GetSuccessResult(JSContext* aCx,
                   JS::MutableHandle<JS::Value> aVal) MOZ_OVERRIDE;

protected:
  class AsyncMetadataGetter : public AsyncHelper
  {
  public:
    AsyncMetadataGetter(nsISupports* aStream, MetadataParameters* aParams,
                        bool aReadWrite)
    : AsyncHelper(aStream),
      mParams(aParams), mReadWrite(aReadWrite)
    { }

  protected:
    nsresult
    DoStreamWork(nsISupports* aStream) MOZ_OVERRIDE;

  private:
    nsRefPtr<MetadataParameters> mParams;
    bool mReadWrite;
  };

  nsRefPtr<MetadataParameters> mParams;
};

} 
} 

#endif 
