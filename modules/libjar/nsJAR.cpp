



#include <string.h>
#include "nsJARInputStream.h"
#include "nsJAR.h"
#include "nsIFile.h"
#include "nsIX509Cert.h"
#include "nsIConsoleService.h"
#include "nsICryptoHash.h"
#include "nsIDataSignatureVerifier.h"
#include "prprf.h"
#include "mozilla/Omnijar.h"

#ifdef XP_UNIX
  #include <sys/stat.h>
#elif defined (XP_WIN)
  #include <io.h>
#endif

using namespace mozilla;











typedef enum
{
  JAR_INVALID       = 1,
  JAR_INTERNAL      = 2,
  JAR_EXTERNAL      = 3
} JARManifestItemType;

class nsJARManifestItem
{
public:
  JARManifestItemType mType;

  
  
  bool                entryVerified;

  
  int16_t             status;

  
  nsCString           calculatedSectionDigest;
  nsCString           storedEntryDigest;

  nsJARManifestItem();
  virtual ~nsJARManifestItem();
};




nsJARManifestItem::nsJARManifestItem(): mType(JAR_INTERNAL),
                                        entryVerified(false),
                                        status(JAR_NOT_SIGNED)
{
}

nsJARManifestItem::~nsJARManifestItem()
{
}






nsJAR::nsJAR(): mZip(new nsZipArchive()),
                mManifestData(8),
                mParsedManifest(false),
                mGlobalStatus(JAR_MANIFEST_NOT_PARSED),
                mReleaseTime(PR_INTERVAL_NO_TIMEOUT),
                mCache(nullptr),
                mLock("nsJAR::mLock"),
                mTotalItemsInManifest(0),
                mOpened(false)
{
}

nsJAR::~nsJAR()
{
  Close();
}

NS_IMPL_QUERY_INTERFACE(nsJAR, nsIZipReader)
NS_IMPL_ADDREF(nsJAR)


MozExternalRefCountType nsJAR::Release(void)
{
  nsrefcnt count;
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "nsJAR");
  if (0 == count) {
    mRefCnt = 1; 
    
    
    delete this;
    return 0;
  }
  else if (1 == count && mCache) {
#ifdef DEBUG
    nsresult rv =
#endif
      mCache->ReleaseZip(this);
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to release zip file");
  }
  return count;
}





NS_IMETHODIMP
nsJAR::Open(nsIFile* zipFile)
{
  NS_ENSURE_ARG_POINTER(zipFile);
  if (mOpened) return NS_ERROR_FAILURE; 

  mZipFile = zipFile;
  mOuterZipEntry.Truncate();
  mOpened = true;

  
  
  nsRefPtr<nsZipArchive> zip = mozilla::Omnijar::GetReader(zipFile);
  if (zip) {
    mZip = zip;
    return NS_OK;
  }
  return mZip->OpenArchive(zipFile, mCache ? mCache->IsMustCacheFdEnabled() : false);
}

NS_IMETHODIMP
nsJAR::OpenInner(nsIZipReader *aZipReader, const nsACString &aZipEntry)
{
  NS_ENSURE_ARG_POINTER(aZipReader);
  if (mOpened) return NS_ERROR_FAILURE; 

  bool exist;
  nsresult rv = aZipReader->HasEntry(aZipEntry, &exist);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(exist, NS_ERROR_FILE_NOT_FOUND);

  rv = aZipReader->GetFile(getter_AddRefs(mZipFile));
  NS_ENSURE_SUCCESS(rv, rv);

  mOpened = true;

  mOuterZipEntry.Assign(aZipEntry);

  nsRefPtr<nsZipHandle> handle;
  rv = nsZipHandle::Init(static_cast<nsJAR*>(aZipReader)->mZip.get(), PromiseFlatCString(aZipEntry).get(),
                         getter_AddRefs(handle));
  if (NS_FAILED(rv))
    return rv;

  return mZip->OpenArchive(handle);
}

NS_IMETHODIMP
nsJAR::GetFile(nsIFile* *result)
{
  *result = mZipFile;
  NS_IF_ADDREF(*result);
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::Close()
{
  mOpened = false;
  mParsedManifest = false;
  mManifestData.Clear();
  mGlobalStatus = JAR_MANIFEST_NOT_PARSED;
  mTotalItemsInManifest = 0;

  nsRefPtr<nsZipArchive> greOmni = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  nsRefPtr<nsZipArchive> appOmni = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);

  if (mZip == greOmni || mZip == appOmni) {
    mZip = new nsZipArchive();
    return NS_OK;
  }
  return mZip->CloseArchive();
}

NS_IMETHODIMP
nsJAR::Test(const nsACString &aEntryName)
{
  return mZip->Test(aEntryName.IsEmpty()? nullptr : PromiseFlatCString(aEntryName).get());
}

NS_IMETHODIMP
nsJAR::Extract(const nsACString &aEntryName, nsIFile* outFile)
{
  
  
  MutexAutoLock lock(mLock);

  nsZipItem *item = mZip->GetItem(PromiseFlatCString(aEntryName).get());
  NS_ENSURE_TRUE(item, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

  
  
  

  nsresult rv = outFile->Remove(false);
  if (rv == NS_ERROR_FILE_DIR_NOT_EMPTY ||
      rv == NS_ERROR_FAILURE)
    return rv;

  if (item->IsDirectory())
  {
    rv = outFile->Create(nsIFile::DIRECTORY_TYPE, item->Mode());
    
    
    
  }
  else
  {
    PRFileDesc* fd;
    rv = outFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE, item->Mode(), &fd);
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString path;
    rv = outFile->GetNativePath(path);
    if (NS_FAILED(rv)) return rv;

    rv = mZip->ExtractFile(item, path.get(), fd);
  }
  if (NS_FAILED(rv)) return rv;

  
  
  outFile->SetLastModifiedTime(item->LastModTime() / PR_USEC_PER_MSEC);

  return NS_OK;
}

NS_IMETHODIMP
nsJAR::GetEntry(const nsACString &aEntryName, nsIZipEntry* *result)
{
  nsZipItem* zipItem = mZip->GetItem(PromiseFlatCString(aEntryName).get());
  NS_ENSURE_TRUE(zipItem, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

  nsJARItem* jarItem = new nsJARItem(zipItem);

  NS_ADDREF(*result = jarItem);
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::HasEntry(const nsACString &aEntryName, bool *result)
{
  *result = mZip->GetItem(PromiseFlatCString(aEntryName).get()) != nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::FindEntries(const nsACString &aPattern, nsIUTF8StringEnumerator **result)
{
  NS_ENSURE_ARG_POINTER(result);

  nsZipFind *find;
  nsresult rv = mZip->FindInit(aPattern.IsEmpty()? nullptr : PromiseFlatCString(aPattern).get(), &find);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIUTF8StringEnumerator *zipEnum = new nsJAREnumerator(find);

  NS_ADDREF(*result = zipEnum);
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::GetInputStream(const nsACString &aFilename, nsIInputStream** result)
{
  return GetInputStreamWithSpec(EmptyCString(), aFilename, result);
}

NS_IMETHODIMP
nsJAR::GetInputStreamWithSpec(const nsACString& aJarDirSpec,
                          const nsACString &aEntryName, nsIInputStream** result)
{
  NS_ENSURE_ARG_POINTER(result);

  
  nsZipItem *item = nullptr;
  const char *entry = PromiseFlatCString(aEntryName).get();
  if (*entry) {
    
    item = mZip->GetItem(entry);
    if (!item) return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
  }
  nsJARInputStream* jis = new nsJARInputStream();
  
  NS_ADDREF(*result = jis);

  nsresult rv = NS_OK;
  if (!item || item->IsDirectory()) {
    rv = jis->InitDirectory(this, aJarDirSpec, entry);
  } else {
    rv = jis->InitFile(this, item);
  }
  if (NS_FAILED(rv)) {
    NS_RELEASE(*result);
  }
  return rv;
}

NS_IMETHODIMP
nsJAR::GetSigningCert(const nsACString& aFilename, nsIX509Cert** aSigningCert)
{
  
  if (!aSigningCert) {
    return NS_ERROR_NULL_POINTER;
  }
  *aSigningCert = nullptr;

  
  
  nsRefPtr<nsZipArchive> greOmni = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  nsRefPtr<nsZipArchive> appOmni = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);

  if (mZip == greOmni || mZip == appOmni)
    return NS_OK;

  
  nsresult rv = ParseManifest();
  if (NS_FAILED(rv)) return rv;
  if (mGlobalStatus == JAR_NO_MANIFEST)
    return NS_OK;

  int16_t requestedStatus;
  if (!aFilename.IsEmpty())
  {
    
    nsJARManifestItem* manItem = mManifestData.Get(aFilename);
    if (!manItem)
      return NS_OK;
    
    if (!manItem->entryVerified)
    {
      nsXPIDLCString entryData;
      uint32_t entryDataLen;
      rv = LoadEntry(aFilename, getter_Copies(entryData), &entryDataLen);
      if (NS_FAILED(rv)) return rv;
      rv = VerifyEntry(manItem, entryData, entryDataLen);
      if (NS_FAILED(rv)) return rv;
    }
    requestedStatus = manItem->status;
  }
  else 
    requestedStatus = mGlobalStatus;

  if (requestedStatus != JAR_VALID_MANIFEST) {
    ReportError(aFilename, requestedStatus);
  } else { 
    *aSigningCert = mSigningCert;
    NS_IF_ADDREF(*aSigningCert);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::GetManifestEntriesCount(uint32_t* count)
{
  *count = mTotalItemsInManifest;
  return NS_OK;
}

nsresult
nsJAR::GetJarPath(nsACString& aResult)
{
  NS_ENSURE_ARG_POINTER(mZipFile);

  return mZipFile->GetNativePath(aResult);
}

nsresult
nsJAR::GetNSPRFileDesc(PRFileDesc** aNSPRFileDesc)
{
  if (!aNSPRFileDesc) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  *aNSPRFileDesc = nullptr;

  if (!mZip) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsZipHandle> handle = mZip->GetFD();
  if (!handle) {
    return NS_ERROR_FAILURE;
  }

  return handle->GetNSPRFileDesc(aNSPRFileDesc);
}




nsresult
nsJAR::LoadEntry(const nsACString &aFilename, char** aBuf, uint32_t* aBufLen)
{
  
  nsresult rv;
  nsCOMPtr<nsIInputStream> manifestStream;
  rv = GetInputStream(aFilename, getter_AddRefs(manifestStream));
  if (NS_FAILED(rv)) return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;

  
  char* buf;
  uint64_t len64;
  rv = manifestStream->Available(&len64);
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(len64 < UINT32_MAX, NS_ERROR_FILE_CORRUPTED); 
  uint32_t len = (uint32_t)len64;
  buf = (char*)malloc(len+1);
  if (!buf) return NS_ERROR_OUT_OF_MEMORY;
  uint32_t bytesRead;
  rv = manifestStream->Read(buf, len, &bytesRead);
  if (bytesRead != len)
    rv = NS_ERROR_FILE_CORRUPTED;
  if (NS_FAILED(rv)) {
    free(buf);
    return rv;
  }
  buf[len] = '\0'; 
  *aBuf = buf;
  if (aBufLen)
    *aBufLen = len;
  return NS_OK;
}


int32_t
nsJAR::ReadLine(const char** src)
{
  if (!*src) {
    return 0;
  }

  
  
  int32_t length;
  char* eol = PL_strpbrk(*src, "\r\n");

  if (eol == nullptr) 
  {
    length = strlen(*src);
    if (length == 0) 
      *src = nullptr;
    else             
      *src += length;
  }
  else
  {
    length = eol - *src;
    if (eol[0] == '\r' && eol[1] == '\n')      
      *src = eol+2;
    else                                       
      *src = eol+1;
  }
  return length;
}



#define JAR_MF 1
#define JAR_SF 2
#define JAR_MF_SEARCH_STRING "(M|/M)ETA-INF/(M|m)(ANIFEST|anifest).(MF|mf)$"
#define JAR_SF_SEARCH_STRING "(M|/M)ETA-INF/*.(SF|sf)$"
#define JAR_MF_HEADER (const char*)"Manifest-Version: 1.0"
#define JAR_SF_HEADER (const char*)"Signature-Version: 1.0"

nsresult
nsJAR::ParseManifest()
{
  
  if (mParsedManifest)
    return NS_OK;
  
  nsCOMPtr<nsIUTF8StringEnumerator> files;
  nsresult rv = FindEntries(nsDependentCString(JAR_MF_SEARCH_STRING), getter_AddRefs(files));
  if (!files) rv = NS_ERROR_FAILURE;
  if (NS_FAILED(rv)) return rv;

  
  bool more;
  rv = files->HasMore(&more);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!more)
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = true;
    return NS_OK;
  }

  nsAutoCString manifestFilename;
  rv = files->GetNext(manifestFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = files->HasMore(&more);
  if (NS_FAILED(rv)) return rv;
  if (more)
  {
    mParsedManifest = true;
    return NS_ERROR_FILE_CORRUPTED; 
  }

  nsXPIDLCString manifestBuffer;
  uint32_t manifestLen;
  rv = LoadEntry(manifestFilename, getter_Copies(manifestBuffer), &manifestLen);
  if (NS_FAILED(rv)) return rv;

  
  rv = ParseOneFile(manifestBuffer, JAR_MF);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = FindEntries(nsDependentCString(JAR_SF_SEARCH_STRING), getter_AddRefs(files));
  if (!files) rv = NS_ERROR_FAILURE;
  if (NS_FAILED(rv)) return rv;
  
  rv = files->HasMore(&more);
  if (NS_FAILED(rv)) return rv;
  if (!more)
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = true;
    return NS_OK;
  }
  rv = files->GetNext(manifestFilename);
  if (NS_FAILED(rv)) return rv;

  rv = LoadEntry(manifestFilename, getter_Copies(manifestBuffer), &manifestLen);
  if (NS_FAILED(rv)) return rv;

  
  nsAutoCString sigFilename(manifestFilename);
  int32_t extension = sigFilename.RFindChar('.') + 1;
  NS_ASSERTION(extension != 0, "Manifest Parser: Missing file extension.");
  (void)sigFilename.Cut(extension, 2);
  nsXPIDLCString sigBuffer;
  uint32_t sigLen;
  {
    nsAutoCString tempFilename(sigFilename); tempFilename.Append("rsa", 3);
    rv = LoadEntry(tempFilename, getter_Copies(sigBuffer), &sigLen);
  }
  if (NS_FAILED(rv))
  {
    nsAutoCString tempFilename(sigFilename); tempFilename.Append("RSA", 3);
    rv = LoadEntry(tempFilename, getter_Copies(sigBuffer), &sigLen);
  }
  if (NS_FAILED(rv))
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = true;
    return NS_OK;
  }

  
  nsCOMPtr<nsIDataSignatureVerifier> verifier(
    do_GetService("@mozilla.org/security/datasignatureverifier;1", &rv));
  if (NS_FAILED(rv)) 
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = true;
    return NS_OK;
  }

  
  int32_t verifyError;
  rv = verifier->VerifySignature(sigBuffer, sigLen, manifestBuffer, manifestLen,
                                 &verifyError, getter_AddRefs(mSigningCert));
  if (NS_FAILED(rv)) return rv;
  if (mSigningCert && verifyError == nsIDataSignatureVerifier::VERIFY_OK) {
    mGlobalStatus = JAR_VALID_MANIFEST;
  } else if (verifyError == nsIDataSignatureVerifier::VERIFY_ERROR_UNKNOWN_ISSUER) {
    mGlobalStatus = JAR_INVALID_UNKNOWN_CA;
  } else {
    mGlobalStatus = JAR_INVALID_SIG;
  }

  
  
  
  
  ParseOneFile(manifestBuffer, JAR_SF);
  mParsedManifest = true;

  return NS_OK;
}

nsresult
nsJAR::ParseOneFile(const char* filebuf, int16_t aFileType)
{
  
  const char* nextLineStart = filebuf;
  nsAutoCString curLine;
  int32_t linelen;
  linelen = ReadLine(&nextLineStart);
  curLine.Assign(filebuf, linelen);

  if ( ((aFileType == JAR_MF) && !curLine.Equals(JAR_MF_HEADER) ) ||
       ((aFileType == JAR_SF) && !curLine.Equals(JAR_SF_HEADER) ) )
     return NS_ERROR_FILE_CORRUPTED;

  
  do {
    linelen = ReadLine(&nextLineStart);
  } while (linelen > 0);

  
  const char* curPos;
  const char* sectionStart = nextLineStart;

  nsJARManifestItem* curItemMF = nullptr;
  bool foundName = false;
  if (aFileType == JAR_MF) {
    curItemMF = new nsJARManifestItem();
  }

  nsAutoCString curItemName;
  nsAutoCString storedSectionDigest;

  for(;;)
  {
    curPos = nextLineStart;
    linelen = ReadLine(&nextLineStart);
    curLine.Assign(curPos, linelen);
    if (linelen == 0)
    
    {
      if (aFileType == JAR_MF)
      {
        mTotalItemsInManifest++;
        if (curItemMF->mType != JAR_INVALID)
        {
          
          if(!foundName)
            curItemMF->mType = JAR_INVALID;
          else
          {
            
            
            if (curItemMF->mType == JAR_INTERNAL)
            {
              bool exists;
              nsresult rv = HasEntry(curItemName, &exists);
              if (NS_FAILED(rv) || !exists)
                curItemMF->mType = JAR_INVALID;
            }
            
            if (mManifestData.Contains(curItemName)) {
              curItemMF->mType = JAR_INVALID;
            }
          }
        }

        if (curItemMF->mType == JAR_INVALID)
          delete curItemMF;
        else 
        {
          uint32_t sectionLength = curPos - sectionStart;
          CalculateDigest(sectionStart, sectionLength,
                          curItemMF->calculatedSectionDigest);
          
          mManifestData.Put(curItemName, curItemMF);
        }
        if (nextLineStart == nullptr) 
          break;

        sectionStart = nextLineStart;
        curItemMF = new nsJARManifestItem();
      } 
      else
        
        
      {
        if (foundName)
        {
          nsJARManifestItem* curItemSF = mManifestData.Get(curItemName);
          if(curItemSF)
          {
            NS_ASSERTION(curItemSF->status == JAR_NOT_SIGNED,
                         "SECURITY ERROR: nsJARManifestItem not correctly initialized");
            curItemSF->status = mGlobalStatus;
            if (curItemSF->status == JAR_VALID_MANIFEST)
            { 
              if (storedSectionDigest.IsEmpty())
                curItemSF->status = JAR_NOT_SIGNED;
              else
              {
                if (!storedSectionDigest.Equals(curItemSF->calculatedSectionDigest))
                  curItemSF->status = JAR_INVALID_MANIFEST;
                curItemSF->calculatedSectionDigest.Truncate();
                storedSectionDigest.Truncate();
              }
            } 
          } 
        } 

        if(nextLineStart == nullptr) 
          break;
      } 
      foundName = false;
      continue;
    } 

    
    
    while(*nextLineStart == ' ')
    {
      curPos = nextLineStart;
      int32_t continuationLen = ReadLine(&nextLineStart) - 1;
      nsAutoCString continuation(curPos+1, continuationLen);
      curLine += continuation;
      linelen += continuationLen;
    }

    
    int32_t colonPos = curLine.FindChar(':');
    if (colonPos == -1)    
      continue;
    
    nsAutoCString lineName;
    curLine.Left(lineName, colonPos);
    nsAutoCString lineData;
    curLine.Mid(lineData, colonPos+2, linelen - (colonPos+2));

    
    
    if (lineName.LowerCaseEqualsLiteral("sha1-digest"))
    
    {
      if(aFileType == JAR_MF)
        curItemMF->storedEntryDigest = lineData;
      else
        storedSectionDigest = lineData;
      continue;
    }

    
    if (!foundName && lineName.LowerCaseEqualsLiteral("name"))
    {
      curItemName = lineData;
      foundName = true;
      continue;
    }

    
    
    if (aFileType == JAR_MF && lineName.LowerCaseEqualsLiteral("magic"))
    {
      if (lineData.LowerCaseEqualsLiteral("javascript"))
        curItemMF->mType = JAR_EXTERNAL;
      else
        curItemMF->mType = JAR_INVALID;
      continue;
    }

  } 
  return NS_OK;
} 

nsresult
nsJAR::VerifyEntry(nsJARManifestItem* aManItem, const char* aEntryData,
                   uint32_t aLen)
{
  if (aManItem->status == JAR_VALID_MANIFEST)
  {
    if (aManItem->storedEntryDigest.IsEmpty())
      
      aManItem->status = JAR_NOT_SIGNED;
    else
    { 
      nsCString calculatedEntryDigest;
      nsresult rv = CalculateDigest(aEntryData, aLen, calculatedEntryDigest);
      if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
      if (!aManItem->storedEntryDigest.Equals(calculatedEntryDigest))
        aManItem->status = JAR_INVALID_ENTRY;
      aManItem->storedEntryDigest.Truncate();
    }
  }
  aManItem->entryVerified = true;
  return NS_OK;
}

void nsJAR::ReportError(const nsACString &aFilename, int16_t errorCode)
{
  
  nsAutoString message;
  message.AssignLiteral("Signature Verification Error: the signature on ");
  if (!aFilename.IsEmpty())
    AppendASCIItoUTF16(aFilename, message);
  else
    message.AppendLiteral("this .jar archive");
  message.AppendLiteral(" is invalid because ");
  switch(errorCode)
  {
  case JAR_NOT_SIGNED:
    message.AppendLiteral("the archive did not contain a valid PKCS7 signature.");
    break;
  case JAR_INVALID_SIG:
    message.AppendLiteral("the digital signature (*.RSA) file is not a valid signature of the signature instruction file (*.SF).");
    break;
  case JAR_INVALID_UNKNOWN_CA:
    message.AppendLiteral("the certificate used to sign this file has an unrecognized issuer.");
    break;
  case JAR_INVALID_MANIFEST:
    message.AppendLiteral("the signature instruction file (*.SF) does not contain a valid hash of the MANIFEST.MF file.");
    break;
  case JAR_INVALID_ENTRY:
    message.AppendLiteral("the MANIFEST.MF file does not contain a valid hash of the file being verified.");
    break;
  case JAR_NO_MANIFEST:
    message.AppendLiteral("the archive did not contain a manifest.");
    break;
  default:
    message.AppendLiteral("of an unknown problem.");
  }

  
  nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));
  if (console)
  {
    console->LogStringMessage(message.get());
  }
#ifdef DEBUG
  char* messageCstr = ToNewCString(message);
  if (!messageCstr) return;
  fprintf(stderr, "%s\n", messageCstr);
  nsMemory::Free(messageCstr);
#endif
}


nsresult nsJAR::CalculateDigest(const char* aInBuf, uint32_t aLen,
                                nsCString& digest)
{
  nsresult rv;

  nsCOMPtr<nsICryptoHash> hasher = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = hasher->Init(nsICryptoHash::SHA1);
  if (NS_FAILED(rv)) return rv;

  rv = hasher->Update((const uint8_t*) aInBuf, aLen);
  if (NS_FAILED(rv)) return rv;

  return hasher->Finish(true, digest);
}

NS_IMPL_ISUPPORTS(nsJAREnumerator, nsIUTF8StringEnumerator)




NS_IMETHODIMP
nsJAREnumerator::HasMore(bool* aResult)
{
    
    if (!mName) {
        NS_ASSERTION(mFind, "nsJAREnumerator: Missing zipFind.");
        nsresult rv = mFind->FindNext( &mName, &mNameLen );
        if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
            *aResult = false;                    
            return NS_OK;
        }
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);    
    }

    *aResult = true;
    return NS_OK;
}




NS_IMETHODIMP
nsJAREnumerator::GetNext(nsACString& aResult)
{
    
    if (!mName) {
        bool     bMore;
        nsresult rv = HasMore(&bMore);
        if (NS_FAILED(rv) || !bMore)
            return NS_ERROR_FAILURE; 
    }
    aResult.Assign(mName, mNameLen);
    mName = 0; 
    return NS_OK;
}


NS_IMPL_ISUPPORTS(nsJARItem, nsIZipEntry)

nsJARItem::nsJARItem(nsZipItem* aZipItem)
    : mSize(aZipItem->Size()),
      mRealsize(aZipItem->RealSize()),
      mCrc32(aZipItem->CRC32()),
      mLastModTime(aZipItem->LastModTime()),
      mCompression(aZipItem->Compression()),
      mPermissions(aZipItem->Mode()),
      mIsDirectory(aZipItem->IsDirectory()),
      mIsSynthetic(aZipItem->isSynthetic)
{
}




NS_IMETHODIMP
nsJARItem::GetCompression(uint16_t *aCompression)
{
    NS_ENSURE_ARG_POINTER(aCompression);

    *aCompression = mCompression;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetSize(uint32_t *aSize)
{
    NS_ENSURE_ARG_POINTER(aSize);

    *aSize = mSize;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetRealSize(uint32_t *aRealsize)
{
    NS_ENSURE_ARG_POINTER(aRealsize);

    *aRealsize = mRealsize;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetCRC32(uint32_t *aCrc32)
{
    NS_ENSURE_ARG_POINTER(aCrc32);

    *aCrc32 = mCrc32;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetIsDirectory(bool *aIsDirectory)
{
    NS_ENSURE_ARG_POINTER(aIsDirectory);

    *aIsDirectory = mIsDirectory;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetIsSynthetic(bool *aIsSynthetic)
{
    NS_ENSURE_ARG_POINTER(aIsSynthetic);

    *aIsSynthetic = mIsSynthetic;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetLastModifiedTime(PRTime* aLastModTime)
{
    NS_ENSURE_ARG_POINTER(aLastModTime);

    *aLastModTime = mLastModTime;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetPermissions(uint32_t* aPermissions)
{
    NS_ENSURE_ARG_POINTER(aPermissions);

    *aPermissions = mPermissions;
    return NS_OK;
}




NS_IMPL_ISUPPORTS(nsZipReaderCache, nsIZipReaderCache, nsIObserver, nsISupportsWeakReference)

nsZipReaderCache::nsZipReaderCache()
  : mLock("nsZipReaderCache.mLock")
  , mZips()
  , mMustCacheFd(false)
#ifdef ZIP_CACHE_HIT_RATE
    ,
    mZipCacheLookups(0),
    mZipCacheHits(0),
    mZipCacheFlushes(0),
    mZipSyncMisses(0)
#endif
{
}

NS_IMETHODIMP
nsZipReaderCache::Init(uint32_t cacheSize)
{
  mCacheSize = cacheSize;


  nsCOMPtr<nsIObserverService> os =
           do_GetService("@mozilla.org/observer-service;1");
  if (os)
  {
    os->AddObserver(this, "memory-pressure", true);
    os->AddObserver(this, "chrome-flush-caches", true);
    os->AddObserver(this, "flush-cache-entry", true);
  }


  return NS_OK;
}

static PLDHashOperator
DropZipReaderCache(const nsACString &aKey, nsJAR* aZip, void*)
{
  aZip->SetZipReaderCache(nullptr);
  return PL_DHASH_NEXT;
}

nsZipReaderCache::~nsZipReaderCache()
{
  mZips.EnumerateRead(DropZipReaderCache, nullptr);

#ifdef ZIP_CACHE_HIT_RATE
  printf("nsZipReaderCache size=%d hits=%d lookups=%d rate=%f%% flushes=%d missed %d\n",
         mCacheSize, mZipCacheHits, mZipCacheLookups,
         (float)mZipCacheHits / mZipCacheLookups,
         mZipCacheFlushes, mZipSyncMisses);
#endif
}

NS_IMETHODIMP
nsZipReaderCache::IsCached(nsIFile* zipFile, bool* aResult)
{
  NS_ENSURE_ARG_POINTER(zipFile);
  nsresult rv;
  nsCOMPtr<nsIZipReader> antiLockZipGrip;
  MutexAutoLock lock(mLock);

  nsAutoCString uri;
  rv = zipFile->GetNativePath(uri);
  if (NS_FAILED(rv))
    return rv;

  uri.Insert(NS_LITERAL_CSTRING("file:"), 0);

  *aResult = mZips.Contains(uri);
  return NS_OK;
}

NS_IMETHODIMP
nsZipReaderCache::GetZip(nsIFile* zipFile, nsIZipReader* *result)
{
  NS_ENSURE_ARG_POINTER(zipFile);
  nsresult rv;
  nsCOMPtr<nsIZipReader> antiLockZipGrip;
  MutexAutoLock lock(mLock);

#ifdef ZIP_CACHE_HIT_RATE
  mZipCacheLookups++;
#endif

  nsAutoCString uri;
  rv = zipFile->GetNativePath(uri);
  if (NS_FAILED(rv)) return rv;

  uri.Insert(NS_LITERAL_CSTRING("file:"), 0);

  nsRefPtr<nsJAR> zip;
  mZips.Get(uri, getter_AddRefs(zip));
  if (zip) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheHits++;
#endif
    zip->ClearReleaseTime();
  } else {
    zip = new nsJAR();
    zip->SetZipReaderCache(this);
    rv = zip->Open(zipFile);
    if (NS_FAILED(rv)) {
      return rv;
    }

    MOZ_ASSERT(!mZips.Contains(uri));
    mZips.Put(uri, zip);
  }
  zip.forget(result);
  return rv;
}

NS_IMETHODIMP
nsZipReaderCache::GetInnerZip(nsIFile* zipFile, const nsACString &entry,
                              nsIZipReader* *result)
{
  NS_ENSURE_ARG_POINTER(zipFile);

  nsCOMPtr<nsIZipReader> outerZipReader;
  nsresult rv = GetZip(zipFile, getter_AddRefs(outerZipReader));
  NS_ENSURE_SUCCESS(rv, rv);

  MutexAutoLock lock(mLock);

#ifdef ZIP_CACHE_HIT_RATE
  mZipCacheLookups++;
#endif

  nsAutoCString uri;
  rv = zipFile->GetNativePath(uri);
  if (NS_FAILED(rv)) return rv;

  uri.Insert(NS_LITERAL_CSTRING("jar:"), 0);
  uri.AppendLiteral("!/");
  uri.Append(entry);

  nsRefPtr<nsJAR> zip;
  mZips.Get(uri, getter_AddRefs(zip));
  if (zip) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheHits++;
#endif
    zip->ClearReleaseTime();
  } else {
    zip = new nsJAR();
    zip->SetZipReaderCache(this);

    rv = zip->OpenInner(outerZipReader, entry);
    if (NS_FAILED(rv)) {
      return rv;
    }

    MOZ_ASSERT(!mZips.Contains(uri));
    mZips.Put(uri, zip);
  }
  zip.forget(result);
  return rv;
}

NS_IMETHODIMP
nsZipReaderCache::SetMustCacheFd(nsIFile* zipFile, bool aMustCacheFd)
{
#if defined(XP_WIN)
  MOZ_CRASH("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
#else

  if (!aMustCacheFd) {
    return NS_OK;
  }

  if (!zipFile) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsAutoCString uri;
  rv = zipFile->GetNativePath(uri);
  if (NS_FAILED(rv)) {
    return rv;
  }
  uri.Insert(NS_LITERAL_CSTRING("file:"), 0);

  MutexAutoLock lock(mLock);

  mMustCacheFd = aMustCacheFd;

  nsRefPtr<nsJAR> zip;
  mZips.Get(uri, getter_AddRefs(zip));
  if (!zip) {
    return NS_ERROR_FAILURE;
  }

  
  PRFileDesc* fd = nullptr;
  zip->GetNSPRFileDesc(&fd);
  if (!fd) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheFlushes++;
#endif
    zip->SetZipReaderCache(nullptr);
    mZips.Remove(uri);
  }
  return NS_OK;
#endif 
}

NS_IMETHODIMP
nsZipReaderCache::GetFd(nsIFile* zipFile, PRFileDesc** aRetVal)
{
#if defined(XP_WIN)
  MOZ_CRASH("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
#else
  if (!zipFile) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsAutoCString uri;
  rv = zipFile->GetNativePath(uri);
  if (NS_FAILED(rv)) {
    return rv;
  }
  uri.Insert(NS_LITERAL_CSTRING("file:"), 0);

  MutexAutoLock lock(mLock);
  nsRefPtr<nsJAR> zip;
  mZips.Get(uri, getter_AddRefs(zip));
  if (!zip) {
    return NS_ERROR_FAILURE;
  }

  zip->ClearReleaseTime();
  rv = zip->GetNSPRFileDesc(aRetVal);
  
  MutexAutoUnlock unlock(mLock);
  nsRefPtr<nsJAR> zipTemp = zip.forget();
  return rv;
#endif 
}

static PLDHashOperator
FindOldestZip(const nsACString &aKey, nsJAR* aZip, void* aClosure)
{
  nsJAR** oldestPtr = static_cast<nsJAR**>(aClosure);
  nsJAR* oldest = *oldestPtr;
  nsJAR* current = aZip;
  PRIntervalTime currentReleaseTime = current->GetReleaseTime();
  if (currentReleaseTime != PR_INTERVAL_NO_TIMEOUT) {
    if (oldest == nullptr ||
        currentReleaseTime < oldest->GetReleaseTime()) {
      *oldestPtr = current;
    }
  }
  return PL_DHASH_NEXT;
}

struct ZipFindData {nsJAR* zip; bool found;};

static PLDHashOperator
FindZip(const nsACString &aKey, nsJAR* aZip, void* aClosure)
{
  ZipFindData* find_data = static_cast<ZipFindData*>(aClosure);

  if (find_data->zip == aZip) {
    find_data->found = true;
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

nsresult
nsZipReaderCache::ReleaseZip(nsJAR* zip)
{
  nsresult rv;
  MutexAutoLock lock(mLock);

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  ZipFindData find_data = {zip, false};
  mZips.EnumerateRead(FindZip, &find_data);
  if (!find_data.found) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipSyncMisses++;
#endif
    return NS_OK;
  }

  zip->SetReleaseTime();

  if (mZips.Count() <= mCacheSize)
    return NS_OK;

  nsJAR* oldest = nullptr;
  mZips.EnumerateRead(FindOldestZip, &oldest);

  
  
  if (!oldest)
    return NS_OK;

#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheFlushes++;
#endif

  
  nsAutoCString uri;
  rv = oldest->GetJarPath(uri);
  if (NS_FAILED(rv))
    return rv;

  if (oldest->mOuterZipEntry.IsEmpty()) {
    uri.Insert(NS_LITERAL_CSTRING("file:"), 0);
  } else {
    uri.Insert(NS_LITERAL_CSTRING("jar:"), 0);
    uri.AppendLiteral("!/");
    uri.Append(oldest->mOuterZipEntry);
  }

  
  
  
  nsRefPtr<nsJAR> removed;
  mZips.Remove(uri, getter_AddRefs(removed));
  NS_ASSERTION(removed, "botched");
  NS_ASSERTION(oldest == removed, "removed wrong entry");

  if (removed)
    removed->SetZipReaderCache(nullptr);

  return NS_OK;
}

static PLDHashOperator
FindFlushableZip(const nsACString &aKey, nsRefPtr<nsJAR>& aCurrent, void*)
{
  if (aCurrent->GetReleaseTime() != PR_INTERVAL_NO_TIMEOUT) {
    aCurrent->SetZipReaderCache(nullptr);
    return PL_DHASH_REMOVE;
  }
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsZipReaderCache::Observe(nsISupports *aSubject,
                          const char *aTopic,
                          const char16_t *aSomeData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    MutexAutoLock lock(mLock);
    mZips.Enumerate(FindFlushableZip, nullptr);
  }
  else if (strcmp(aTopic, "chrome-flush-caches") == 0) {
    MutexAutoLock lock(mLock);
    mZips.EnumerateRead(DropZipReaderCache, nullptr);
    mZips.Clear();
  }
  else if (strcmp(aTopic, "flush-cache-entry") == 0) {
    nsCOMPtr<nsIFile> file = do_QueryInterface(aSubject);
    if (!file)
      return NS_OK;

    nsAutoCString uri;
    if (NS_FAILED(file->GetNativePath(uri)))
      return NS_OK;

    uri.Insert(NS_LITERAL_CSTRING("file:"), 0);

    MutexAutoLock lock(mLock);

    nsRefPtr<nsJAR> zip;
    mZips.Get(uri, getter_AddRefs(zip));
    if (!zip)
      return NS_OK;

#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheFlushes++;
#endif

    zip->SetZipReaderCache(nullptr);

    mZips.Remove(uri);
  }
  return NS_OK;
}


