

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zFolderOutStream.h"
#include "7zMethods.h"
#include "7zDecode.h"


#include "../../../Common/ComTry.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

namespace NArchive {
namespace N7z {

struct CExtractFolderInfo
{
  #ifdef _7Z_VOL
  int VolumeIndex;
  #endif
  CNum FileIndex;
  CNum FolderIndex;
  CBoolVector ExtractStatuses;
  UInt64 UnPackSize;
  CExtractFolderInfo(
    #ifdef _7Z_VOL
    int volumeIndex, 
    #endif
    CNum fileIndex, CNum folderIndex): 
    #ifdef _7Z_VOL
    VolumeIndex(volumeIndex),
    #endif
    FileIndex(fileIndex),
    FolderIndex(folderIndex), 
    UnPackSize(0) 
  {
    if (fileIndex != kNumNoIndex)
    {
      ExtractStatuses.Reserve(1);
      ExtractStatuses.Add(true);
    }
  };
};

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallbackSpec)
{
  COM_TRY_BEGIN
  bool testMode = (testModeSpec != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UInt64 importantTotalUnPacked = 0;

  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = 
    #ifdef _7Z_VOL
    _refs.Size();
    #else
    _database.Files.Size();
    #endif

  if(numItems == 0)
    return S_OK;

  






  
  CObjectVector<CExtractFolderInfo> extractFolderInfoVector;
  for(UInt32 ii = 0; ii < numItems; ii++)
  {
    
    UInt32 ref2Index = allFilesMode ? ii : indices[ii];
    

    
    {
      #ifdef _7Z_VOL
      
      const CRef &ref = _refs[ref2Index];

      int volumeIndex = ref.VolumeIndex;
      const CVolume &volume = _volumes[volumeIndex];
      const CArchiveDatabaseEx &database = volume.Database;
      UInt32 fileIndex = ref.ItemIndex;
      #else
      const CArchiveDatabaseEx &database = _database;
      UInt32 fileIndex = ref2Index;
      #endif

      CNum folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
      if (folderIndex == kNumNoIndex)
      {
        extractFolderInfoVector.Add(CExtractFolderInfo(
            #ifdef _7Z_VOL
            volumeIndex, 
            #endif
            fileIndex, kNumNoIndex));
        continue;
      }
      if (extractFolderInfoVector.IsEmpty() || 
        folderIndex != extractFolderInfoVector.Back().FolderIndex 
        #ifdef _7Z_VOL
        || volumeIndex != extractFolderInfoVector.Back().VolumeIndex
        #endif
        )
      {
        extractFolderInfoVector.Add(CExtractFolderInfo(
            #ifdef _7Z_VOL
            volumeIndex, 
            #endif
            kNumNoIndex, folderIndex));
        const CFolder &folderInfo = database.Folders[folderIndex];
        UInt64 unPackSize = folderInfo.GetUnPackSize();
        importantTotalUnPacked += unPackSize;
        extractFolderInfoVector.Back().UnPackSize = unPackSize;
      }
      
      CExtractFolderInfo &efi = extractFolderInfoVector.Back();
      
      
      CNum startIndex = database.FolderStartFileIndex[folderIndex];
      for (CNum index = efi.ExtractStatuses.Size();
          index <= fileIndex - startIndex; index++)
      {
        
        
        
        
        efi.ExtractStatuses.Add(index == fileIndex - startIndex);
      }
    }
  }

  extractCallback->SetTotal(importantTotalUnPacked);

  CDecoder decoder(
    #ifdef _ST_MODE
    false
    #else
    true
    #endif
    );
  

  UInt64 currentImportantTotalUnPacked = 0;
  UInt64 totalFolderUnPacked;

  for(int i = 0; i < extractFolderInfoVector.Size(); i++, 
      currentImportantTotalUnPacked += totalFolderUnPacked)
  {
    const CExtractFolderInfo &efi = extractFolderInfoVector[i];
    totalFolderUnPacked = efi.UnPackSize;

    RINOK(extractCallback->SetCompleted(&currentImportantTotalUnPacked));

    CFolderOutStream *folderOutStream = new CFolderOutStream;
    CMyComPtr<ISequentialOutStream> outStream(folderOutStream);

    #ifdef _7Z_VOL
    const CVolume &volume = _volumes[efi.VolumeIndex];
    const CArchiveDatabaseEx &database = volume.Database;
    #else
    const CArchiveDatabaseEx &database = _database;
    #endif

    CNum startIndex;
    if (efi.FileIndex != kNumNoIndex)
      startIndex = efi.FileIndex;
    else
      startIndex = database.FolderStartFileIndex[efi.FolderIndex];


    HRESULT result = folderOutStream->Init(&database, 
        #ifdef _7Z_VOL
        volume.StartRef2Index, 
        #else
        0,
        #endif
        startIndex, 
        &efi.ExtractStatuses, extractCallback, testMode);

    RINOK(result);

    if (efi.FileIndex != kNumNoIndex)
      continue;

    CNum folderIndex = efi.FolderIndex;
    const CFolder &folderInfo = database.Folders[folderIndex];

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);

    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, NULL, &currentImportantTotalUnPacked);

    CNum packStreamIndex = database.FolderStartPackStreamIndex[folderIndex];
    UInt64 folderStartPackPos = database.GetFolderStreamPos(folderIndex, 0);

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (extractCallback)
      extractCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    #endif

    try
    {
      HRESULT result = decoder.Decode(
          #ifdef _7Z_VOL
          volume.Stream,
          #else
          _inStream,
          #endif
          folderStartPackPos, 
          &database.PackSizes[packStreamIndex],
          folderInfo,
          outStream,
          compressProgress
          #ifndef _NO_CRYPTO
          , getTextPassword
          #endif
          #ifdef COMPRESS_MT
          , true, _numThreads
          #endif
          );

      if (result == S_FALSE)
      {
        RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kDataError));
        continue;
      }
      if (result == E_NOTIMPL)
      {
        RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }
      if (result != S_OK)
        return result;
      if (folderOutStream->WasWritingFinished() != S_OK)
      {
        RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kDataError));
        continue;
      }
    }
    catch(...)
    {
      RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kDataError));
      continue;
    }
  }
  return S_OK;
  COM_TRY_END
}

}}
