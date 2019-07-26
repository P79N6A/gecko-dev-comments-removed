





#ifndef mozilla_dom_file_domarchivezipevent_h__
#define mozilla_dom_file_domarchivezipevent_h__

#include "ArchiveEvent.h"

#include "FileCommon.h"
#include "zipstruct.h"

#include "DictionaryHelpers.h"

BEGIN_FILE_NAMESPACE




class ArchiveZipItem : public ArchiveItem
{
public:
  ArchiveZipItem(const char* aFilename,
                 const ZipCentral& aCentralStruct,
                 const nsAString& aEncoding);
  virtual ~ArchiveZipItem();

  nsresult GetFilename(nsString& aFilename);

  
  virtual nsIDOMFile* File(ArchiveReader* aArchiveReader);

public: 
  static uint32_t StrToInt32(const uint8_t* aStr);
  static uint16_t StrToInt16(const uint8_t* aStr);

private:
  nsresult ConvertFilename();

private: 
  nsCString mFilename;

  nsString mFilenameU;
  ZipCentral mCentralStruct;

  nsString mEncoding;
};




class ArchiveReaderZipEvent : public ArchiveReaderEvent
{
public:
  ArchiveReaderZipEvent(ArchiveReader* aArchiveReader,
                        const nsAString& aEncoding);

  nsresult Exec();

private:
  nsString mEncoding;
};

END_FILE_NAMESPACE

#endif 

