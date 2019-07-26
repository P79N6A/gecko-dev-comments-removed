





#ifndef mozilla_dom_archivereader_domarchivezipevent_h__
#define mozilla_dom_archivereader_domarchivezipevent_h__

#include "mozilla/Attributes.h"
#include "ArchiveEvent.h"

#include "ArchiveReaderCommon.h"
#include "zipstruct.h"

BEGIN_ARCHIVEREADER_NAMESPACE




class ArchiveZipItem : public ArchiveItem
{
public:
  ArchiveZipItem(const char* aFilename,
                 const ZipCentral& aCentralStruct,
                 const nsACString& aEncoding);
  virtual ~ArchiveZipItem();

  nsresult GetFilename(nsString& aFilename) MOZ_OVERRIDE;

  
  virtual nsIDOMFile* File(ArchiveReader* aArchiveReader) MOZ_OVERRIDE;

public: 
  static uint32_t StrToInt32(const uint8_t* aStr);
  static uint16_t StrToInt16(const uint8_t* aStr);

private:
  nsresult ConvertFilename();

private: 
  nsCString mFilename;

  nsString mFilenameU;
  ZipCentral mCentralStruct;

  nsCString mEncoding;
};




class ArchiveReaderZipEvent : public ArchiveReaderEvent
{
public:
  ArchiveReaderZipEvent(ArchiveReader* aArchiveReader,
                        const nsACString& aEncoding);

  nsresult Exec() MOZ_OVERRIDE;

private:
  nsCString mEncoding;
};

END_ARCHIVEREADER_NAMESPACE

#endif 
