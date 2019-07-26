





#ifndef mozilla_dom_file_metadatahelper_h__
#define mozilla_dom_file_metadatahelper_h__

#include "mozilla/Attributes.h"
#include "FileCommon.h"

#include "nsIFileStreams.h"

#include "AsyncHelper.h"
#include "FileHelper.h"

class nsIFileStream;

BEGIN_FILE_NAMESPACE

class MetadataHelper;

class MetadataParameters
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
  GetSuccessResult(JSContext* aCx, JS::Value* aVal) MOZ_OVERRIDE;

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

END_FILE_NAMESPACE

#endif 
