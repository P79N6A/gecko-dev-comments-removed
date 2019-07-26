





#ifndef mozilla_dom_file_domarchiveevent_h__
#define mozilla_dom_file_domarchiveevent_h__

#include "ArchiveReader.h"

#include "nsISeekableStream.h"
#include "nsIMIMEService.h"
#include "nsDOMFile.h"

#include "FileCommon.h"

BEGIN_FILE_NAMESPACE





class ArchiveItem : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  ArchiveItem();
  virtual ~ArchiveItem();

  
  nsCString GetType();
  void SetType(const nsCString& aType);

  
  virtual nsresult GetFilename(nsString& aFilename) = 0;

  
  virtual nsIDOMFile* File(ArchiveReader* aArchiveReader) = 0;

protected:
  nsCString mType;
};






class ArchiveReaderEvent : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  ArchiveReaderEvent(ArchiveReader* aArchiveReader);

  virtual ~ArchiveReaderEvent();

  
  virtual nsresult Exec() = 0;

protected:
  nsresult GetType(nsCString& aExt,
                   nsCString& aMimeType);

  nsresult RunShare(nsresult aStatus);
  void ShareMainThread();

protected: 
  ArchiveReader* mArchiveReader;

  nsCOMPtr<nsIMIMEService> mMimeService;

  nsTArray<nsRefPtr<ArchiveItem> > mFileList; 
  nsresult mStatus;
};

END_FILE_NAMESPACE

#endif 

