





#ifndef mozilla_dom_quota_filestreams_h__
#define mozilla_dom_quota_filestreams_h__

#include "QuotaCommon.h"

#include "nsFileStreams.h"

#include "QuotaObject.h"

BEGIN_QUOTA_NAMESPACE

template <class FileStreamBase>
class FileQuotaStream : public FileStreamBase
{
public:
  
  NS_IMETHOD
  SetEOF() MOZ_OVERRIDE;

  NS_IMETHOD
  Close() MOZ_OVERRIDE;

protected:
  FileQuotaStream(const nsACString& aOrigin)
  : mOrigin(aOrigin)
  { }

  
  virtual nsresult
  DoOpen() MOZ_OVERRIDE;

  nsCString mOrigin;
  nsRefPtr<QuotaObject> mQuotaObject;
};

template <class FileStreamBase>
class FileQuotaStreamWithWrite : public FileQuotaStream<FileStreamBase>
{
public:
  
  NS_IMETHOD
  Write(const char* aBuf, uint32_t aCount, uint32_t* _retval) MOZ_OVERRIDE;

protected:
  FileQuotaStreamWithWrite(const nsACString& aOrigin)
  : FileQuotaStream<FileStreamBase>(aOrigin)
  { }
};

class FileInputStream : public FileQuotaStream<nsFileInputStream>
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<FileInputStream>
  Create(const nsACString& aOrigin, nsIFile* aFile, int32_t aIOFlags = -1,
         int32_t aPerm = -1, int32_t aBehaviorFlags = 0);

private:
  FileInputStream(const nsACString& aOrigin)
  : FileQuotaStream<nsFileInputStream>(aOrigin)
  { }

  virtual ~FileInputStream() {
    Close();
  }
};

class FileOutputStream : public FileQuotaStreamWithWrite<nsFileOutputStream>
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<FileOutputStream>
  Create(const nsACString& aOrigin, nsIFile* aFile, int32_t aIOFlags = -1,
         int32_t aPerm = -1, int32_t aBehaviorFlags = 0);

private:
  FileOutputStream(const nsACString& aOrigin)
  : FileQuotaStreamWithWrite<nsFileOutputStream>(aOrigin)
  { }

  virtual ~FileOutputStream() {
    Close();
  }
};

class FileStream : public FileQuotaStreamWithWrite<nsFileStream>
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  static already_AddRefed<FileStream>
  Create(const nsACString& aOrigin, nsIFile* aFile, int32_t aIOFlags = -1,
         int32_t aPerm = -1, int32_t aBehaviorFlags = 0);

private:
  FileStream(const nsACString& aOrigin)
  : FileQuotaStreamWithWrite<nsFileStream>(aOrigin)
  { }

  virtual ~FileStream() {
    Close();
  }
};

END_QUOTA_NAMESPACE

#endif
