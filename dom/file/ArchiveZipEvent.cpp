





#include "ArchiveZipEvent.h"
#include "ArchiveZipFile.h"

#include "nsContentUtils.h"
#include "nsCExternalHandlerService.h"

USING_FILE_NAMESPACE

#ifndef PATH_MAX
#  define PATH_MAX 65536 // The filename length is stored in 2 bytes
#endif


ArchiveZipItem::ArchiveZipItem(const char* aFilename,
                               ZipCentral& aCentralStruct)
: mFilename(aFilename),
  mCentralStruct(aCentralStruct)
{
  MOZ_COUNT_CTOR(ArchiveZipItem);
}

ArchiveZipItem::~ArchiveZipItem()
{
  MOZ_COUNT_DTOR(ArchiveZipItem);
}


nsCString
ArchiveZipItem::GetFilename()
{
  return mFilename;
}

void
ArchiveZipItem::SetFilename(const nsCString& aFilename)
{
  mFilename = aFilename;
}



nsIDOMFile*
ArchiveZipItem::File(ArchiveReader* aArchiveReader)
{
  return new ArchiveZipFile(NS_ConvertUTF8toUTF16(mFilename),
                            NS_ConvertUTF8toUTF16(GetType()),
                            StrToInt32(mCentralStruct.orglen),
                            mCentralStruct,
                            aArchiveReader);
}

uint32_t
ArchiveZipItem::StrToInt32(const uint8_t* aStr)
{
  return (uint32_t)( (aStr [0] <<  0) |
                     (aStr [1] <<  8) |
                     (aStr [2] << 16) |
                     (aStr [3] << 24) );
}

uint16_t
ArchiveZipItem::StrToInt16(const uint8_t* aStr)
{
  return (uint16_t) ((aStr [0]) | (aStr [1] << 8));
}



ArchiveReaderZipEvent::ArchiveReaderZipEvent(ArchiveReader* aArchiveReader)
: ArchiveReaderEvent(aArchiveReader)
{
}


nsresult
ArchiveReaderZipEvent::Exec()
{
  uint32_t centralOffset(0);
  nsresult rv;

  nsCOMPtr<nsIInputStream> inputStream;
  rv = mArchiveReader->GetInputStream(getter_AddRefs(inputStream));
  if (NS_FAILED(rv) || !inputStream) {
    return RunShare(NS_ERROR_UNEXPECTED);
  }

  
  nsCOMPtr<nsISeekableStream> seekableStream;
  seekableStream = do_QueryInterface(inputStream);
  if (!seekableStream) {
    return RunShare(NS_ERROR_UNEXPECTED);
  }

  uint64_t size;
  rv = mArchiveReader->GetSize(&size);
  if (NS_FAILED(rv)) {
    return RunShare(NS_ERROR_UNEXPECTED);
  }

  
  for (uint64_t curr = size - ZIPEND_SIZE; curr > 4; --curr)
  {
    seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, curr);

    uint8_t buffer[ZIPEND_SIZE];
    uint32_t ret;

    rv = inputStream->Read((char*)buffer, sizeof(buffer), &ret);
    if (NS_FAILED(rv) || ret != sizeof(buffer)) {
      return RunShare(NS_ERROR_UNEXPECTED);
    }

    
    if (ArchiveZipItem::StrToInt32(buffer) == ENDSIG) {
      centralOffset = ArchiveZipItem::StrToInt32(((ZipEnd*)buffer)->offset_central_dir);
      break;
    }
  }

  
  if (!centralOffset || centralOffset >= size - ZIPEND_SIZE) {
    return RunShare(NS_ERROR_FAILURE);
  }

  
  seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, centralOffset);

  
  while (centralOffset <= size - ZIPCENTRAL_SIZE) {
    ZipCentral centralStruct;
    uint32_t ret;
    
    rv = inputStream->Read((char*)&centralStruct, ZIPCENTRAL_SIZE, &ret);
    if (NS_FAILED(rv) || ret != ZIPCENTRAL_SIZE) {
      return RunShare(NS_ERROR_UNEXPECTED);
    }

    uint16_t filenameLen = ArchiveZipItem::StrToInt16(centralStruct.filename_len);
    uint16_t extraLen = ArchiveZipItem::StrToInt16(centralStruct.extrafield_len);
    uint16_t commentLen = ArchiveZipItem::StrToInt16(centralStruct.commentfield_len);

    
    centralOffset += ZIPCENTRAL_SIZE + filenameLen + extraLen + commentLen;
    if (filenameLen == 0 || filenameLen >= PATH_MAX || centralOffset >= size) {
      return RunShare(NS_ERROR_FILE_CORRUPTED);
    }

    
    char* filename = (char*)PR_Malloc(filenameLen + 1);
    rv = inputStream->Read(filename, filenameLen, &ret);
    if (NS_FAILED(rv) || ret != filenameLen) {
      return RunShare(NS_ERROR_UNEXPECTED);
    }

    filename[filenameLen] = 0;

    
    if (filename[filenameLen - 1] != '/') {
      mFileList.AppendElement(new ArchiveZipItem(filename, centralStruct));
    }

    PR_Free(filename);

    
    seekableStream->Seek(nsISeekableStream::NS_SEEK_CUR, extraLen + commentLen);
  }

  return RunShare(NS_OK);
}
