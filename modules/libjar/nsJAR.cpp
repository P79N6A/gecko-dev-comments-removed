









































#include <string.h>
#include "nsJARInputStream.h"
#include "nsJAR.h"
#include "nsILocalFile.h"
#include "nsIConsoleService.h"
#include "nsICryptoHash.h"
#include "prprf.h"

#ifdef XP_UNIX
  #include <sys/stat.h>
#elif defined (XP_WIN) || defined(XP_OS2)
  #include <io.h>
#endif











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

  
  
  PRBool              entryVerified;
  
  
  PRInt16             status;
  
  
  nsCString           calculatedSectionDigest;
  nsCString           storedEntryDigest;

  nsJARManifestItem();
  virtual ~nsJARManifestItem();
};




nsJARManifestItem::nsJARManifestItem(): mType(JAR_INTERNAL),
                                        entryVerified(PR_FALSE),
                                        status(JAR_NOT_SIGNED)
{
}

nsJARManifestItem::~nsJARManifestItem()
{
}




static PRBool
DeleteManifestEntry(nsHashKey* aKey, void* aData, void* closure)
{

  delete (nsJARManifestItem*)aData;
  return PR_TRUE;
}


nsJAR::nsJAR(): mManifestData(nsnull, nsnull, DeleteManifestEntry, nsnull, 10),
                mParsedManifest(PR_FALSE),
                mGlobalStatus(JAR_MANIFEST_NOT_PARSED),
                mReleaseTime(PR_INTERVAL_NO_TIMEOUT), 
                mCache(nsnull), 
                mLock(nsnull),
                mTotalItemsInManifest(0)
{
}

nsJAR::~nsJAR()
{
  Close();
}

NS_IMPL_THREADSAFE_QUERY_INTERFACE2(nsJAR, nsIZipReader, nsIJAR)
NS_IMPL_THREADSAFE_ADDREF(nsJAR)


nsrefcnt nsJAR::Release(void) 
{
  nsrefcnt count; 
  NS_PRECONDITION(0 != mRefCnt, "dup release"); 
  count = PR_AtomicDecrement((PRInt32 *)&mRefCnt); 
  NS_LOG_RELEASE(this, count, "nsJAR"); 
  if (0 == count) {
    mRefCnt = 1;  
     
     
    NS_DELETEXPCOM(this); 
    return 0; 
  }
  else if (1 == count && mCache) {
    nsresult rv = mCache->ReleaseZip(this);
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to release zip file");
  }
  return count; 
} 





NS_IMETHODIMP
nsJAR::Open(nsIFile* zipFile)
{
  NS_ENSURE_ARG_POINTER(zipFile);
  if (mLock) return NS_ERROR_FAILURE; 

  mZipFile = zipFile;

  mLock = PR_NewLock();
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  PRFileDesc *fd = OpenFile();
  NS_ENSURE_TRUE(fd, NS_ERROR_FAILURE);

  nsresult rv = mZip.OpenArchive(fd);
  if (NS_FAILED(rv)) Close();

  return rv;
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
  if (mLock) {
    PR_DestroyLock(mLock);
    mLock = nsnull;
  }

  mParsedManifest = PR_FALSE;
  mManifestData.Reset();
  mGlobalStatus = JAR_MANIFEST_NOT_PARSED;
  mTotalItemsInManifest = 0;

  return mZip.CloseArchive();
}

NS_IMETHODIMP
nsJAR::Test(const char *aEntryName)
{
  return mZip.Test(aEntryName);
}

NS_IMETHODIMP
nsJAR::Extract(const char *zipEntry, nsIFile* outFile)
{
  
  
  nsAutoLock lock(mLock);

  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(outFile, &rv);
  if (NS_FAILED(rv)) return rv;

  nsZipItem *item = mZip.GetItem(zipEntry);
  NS_ENSURE_TRUE(item, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

  
  
  

  
  
  
  
  
  rv = localFile->Remove(PR_FALSE);
  if (rv == NS_ERROR_FILE_DIR_NOT_EMPTY ||
      rv == NS_ERROR_FAILURE)
    return rv;

  if (item->isDirectory)
  {
    rv = localFile->Create(nsIFile::DIRECTORY_TYPE, item->mode);
    
    
    
  }
  else
  {
    PRFileDesc* fd;
    rv = localFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE, item->mode, &fd);
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString path;
    rv = outFile->GetNativePath(path);
    if (NS_FAILED(rv)) return rv;

    rv = mZip.ExtractFile(item, path.get(), fd);
  }
  if (NS_FAILED(rv)) return rv;

  PRTime prtime = GetModTime(item->date, item->time);
  
  PRTime conversion = LL_ZERO;
  PRTime newTime = LL_ZERO;
  LL_I2L(conversion, PR_USEC_PER_MSEC);
  LL_DIV(newTime, prtime, conversion);
  
  outFile->SetLastModifiedTime(newTime);

  return NS_OK;
}

NS_IMETHODIMP    
nsJAR::GetEntry(const char *aEntryName, nsIZipEntry* *result)
{
  nsZipItem* zipItem = mZip.GetItem(aEntryName);
  NS_ENSURE_TRUE(zipItem, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

  nsJARItem* jarItem = new nsJARItem(zipItem);
  NS_ENSURE_TRUE(jarItem, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*result = jarItem);
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::HasEntry(const nsACString &aEntryName, PRBool *result)
{
  *result = mZip.GetItem(PromiseFlatCString(aEntryName).get()) != nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::FindEntries(const char *aPattern, nsIUTF8StringEnumerator **result)
{
  NS_ENSURE_ARG_POINTER(result);

  nsZipFind *find;
  nsresult rv = mZip.FindInit(aPattern, &find);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIUTF8StringEnumerator *zipEnum = new nsJAREnumerator(find);
  if (!zipEnum) {
    delete find;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*result = zipEnum);
  return NS_OK;
}

NS_IMETHODIMP
nsJAR::GetInputStream(const char* aFilename, nsIInputStream** result)
{
  return GetInputStreamWithSpec(EmptyCString(), aFilename, result);
}

NS_IMETHODIMP
nsJAR::GetInputStreamWithSpec(const nsACString& aJarDirSpec, 
                          const char* aEntryName, nsIInputStream** result)
{
  NS_ENSURE_ARG_POINTER(aEntryName);
  NS_ENSURE_ARG_POINTER(result);

  
  nsZipItem *item = nsnull;
  if (*aEntryName) {
    
    item = mZip.GetItem(aEntryName);
    if (!item) return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
  }
  nsJARInputStream* jis = new nsJARInputStream();
  
  NS_ENSURE_TRUE(jis, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*result = jis);

  nsresult rv = NS_OK;
  if (!item || item->isDirectory) {
    rv = jis->InitDirectory(this, aJarDirSpec, aEntryName);
  } else {
    rv = jis->InitFile(this, item);
  }
  if (NS_FAILED(rv)) {
    NS_RELEASE(*result);
  }
  return rv;
}





NS_IMETHODIMP
nsJAR::GetCertificatePrincipal(const char* aFilename, nsIPrincipal** aPrincipal)
{
  
  if (!aPrincipal)
    return NS_ERROR_NULL_POINTER;
  *aPrincipal = nsnull;

  
  nsresult rv = ParseManifest();
  if (NS_FAILED(rv)) return rv;
  if (mGlobalStatus == JAR_NO_MANIFEST)
    return NS_OK;

  PRInt16 requestedStatus;
  if (aFilename)
  {
    
    nsCStringKey key(aFilename);
    nsJARManifestItem* manItem = static_cast<nsJARManifestItem*>(mManifestData.Get(&key));
    if (!manItem)
      return NS_OK;
    
    if (!manItem->entryVerified)
    {
      nsXPIDLCString entryData;
      PRUint32 entryDataLen;
      rv = LoadEntry(aFilename, getter_Copies(entryData), &entryDataLen);
      if (NS_FAILED(rv)) return rv;
      rv = VerifyEntry(manItem, entryData, entryDataLen);
      if (NS_FAILED(rv)) return rv;
    }
    requestedStatus = manItem->status;
  }
  else 
    requestedStatus = mGlobalStatus;

  if (requestedStatus != JAR_VALID_MANIFEST)
    ReportError(aFilename, requestedStatus);
  else 
  {
    *aPrincipal = mPrincipal;
    NS_IF_ADDREF(*aPrincipal);
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsJAR::GetManifestEntriesCount(PRUint32* count)
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

PRFileDesc*
nsJAR::OpenFile()
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(mZipFile, &rv);
  if (NS_FAILED(rv)) return nsnull;

  PRFileDesc* fd;
  rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0000, &fd);
  if (NS_FAILED(rv)) return nsnull;

  return fd;
}




nsresult 
nsJAR::LoadEntry(const char* aFilename, char** aBuf, PRUint32* aBufLen)
{
  
  nsresult rv;
  nsCOMPtr<nsIInputStream> manifestStream;
  rv = GetInputStream(aFilename, getter_AddRefs(manifestStream));
  if (NS_FAILED(rv)) return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
  
  
  char* buf;
  PRUint32 len;
  rv = manifestStream->Available(&len);
  if (NS_FAILED(rv)) return rv;
  if (len == PRUint32(-1))
    return NS_ERROR_FILE_CORRUPTED; 
  buf = (char*)PR_MALLOC(len+1);
  if (!buf) return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 bytesRead;
  rv = manifestStream->Read(buf, len, &bytesRead);
  if (bytesRead != len) 
    rv = NS_ERROR_FILE_CORRUPTED;
  if (NS_FAILED(rv)) {
    PR_FREEIF(buf);
    return rv;
  }
  buf[len] = '\0'; 
  *aBuf = buf;
  if (aBufLen)
    *aBufLen = len;
  return NS_OK;
}


PRInt32
nsJAR::ReadLine(const char** src)
{
  
  
  PRInt32 length;
  char* eol = PL_strpbrk(*src, "\r\n");

  if (eol == nsnull) 
  {
    length = PL_strlen(*src);
    if (length == 0) 
      *src = nsnull;
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
  nsresult rv = FindEntries(JAR_MF_SEARCH_STRING, getter_AddRefs(files));
  if (!files) rv = NS_ERROR_FAILURE;
  if (NS_FAILED(rv)) return rv;

  
  PRBool more;
  rv = files->HasMore(&more);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!more)
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = PR_TRUE;
    return NS_OK;
  }

  nsCAutoString manifestFilename;
  rv = files->GetNext(manifestFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = files->HasMore(&more);
  if (NS_FAILED(rv)) return rv;
  if (more)
  {
    mParsedManifest = PR_TRUE;
    return NS_ERROR_FILE_CORRUPTED; 
  }

  nsXPIDLCString manifestBuffer;
  PRUint32 manifestLen;
  rv = LoadEntry(manifestFilename.get(), getter_Copies(manifestBuffer), &manifestLen);
  if (NS_FAILED(rv)) return rv;

  
  rv = ParseOneFile(manifestBuffer, JAR_MF);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = FindEntries(JAR_SF_SEARCH_STRING, getter_AddRefs(files));
  if (!files) rv = NS_ERROR_FAILURE;
  if (NS_FAILED(rv)) return rv;
  
  rv = files->HasMore(&more);
  if (NS_FAILED(rv)) return rv;
  if (!more)
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = PR_TRUE;
    return NS_OK;
  }
  rv = files->GetNext(manifestFilename);
  if (NS_FAILED(rv)) return rv;

  rv = LoadEntry(manifestFilename.get(), getter_Copies(manifestBuffer), &manifestLen);
  if (NS_FAILED(rv)) return rv;
  
  
  nsCAutoString sigFilename(manifestFilename);
  PRInt32 extension = sigFilename.RFindChar('.') + 1;
  NS_ASSERTION(extension != 0, "Manifest Parser: Missing file extension.");
  (void)sigFilename.Cut(extension, 2);
  nsXPIDLCString sigBuffer;
  PRUint32 sigLen;
  {
    nsCAutoString tempFilename(sigFilename); tempFilename.Append("rsa", 3);
    rv = LoadEntry(tempFilename.get(), getter_Copies(sigBuffer), &sigLen);
  }
  if (NS_FAILED(rv))
  {
    nsCAutoString tempFilename(sigFilename); tempFilename.Append("RSA", 3);
    rv = LoadEntry(tempFilename.get(), getter_Copies(sigBuffer), &sigLen);
  }
  if (NS_FAILED(rv))
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = PR_TRUE;
    return NS_OK;
  }

  
  nsCOMPtr<nsISignatureVerifier> verifier = 
           do_GetService(SIGNATURE_VERIFIER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) 
  {
    mGlobalStatus = JAR_NO_MANIFEST;
    mParsedManifest = PR_TRUE;
    return NS_OK;
  }

  
  PRInt32 verifyError;
  rv = verifier->VerifySignature(sigBuffer, sigLen, manifestBuffer, manifestLen, 
                                 &verifyError, getter_AddRefs(mPrincipal));
  if (NS_FAILED(rv)) return rv;
  if (mPrincipal && verifyError == 0)
    mGlobalStatus = JAR_VALID_MANIFEST;
  else if (verifyError == nsISignatureVerifier::VERIFY_ERROR_UNKNOWN_CA)
    mGlobalStatus = JAR_INVALID_UNKNOWN_CA;
  else
    mGlobalStatus = JAR_INVALID_SIG;

  
  
  
  
  ParseOneFile(manifestBuffer, JAR_SF);
  mParsedManifest = PR_TRUE;

  return NS_OK;
}

nsresult
nsJAR::ParseOneFile(const char* filebuf, PRInt16 aFileType)
{
  
  const char* nextLineStart = filebuf;
  nsCAutoString curLine;
  PRInt32 linelen;
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

  nsJARManifestItem* curItemMF = nsnull;
  PRBool foundName = PR_FALSE;
  if (aFileType == JAR_MF)
    if (!(curItemMF = new nsJARManifestItem()))
      return NS_ERROR_OUT_OF_MEMORY;

  nsCAutoString curItemName;
  nsCAutoString storedSectionDigest;

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
              PRBool exists;
              nsresult rv = HasEntry(curItemName, &exists);
              if (NS_FAILED(rv) || !exists)
                curItemMF->mType = JAR_INVALID;
            }
            
            nsCStringKey key(curItemName);
            if (mManifestData.Exists(&key))
              curItemMF->mType = JAR_INVALID;
          }
        }

        if (curItemMF->mType == JAR_INVALID)
          delete curItemMF;
        else 
        {
          PRUint32 sectionLength = curPos - sectionStart;
          CalculateDigest(sectionStart, sectionLength,
                          curItemMF->calculatedSectionDigest);
          
          nsCStringKey itemKey(curItemName);
          mManifestData.Put(&itemKey, (void*)curItemMF);
        }
        if (nextLineStart == nsnull) 
          break;

        sectionStart = nextLineStart;
        if (!(curItemMF = new nsJARManifestItem()))
          return NS_ERROR_OUT_OF_MEMORY;
      } 
      else
        
        
      {
        if (foundName)
        {
          nsJARManifestItem* curItemSF;
          nsCStringKey key(curItemName);
          curItemSF = (nsJARManifestItem*)mManifestData.Get(&key);
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

        if(nextLineStart == nsnull) 
          break;
      } 
      foundName = PR_FALSE;
      continue;
    } 

    
    
    while(*nextLineStart == ' ')
    {
      curPos = nextLineStart;
      PRInt32 continuationLen = ReadLine(&nextLineStart) - 1;
      nsCAutoString continuation(curPos+1, continuationLen);
      curLine += continuation;
      linelen += continuationLen;
    }

    
    PRInt32 colonPos = curLine.FindChar(':');
    if (colonPos == -1)    
      continue;
    
    nsCAutoString lineName;
    curLine.Left(lineName, colonPos);
    nsCAutoString lineData;
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
      foundName = PR_TRUE;
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
                   PRUint32 aLen)
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
  aManItem->entryVerified = PR_TRUE;
  return NS_OK;
}

void nsJAR::ReportError(const char* aFilename, PRInt16 errorCode)
{
  
  nsAutoString message; 
  message.AssignLiteral("Signature Verification Error: the signature on ");
  if (aFilename)
    message.AppendWithConversion(aFilename);
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


nsresult nsJAR::CalculateDigest(const char* aInBuf, PRUint32 aLen,
                                nsCString& digest)
{
  nsresult rv;

  nsCOMPtr<nsICryptoHash> hasher = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = hasher->Init(nsICryptoHash::SHA1);
  if (NS_FAILED(rv)) return rv;

  rv = hasher->Update((const PRUint8*) aInBuf, aLen);
  if (NS_FAILED(rv)) return rv;

  return hasher->Finish(PR_TRUE, digest);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsJAREnumerator, nsIUTF8StringEnumerator)
  



NS_IMETHODIMP
nsJAREnumerator::HasMore(PRBool* aResult)
{
    
    if (!mCurr) {
        NS_ASSERTION(mFind, "nsJAREnumerator: Missing zipFind.");
        nsresult rv = mFind->FindNext( &mCurr );
        if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
            *aResult = PR_FALSE;                    
            return NS_OK;
        }
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);    
    }

    *aResult = PR_TRUE;
    return NS_OK;
}




NS_IMETHODIMP
nsJAREnumerator::GetNext(nsACString& aResult)
{
    
    if (!mCurr) {
        PRBool   bMore;
        nsresult rv = HasMore(&bMore);
        if (NS_FAILED(rv) || !bMore)
            return NS_ERROR_FAILURE; 
    }
    aResult = mCurr;
    mCurr = 0; 
    return NS_OK;
}


NS_IMPL_THREADSAFE_ISUPPORTS1(nsJARItem, nsIZipEntry)

nsJARItem::nsJARItem(nsZipItem* aZipItem)
    : mSize(aZipItem->size),
      mRealsize(aZipItem->realsize),
      mCrc32(aZipItem->crc32),
      mDate(aZipItem->date),
      mTime(aZipItem->time),
      mCompression(aZipItem->compression),
      mIsDirectory(aZipItem->isDirectory),
      mIsSynthetic(aZipItem->isSynthetic)
{
}




NS_IMETHODIMP
nsJARItem::GetCompression(PRUint16 *aCompression)
{
    NS_ENSURE_ARG_POINTER(aCompression);

    *aCompression = mCompression;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetSize(PRUint32 *aSize)
{
    NS_ENSURE_ARG_POINTER(aSize);

    *aSize = mSize;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetRealSize(PRUint32 *aRealsize)
{
    NS_ENSURE_ARG_POINTER(aRealsize);

    *aRealsize = mRealsize;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetCRC32(PRUint32 *aCrc32)
{
    NS_ENSURE_ARG_POINTER(aCrc32);

    *aCrc32 = mCrc32;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetIsDirectory(PRBool *aIsDirectory)
{
    NS_ENSURE_ARG_POINTER(aIsDirectory);

    *aIsDirectory = mIsDirectory;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetIsSynthetic(PRBool *aIsSynthetic)
{
    NS_ENSURE_ARG_POINTER(aIsSynthetic);

    *aIsSynthetic = mIsSynthetic;
    return NS_OK;
}




NS_IMETHODIMP
nsJARItem::GetLastModifiedTime(PRTime* aLastModTime)
{
    NS_ENSURE_ARG_POINTER(aLastModTime);

    *aLastModTime = GetModTime(mDate, mTime);
    return NS_OK;
}




NS_IMPL_THREADSAFE_ISUPPORTS3(nsZipReaderCache, nsIZipReaderCache, nsIObserver, nsISupportsWeakReference)

nsZipReaderCache::nsZipReaderCache()
  : mLock(nsnull),
    mZips(16)
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
nsZipReaderCache::Init(PRUint32 cacheSize)
{
  mCacheSize = cacheSize; 
  

  nsCOMPtr<nsIObserverService> os = 
           do_GetService("@mozilla.org/observer-service;1");
  if (os)
  {
    os->AddObserver(this, "memory-pressure", PR_TRUE);
    os->AddObserver(this, "chrome-flush-caches", PR_TRUE);
  }


  mLock = PR_NewLock();
  return mLock ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

static PRBool
DropZipReaderCache(nsHashKey *aKey, void *aData, void* closure)
{
  nsJAR* zip = (nsJAR*)aData;
  zip->SetZipReaderCache(nsnull);
  return PR_TRUE;
}

nsZipReaderCache::~nsZipReaderCache()
{
  if (mLock)
    PR_DestroyLock(mLock);
  mZips.Enumerate(DropZipReaderCache, nsnull);

#ifdef ZIP_CACHE_HIT_RATE
  printf("nsZipReaderCache size=%d hits=%d lookups=%d rate=%f%% flushes=%d missed %d\n",
         mCacheSize, mZipCacheHits, mZipCacheLookups, 
         (float)mZipCacheHits / mZipCacheLookups, 
         mZipCacheFlushes, mZipSyncMisses);
#endif
}

NS_IMETHODIMP
nsZipReaderCache::GetZip(nsIFile* zipFile, nsIZipReader* *result)
{
  NS_ENSURE_ARG_POINTER(zipFile);
  nsresult rv;
  nsCOMPtr<nsIJAR> antiLockZipGrip;
  nsAutoLock lock(mLock);

#ifdef ZIP_CACHE_HIT_RATE
  mZipCacheLookups++;
#endif

  nsCAutoString path;
  rv = zipFile->GetNativePath(path);
  if (NS_FAILED(rv)) return rv;

  nsCStringKey key(path);
  nsJAR* zip = static_cast<nsJAR*>(static_cast<nsIZipReader*>(mZips.Get(&key))); 
  if (zip) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheHits++;
#endif
    zip->ClearReleaseTime();
  }
  else {
    if (zip) {
      antiLockZipGrip = zip;
      mZips.Remove(&key);
    }
    zip = new nsJAR();
    if (zip == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(zip);
    zip->SetZipReaderCache(this);

    rv = zip->Open(zipFile);
    if (NS_FAILED(rv)) {
      NS_RELEASE(zip);
      return rv;
    }

    PRBool collision = mZips.Put(&key, static_cast<nsIZipReader*>(zip)); 
    NS_ASSERTION(!collision, "horked");
  }
  *result = zip;
  return rv;
}

static PRBool
FindOldestZip(nsHashKey *aKey, void *aData, void* closure)
{
  nsJAR** oldestPtr = (nsJAR**)closure;
  nsJAR* oldest = *oldestPtr;
  nsJAR* current = (nsJAR*)aData;
  PRIntervalTime currentReleaseTime = current->GetReleaseTime();
  if (currentReleaseTime != PR_INTERVAL_NO_TIMEOUT) {
    if (oldest == nsnull ||
        currentReleaseTime < oldest->GetReleaseTime()) {
      *oldestPtr = current;
    }    
  }
  return PR_TRUE;
}

struct ZipFindData {nsJAR* zip; PRBool found;}; 

static PRBool
FindZip(nsHashKey *aKey, void *aData, void* closure)
{
  ZipFindData* find_data = (ZipFindData*)closure;

  if (find_data->zip == (nsJAR*)aData) {
    find_data->found = PR_TRUE; 
    return PR_FALSE;
  }
  return PR_TRUE;
}

nsresult
nsZipReaderCache::ReleaseZip(nsJAR* zip)
{
  nsresult rv;
  nsAutoLock lock(mLock);

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  ZipFindData find_data = {zip, PR_FALSE};
  mZips.Enumerate(FindZip, &find_data);
  if (!find_data.found) {
#ifdef ZIP_CACHE_HIT_RATE
    mZipSyncMisses++;
#endif
    return NS_OK;
  }

  zip->SetReleaseTime();

  if (mZips.Count() <= mCacheSize)
    return NS_OK;

  nsJAR* oldest = nsnull;
  mZips.Enumerate(FindOldestZip, &oldest);
  
  
  
  if (!oldest)
    return NS_OK;

#ifdef ZIP_CACHE_HIT_RATE
    mZipCacheFlushes++;
#endif

  
  
  
  oldest->SetZipReaderCache(nsnull);

  
  nsCAutoString path;
  rv = oldest->GetJarPath(path);
  if (NS_FAILED(rv)) return rv;

  nsCStringKey key(path);
  PRBool removed = mZips.Remove(&key);  
  NS_ASSERTION(removed, "botched");

  return NS_OK;
}

static PRBool
FindFlushableZip(nsHashKey *aKey, void *aData, void* closure)
{
  nsHashKey** flushableKeyPtr = (nsHashKey**)closure;
  nsJAR* current = (nsJAR*)aData;
  
  if (current->GetReleaseTime() != PR_INTERVAL_NO_TIMEOUT) {
    *flushableKeyPtr = aKey;
    current->SetZipReaderCache(nsnull);
    return PR_FALSE;
  }
  return PR_TRUE;
}

NS_IMETHODIMP
nsZipReaderCache::Observe(nsISupports *aSubject,
                          const char *aTopic, 
                          const PRUnichar *aSomeData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    nsAutoLock lock(mLock);
    while (PR_TRUE) {
      nsHashKey* flushable = nsnull;
      mZips.Enumerate(FindFlushableZip, &flushable); 
      if ( ! flushable )
        break;
      PRBool removed = mZips.Remove(flushable);  
      NS_ASSERTION(removed, "botched");

#ifdef xDEBUG_jband
      printf("flushed something from the jar cache\n");
#endif
    }
  }
  else if (strcmp(aTopic, "chrome-flush-caches") == 0) {
    mZips.Enumerate(DropZipReaderCache, nsnull);
    mZips.Reset();
  }
  return NS_OK;
}

PRTime GetModTime(PRUint16 aDate, PRUint16 aTime)
{
  PRExplodedTime time;

  time.tm_usec = 0;
  
  time.tm_hour = (aTime >> 11) & 0x1F;
  time.tm_min = (aTime >> 5) & 0x3F;
  time.tm_sec = (aTime & 0x1F) * 2;

  time.tm_year = (aDate >> 9) + 1980;
  time.tm_month = ((aDate >> 5) & 0x0F)-1;
  time.tm_mday = aDate & 0x1F;
  
  time.tm_params.tp_gmt_offset = 0;
  time.tm_params.tp_dst_offset = 0;
  
  PR_NormalizeTime(&time, PR_GMTParameters);
  time.tm_params = PR_LocalTimeParameters(&time);
  
  return PR_ImplodeTime(&time);
}


