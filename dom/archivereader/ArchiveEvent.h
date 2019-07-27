





#ifndef mozilla_dom_archivereader_domarchiveevent_h__
#define mozilla_dom_archivereader_domarchiveevent_h__

#include "ArchiveReader.h"

#include "nsISeekableStream.h"
#include "nsIMIMEService.h"
#include "nsDOMFile.h"

#include "ArchiveReaderCommon.h"

BEGIN_ARCHIVEREADER_NAMESPACE





class ArchiveItem : public nsISupports
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  ArchiveItem();

  
  nsCString GetType();
  void SetType(const nsCString& aType);

  
  virtual nsresult GetFilename(nsString& aFilename) = 0;

  
  virtual nsIDOMFile* File(ArchiveReader* aArchiveReader) = 0;

protected:
  virtual ~ArchiveItem();

  nsCString mType;
};






class ArchiveReaderEvent : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  explicit ArchiveReaderEvent(ArchiveReader* aArchiveReader);

protected:
  virtual ~ArchiveReaderEvent();

public:
  
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

END_ARCHIVEREADER_NAMESPACE

#endif 
