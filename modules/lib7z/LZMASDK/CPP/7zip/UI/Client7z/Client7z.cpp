

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/MyInitGuid.h"
#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/NtCheck.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../../Common/FileStreams.h"

#include "../../Archive/IArchive.h"

#include "../../IPassword.h"
#include "../../MyVersion.h"



DEFINE_GUID(CLSID_CFormat7z,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

using namespace NWindows;

#define kDllName "7z.dll"

static const char *kCopyrightString = MY_7ZIP_VERSION
" ("  kDllName " client) "
MY_COPYRIGHT " " MY_DATE;

static const char *kHelpString =
"Usage: Client7z.exe [a | l | x ] archive.7z [fileName ...]\n"
"Examples:\n"
"  Client7z.exe a archive.7z f1.txt f2.txt  : compress two files to archive.7z\n"
"  Client7z.exe l archive.7z   : List contents of archive.7z\n"
"  Client7z.exe x archive.7z   : eXtract files from archive.7z\n";


typedef UINT32 (WINAPI * CreateObjectFunc)(
    const GUID *clsID,
    const GUID *interfaceID,
    void **outObject);


void PrintString(const UString &s)
{
  printf("%s", (LPCSTR)GetOemString(s));
}

void PrintString(const AString &s)
{
  printf("%s", (LPCSTR)s);
}

void PrintNewLine()
{
  PrintString("\n");
}

void PrintStringLn(const AString &s)
{
  PrintString(s);
  PrintNewLine();
}

void PrintError(const AString &s)
{
  PrintNewLine();
  PrintString(s);
  PrintNewLine();
}

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if (prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}


static const wchar_t *kEmptyFileAlias = L"[Content]";






class CArchiveOpenCallback:
  public IArchiveOpenCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  bool PasswordIsDefined;
  UString Password;

  CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * , const UInt64 * )
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * , const UInt64 * )
{
  return S_OK;
}
  
STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    
    
    
    PrintError("Password is not defined");
    return E_ABORT;
  }
  return StringToBstr(Password, password);
}





static const wchar_t *kCantDeleteOutputFile = L"ERROR: Can not delete output file ";

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCRCFailed = "CRC Failed";
static const char *kDataError = "Data Error";
static const char *kUnknownError = "Unknown Error";

class CArchiveExtractCallback:
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CMyComPtr<IInArchive> _archiveHandler;
  UString _directoryPath;  
  UString _filePath;       
  UString _diskFilePath;   
  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME MTime;
    UInt32 Attrib;
    bool isDir;
    bool AttribDefined;
    bool MTimeDefined;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;

public:
  void Init(IInArchive *archiveHandler, const UString &directoryPath);

  UInt64 NumErrors;
  bool PasswordIsDefined;
  UString Password;

  CArchiveExtractCallback() : PasswordIsDefined(false) {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath)
{
  NumErrors = 0;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 )
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * )
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
    ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();

  {
    
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop));
    
    UString fullPath;
    if (prop.vt == VT_EMPTY)
      fullPath = kEmptyFileAlias;
    else
    {
      if (prop.vt != VT_BSTR)
        return E_FAIL;
      fullPath = prop.bstrVal;
    }
    _filePath = fullPath;
  }

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
    return S_OK;

  {
    
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop));
    if (prop.vt == VT_EMPTY)
    {
      _processedFileInfo.Attrib = 0;
      _processedFileInfo.AttribDefined = false;
    }
    else
    {
      if (prop.vt != VT_UI4)
        return E_FAIL;
      _processedFileInfo.Attrib = prop.ulVal;
      _processedFileInfo.AttribDefined = true;
    }
  }

  RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.isDir));

  {
    
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop));
    _processedFileInfo.MTimeDefined = false;
    switch(prop.vt)
    {
      case VT_EMPTY:
        
        break;
      case VT_FILETIME:
        _processedFileInfo.MTime = prop.filetime;
        _processedFileInfo.MTimeDefined = true;
        break;
      default:
        return E_FAIL;
    }

  }
  {
    
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop));
    bool newFileSizeDefined = (prop.vt != VT_EMPTY);
    UInt64 newFileSize;
    if (newFileSizeDefined)
      newFileSize = ConvertPropVariantToUInt64(prop);
  }

  
  {
    
    int slashPos = _filePath.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (slashPos >= 0)
      NFile::NDirectory::CreateComplexDirectory(_directoryPath + _filePath.Left(slashPos));
  }

  UString fullProcessedPath = _directoryPath + _filePath;
  _diskFilePath = fullProcessedPath;

  if (_processedFileInfo.isDir)
  {
    NFile::NDirectory::CreateComplexDirectory(fullProcessedPath);
  }
  else
  {
    NFile::NFind::CFileInfoW fi;
    if (fi.Find(fullProcessedPath))
    {
      if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
      {
        PrintString(UString(kCantDeleteOutputFile) + fullProcessedPath);
        return E_ABORT;
      }
    }
    
    _outFileStreamSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
    if (!_outFileStreamSpec->Open(fullProcessedPath, CREATE_ALWAYS))
    {
      PrintString((UString)L"can not open output file " + fullProcessedPath);
      return E_ABORT;
    }
    _outFileStream = outStreamLoc;
    *outStream = outStreamLoc.Detach();
  }
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:  _extractMode = true; break;
  };
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:  PrintString(kExtractingString); break;
    case NArchive::NExtract::NAskMode::kTest:  PrintString(kTestingString); break;
    case NArchive::NExtract::NAskMode::kSkip:  PrintString(kSkippingString); break;
  };
  PrintString(_filePath);
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      NumErrors++;
      PrintString("     ");
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          PrintString(kUnsupportedMethod);
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          PrintString(kCRCFailed);
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          PrintString(kDataError);
          break;
        default:
          PrintString(kUnknownError);
      }
    }
  }

  if (_outFileStream != NULL)
  {
    if (_processedFileInfo.MTimeDefined)
      _outFileStreamSpec->SetMTime(&_processedFileInfo.MTime);
    RINOK(_outFileStreamSpec->Close());
  }
  _outFileStream.Release();
  if (_extractMode && _processedFileInfo.AttribDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attrib);
  PrintNewLine();
  return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    
    
    
    PrintError("Password is not defined");
    return E_ABORT;
  }
  return StringToBstr(Password, password);
}






struct CDirItem
{
  UInt64 Size;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  UString Name;
  UString FullPath;
  UInt32 Attrib;

  bool isDir() const { return (Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

class CArchiveUpdateCallback:
  public IArchiveUpdateCallback2,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)

  
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);
  STDMETHOD(GetUpdateItemInfo)(UInt32 index,
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive);
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream);
  STDMETHOD(SetOperationResult)(Int32 operationResult);
  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size);
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream);

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

public:
  CRecordVector<UInt64> VolumesSizes;
  UString VolName;
  UString VolExt;

  UString DirPrefix;
  const CObjectVector<CDirItem> *DirItems;

  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;

  bool m_NeedBeClosed;

  UStringVector FailedFiles;
  CRecordVector<HRESULT> FailedCodes;

  CArchiveUpdateCallback(): PasswordIsDefined(false), AskPassword(false), DirItems(0) {};

  ~CArchiveUpdateCallback() { Finilize(); }
  HRESULT Finilize();

  void Init(const CObjectVector<CDirItem> *dirItems)
  {
    DirItems = dirItems;
    m_NeedBeClosed = false;
    FailedFiles.Clear();
    FailedCodes.Clear();
  }
};

STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 )
{
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 * )
{
  return S_OK;
}


STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG ** )
{
  return E_NOTIMPL;
}

STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 ,
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)
{
  if (newData != NULL)
    *newData = BoolToInt(true);
  if (newProperties != NULL)
    *newProperties = BoolToInt(true);
  if (indexInArchive != NULL)
    *indexInArchive = (UInt32)-1;
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  
  if (propID == kpidIsAnti)
  {
    prop = false;
    prop.Detach(value);
    return S_OK;
  }

  {
    const CDirItem &dirItem = (*DirItems)[index];
    switch(propID)
    {
      case kpidPath:  prop = dirItem.Name; break;
      case kpidIsDir:  prop = dirItem.isDir(); break;
      case kpidSize:  prop = dirItem.Size; break;
      case kpidAttrib:  prop = dirItem.Attrib; break;
      case kpidCTime:  prop = dirItem.CTime; break;
      case kpidATime:  prop = dirItem.ATime; break;
      case kpidMTime:  prop = dirItem.MTime; break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CArchiveUpdateCallback::Finilize()
{
  if (m_NeedBeClosed)
  {
    PrintNewLine();
    m_NeedBeClosed = false;
  }
  return S_OK;
}

static void GetStream2(const wchar_t *name)
{
  PrintString("Compressing  ");
  if (name[0] == 0)
    name = kEmptyFileAlias;
  PrintString(name);
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
  RINOK(Finilize());

  const CDirItem &dirItem = (*DirItems)[index];
  GetStream2(dirItem.Name);
 
  if (dirItem.isDir())
    return S_OK;

  {
    CInFileStream *inStreamSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
    UString path = DirPrefix + dirItem.FullPath;
    if (!inStreamSpec->Open(path))
    {
      DWORD sysError = ::GetLastError();
      FailedCodes.Add(sysError);
      FailedFiles.Add(path);
      
      {
        PrintNewLine();
        PrintError("WARNING: can't open file");
        
        return S_FALSE;
      }
      
    }
    *inStream = inStreamLoc.Detach();
  }
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(Int32 )
{
  m_NeedBeClosed = true;
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(UInt32 index, UInt64 *size)
{
  if (VolumesSizes.Size() == 0)
    return S_FALSE;
  if (index >= (UInt32)VolumesSizes.Size())
    index = VolumesSizes.Size() - 1;
  *size = VolumesSizes[index];
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)
{
  wchar_t temp[16];
  ConvertUInt32ToString(index + 1, temp);
  UString res = temp;
  while (res.Length() < 2)
    res = UString(L'0') + res;
  UString fileName = VolName;
  fileName += L'.';
  fileName += res;
  fileName += VolExt;
  COutFileStream *streamSpec = new COutFileStream;
  CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
  if (!streamSpec->Create(fileName, false))
    return ::GetLastError();
  *volumeStream = streamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  if (!PasswordIsDefined)
  {
    if (AskPassword)
    {
      
      
      
      PrintError("Password is not defined");
      return E_ABORT;
    }
  }
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  return StringToBstr(Password, password);
}






#define NT_CHECK_FAIL_ACTION PrintError("Unsupported Windows version"); return 1;

int MY_CDECL main(int numArgs, const char *args[])
{
  NT_CHECK

  PrintStringLn(kCopyrightString);

  if (numArgs < 3)
  {
    PrintStringLn(kHelpString);
    return 1;
  }
  NWindows::NDLL::CLibrary lib;
  if (!lib.Load(TEXT(kDllName)))
  {
    PrintError("Can not load 7-zip library");
    return 1;
  }
  CreateObjectFunc createObjectFunc = (CreateObjectFunc)lib.GetProc("CreateObject");
  if (createObjectFunc == 0)
  {
    PrintError("Can not get CreateObject");
    return 1;
  }

  char c;
  {
    AString command = args[1];
    if (command.Length() != 1)
    {
      PrintError("incorrect command");
      return 1;
    }
    c = MyCharLower(command[0]);
  }
  UString archiveName = GetUnicodeString(args[2]);
  if (c == 'a')
  {
    
    if (numArgs < 4)
    {
      PrintStringLn(kHelpString);
      return 1;
    }
    CObjectVector<CDirItem> dirItems;
    int i;
    for (i = 3; i < numArgs; i++)
    {
      CDirItem di;
      UString name = GetUnicodeString(args[i]);
      
      NFile::NFind::CFileInfoW fi;
      if (!fi.Find(name))
      {
        PrintString(UString(L"Can't find file") + name);
        return 1;
      }

      di.Attrib = fi.Attrib;
      di.Size = fi.Size;
      di.CTime = fi.CTime;
      di.ATime = fi.ATime;
      di.MTime = fi.MTime;
      di.Name = name;
      di.FullPath = name;
      dirItems.Add(di);
    }
    COutFileStream *outFileStreamSpec = new COutFileStream;
    CMyComPtr<IOutStream> outFileStream = outFileStreamSpec;
    if (!outFileStreamSpec->Create(archiveName, false))
    {
      PrintError("can't create archive file");
      return 1;
    }

    CMyComPtr<IOutArchive> outArchive;
    if (createObjectFunc(&CLSID_CFormat7z, &IID_IOutArchive, (void **)&outArchive) != S_OK)
    {
      PrintError("Can not get class object");
      return 1;
    }

    CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
    CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCallbackSpec);
    updateCallbackSpec->Init(&dirItems);
    
    

    






















    
    HRESULT result = outArchive->UpdateItems(outFileStream, dirItems.Size(), updateCallback);
    updateCallbackSpec->Finilize();
    if (result != S_OK)
    {
      PrintError("Update Error");
      return 1;
    }
    for (i = 0; i < updateCallbackSpec->FailedFiles.Size(); i++)
    {
      PrintNewLine();
      PrintString((UString)L"Error for file: " + updateCallbackSpec->FailedFiles[i]);
    }
    if (updateCallbackSpec->FailedFiles.Size() != 0)
      return 1;
  }
  else
  {
    if (numArgs != 3)
    {
      PrintStringLn(kHelpString);
      return 1;
    }

    bool listCommand;
    if (c == 'l')
      listCommand = true;
    else if (c == 'x')
      listCommand = false;
    else
    {
      PrintError("incorrect command");
      return 1;
    }
  
    CMyComPtr<IInArchive> archive;
    if (createObjectFunc(&CLSID_CFormat7z, &IID_IInArchive, (void **)&archive) != S_OK)
    {
      PrintError("Can not get class object");
      return 1;
    }
    
    CInFileStream *fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file = fileSpec;
    
    if (!fileSpec->Open(archiveName))
    {
      PrintError("Can not open archive file");
      return 1;
    }

    {
      CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
      CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
      openCallbackSpec->PasswordIsDefined = false;
      
      
      
      if (archive->Open(file, 0, openCallback) != S_OK)
      {
        PrintError("Can not open archive");
        return 1;
      }
    }
    
    if (listCommand)
    {
      
      UInt32 numItems = 0;
      archive->GetNumberOfItems(&numItems);
      for (UInt32 i = 0; i < numItems; i++)
      {
        {
          
          NWindows::NCOM::CPropVariant prop;
          archive->GetProperty(i, kpidSize, &prop);
          UString s = ConvertPropVariantToString(prop);
          PrintString(s);
          PrintString("  ");
        }
        {
          
          NWindows::NCOM::CPropVariant prop;
          archive->GetProperty(i, kpidPath, &prop);
          UString s = ConvertPropVariantToString(prop);
          PrintString(s);
        }
        PrintString("\n");
      }
    }
    else
    {
      
      CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
      CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
      extractCallbackSpec->Init(archive, L""); 
      extractCallbackSpec->PasswordIsDefined = false;
      
      
      HRESULT result = archive->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
      if (result != S_OK)
      {
        PrintError("Extract Error");
        return 1;
      }
    }
  }
  return 0;
}
