





#ifndef mozilla_dom_file_domarchivezipevent_h__
#define mozilla_dom_file_domarchivezipevent_h__

#include "ArchiveEvent.h"

#include "FileCommon.h"
#include "zipstruct.h"

BEGIN_FILE_NAMESPACE

class ArchiveZipItem : public ArchiveItem
{
public:
  ArchiveZipItem(const char* aFilename,
                 ZipCentral& aCentralStruct);
  virtual ~ArchiveZipItem();

  void SetFilename(const nsCString& aFilename);
  nsCString GetFilename();

  
  virtual nsIDOMFile* File(ArchiveReader* aArchiveReader);

public: 
  static uint32_t StrToInt32(const uint8_t* aStr);
  static uint16_t StrToInt16(const uint8_t* aStr);

private: 
  nsCString mFilename;
  ZipCentral mCentralStruct;
};

class ArchiveReaderZipEvent : public ArchiveReaderEvent
{
public:
  ArchiveReaderZipEvent(ArchiveReader* aArchiveReader);

  nsresult Exec();
};

END_FILE_NAMESPACE

#endif 

