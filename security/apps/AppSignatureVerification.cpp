





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif

#include "nsNSSCertificateDB.h"

#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "mozilla/RefPtr.h"
#include "CryptoTask.h"
#include "AppTrustDomain.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsDataSignatureVerifier.h"
#include "nsHashKeys.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIStringEnumerator.h"
#include "nsIZipReader.h"
#include "nsNSSCertificate.h"
#include "nsProxyRelease.h"
#include "nsString.h"
#include "nsTHashtable.h"

#include "base64.h"
#include "certdb.h"
#include "secmime.h"
#include "plstr.h"
#include "prlog.h"

using namespace mozilla::pkix;
using namespace mozilla;
using namespace mozilla::psm;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace {





nsresult
FindAndLoadOneEntry(nsIZipReader * zip,
                    const nsACString & searchPattern,
                     nsACString & filename,
                     SECItem & buf,
                     Digest * bufDigest)
{
  nsCOMPtr<nsIUTF8StringEnumerator> files;
  nsresult rv = zip->FindEntries(searchPattern, getter_AddRefs(files));
  if (NS_FAILED(rv) || !files) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  bool more;
  rv = files->HasMore(&more);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!more) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  rv = files->GetNext(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = files->HasMore(&more);
  NS_ENSURE_SUCCESS(rv, rv);
  if (more) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  nsCOMPtr<nsIInputStream> stream;
  rv = zip->GetInputStream(filename, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  uint64_t len64;
  rv = stream->Available(&len64);
  NS_ENSURE_SUCCESS(rv, rv);


  
  
  
  
  
  
  
  
  static const uint32_t MAX_LENGTH = 1024 * 1024;
  static_assert(MAX_LENGTH < UINT32_MAX, "MAX_LENGTH < UINT32_MAX");
  NS_ENSURE_TRUE(len64 < MAX_LENGTH, NS_ERROR_FILE_CORRUPTED);
  NS_ENSURE_TRUE(len64 < UINT32_MAX, NS_ERROR_FILE_CORRUPTED); 
  SECITEM_AllocItem(buf, static_cast<uint32_t>(len64 + 1));

  
  
  
  uint32_t bytesRead;
  rv = stream->Read(char_ptr_cast(buf.data), buf.len, &bytesRead);
  NS_ENSURE_SUCCESS(rv, rv);
  if (bytesRead != len64) {
    return NS_ERROR_SIGNED_JAR_ENTRY_INVALID;
  }

  buf.data[buf.len - 1] = 0; 

  if (bufDigest) {
    rv = bufDigest->DigestBuf(SEC_OID_SHA1, buf.data, buf.len - 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}










nsresult
VerifyEntryContentDigest(nsIZipReader * zip, const nsACString & aFilename,
                         const SECItem & digestFromManifest, SECItem & buf)
{
  MOZ_ASSERT(buf.len > 0);
  if (digestFromManifest.len != SHA1_LENGTH)
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;

  nsresult rv;

  nsCOMPtr<nsIInputStream> stream;
  rv = zip->GetInputStream(aFilename, getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    return NS_ERROR_SIGNED_JAR_ENTRY_MISSING;
  }

  uint64_t len64;
  rv = stream->Available(&len64);
  NS_ENSURE_SUCCESS(rv, rv);
  if (len64 > UINT32_MAX) {
    return NS_ERROR_SIGNED_JAR_ENTRY_TOO_LARGE;
  }

  ScopedPK11Context digestContext(PK11_CreateDigestContext(SEC_OID_SHA1));
  if (!digestContext) {
    return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
  }

  rv = MapSECStatus(PK11_DigestBegin(digestContext));
  NS_ENSURE_SUCCESS(rv, rv);

  uint64_t totalBytesRead = 0;
  for (;;) {
    uint32_t bytesRead;
    rv = stream->Read(char_ptr_cast(buf.data), buf.len, &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bytesRead == 0) {
      break; 
    }

    totalBytesRead += bytesRead;
    if (totalBytesRead >= UINT32_MAX) {
      return NS_ERROR_SIGNED_JAR_ENTRY_TOO_LARGE;
    }

    rv = MapSECStatus(PK11_DigestOp(digestContext, buf.data, bytesRead));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (totalBytesRead != len64) {
    
    
    return NS_ERROR_SIGNED_JAR_ENTRY_INVALID;
  }

  
  Digest digest;
  rv = digest.End(SEC_OID_SHA1, digestContext);
  NS_ENSURE_SUCCESS(rv, rv);

  if (SECITEM_CompareItem(&digestFromManifest, &digest.get()) != SECEqual) {
    return NS_ERROR_SIGNED_JAR_MODIFIED_ENTRY;
  }

  return NS_OK;
}



nsresult
ReadLine( const char* & nextLineStart,  nsCString & line,
         bool allowContinuations = true)
{
  line.Truncate();
  size_t previousLength = 0;
  size_t currentLength = 0;
  for (;;) {
    const char* eol = PL_strpbrk(nextLineStart, "\r\n");

    if (!eol) { 
      eol = nextLineStart + strlen(nextLineStart);
    }

    previousLength = currentLength;
    line.Append(nextLineStart, eol - nextLineStart);
    currentLength = line.Length();

    
    
    static const size_t lineLimit = 72;
    if (currentLength - previousLength > lineLimit) {
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
    }

    
    
    if (currentLength > 65535) {
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
    }

    if (*eol == '\r') {
      ++eol;
    }
    if (*eol == '\n') {
      ++eol;
    }

    nextLineStart = eol;

    if (*eol != ' ') {
      
      return NS_OK;
    }

    
    if (!allowContinuations) {
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
    }

    ++nextLineStart; 
  }
}


#define JAR_MF_SEARCH_STRING "(M|/M)ETA-INF/(M|m)(ANIFEST|anifest).(MF|mf)$"
#define JAR_SF_SEARCH_STRING "(M|/M)ETA-INF/*.(SF|sf)$"
#define JAR_RSA_SEARCH_STRING "(M|/M)ETA-INF/*.(RSA|rsa)$"
#define JAR_MF_HEADER "Manifest-Version: 1.0"
#define JAR_SF_HEADER "Signature-Version: 1.0"

nsresult
ParseAttribute(const nsAutoCString & curLine,
                nsAutoCString & attrName,
                nsAutoCString & attrValue)
{
  
  int32_t colonPos = curLine.FindChar(':');
  if (colonPos == kNotFound) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  
  int32_t nameEnd = colonPos;
  for (;;) {
    if (nameEnd == 0) {
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID; 
    }
    if (curLine[nameEnd - 1] != ' ')
      break;
    --nameEnd;
  }
  curLine.Left(attrName, nameEnd);

  
  
  int32_t valueStart = colonPos + 1;
  int32_t curLineLength = curLine.Length();
  while (valueStart != curLineLength && curLine[valueStart] == ' ') {
    ++valueStart;
  }
  curLine.Right(attrValue, curLineLength - valueStart);

  return NS_OK;
}


nsresult
CheckManifestVersion(const char* & nextLineStart,
                     const nsACString & expectedHeader)
{
  
  
  
  nsAutoCString curLine;
  nsresult rv = ReadLine(nextLineStart, curLine, false);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!curLine.Equals(expectedHeader)) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }
  return NS_OK;
}























nsresult
ParseSF(const char* filebuf,  SECItem & mfDigest)
{
  nsresult rv;

  const char* nextLineStart = filebuf;
  rv = CheckManifestVersion(nextLineStart, NS_LITERAL_CSTRING(JAR_SF_HEADER));
  if (NS_FAILED(rv))
    return rv;

  
  for (;;) {
    nsAutoCString curLine;
    rv = ReadLine(nextLineStart, curLine);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (curLine.Length() == 0) {
      
      
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
    }

    nsAutoCString attrName;
    nsAutoCString attrValue;
    rv = ParseAttribute(curLine, attrName, attrValue);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (attrName.LowerCaseEqualsLiteral("sha1-digest-manifest")) {
      rv = MapSECStatus(ATOB_ConvertAsciiToItem(&mfDigest, attrValue.get()));
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      
      
      
      
      
      
      
      break;
    }

    
  }

  return NS_OK;
}




nsresult
ParseMF(const char* filebuf, nsIZipReader * zip,
         nsTHashtable<nsCStringHashKey> & mfItems,
        ScopedAutoSECItem & buf)
{
  nsresult rv;

  const char* nextLineStart = filebuf;

  rv = CheckManifestVersion(nextLineStart, NS_LITERAL_CSTRING(JAR_MF_HEADER));
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  {
    nsAutoCString line;
    do {
      rv = ReadLine(nextLineStart, line);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } while (line.Length() > 0);

    
    if (*nextLineStart == '\0') {
      return NS_OK;
    }
  }

  nsAutoCString curItemName;
  ScopedAutoSECItem digest;

  for (;;) {
    nsAutoCString curLine;
    rv = ReadLine(nextLineStart, curLine);
    NS_ENSURE_SUCCESS(rv, rv);

    if (curLine.Length() == 0) {
      

      if (curItemName.Length() == 0) {
        
        
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
      }

      if (digest.len == 0) {
        
        
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
      }

      if (mfItems.Contains(curItemName)) {
        
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
      }

      
      
      rv = VerifyEntryContentDigest(zip, curItemName, digest, buf);
      if (NS_FAILED(rv))
        return rv;

      mfItems.PutEntry(curItemName);

      if (*nextLineStart == '\0') 
        break;

      
      
      curItemName.Truncate();
      digest.reset();

      continue; 
    }

    nsAutoCString attrName;
    nsAutoCString attrValue;
    rv = ParseAttribute(curLine, attrName, attrValue);
    if (NS_FAILED(rv)) {
      return rv;
    }

    

    
    if (attrName.LowerCaseEqualsLiteral("sha1-digest"))
    {
      if (digest.len > 0) 
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;

      rv = MapSECStatus(ATOB_ConvertAsciiToItem(&digest, attrValue.get()));
      if (NS_FAILED(rv))
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;

      continue;
    }

    
    if (attrName.LowerCaseEqualsLiteral("name"))
    {
      if (MOZ_UNLIKELY(curItemName.Length() > 0)) 
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;

      if (MOZ_UNLIKELY(attrValue.Length() == 0))
        return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;

      curItemName = attrValue;

      continue;
    }

    
    if (attrName.LowerCaseEqualsLiteral("magic")) {
      
      
      
      return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
    }

    
  }

  return NS_OK;
}

struct VerifyCertificateContext {
  AppTrustedRoot trustedRoot;
  ScopedCERTCertList& builtChain;
};

nsresult
VerifyCertificate(CERTCertificate* signerCert, void* voidContext, void* pinArg)
{
  
  if (NS_WARN_IF(!signerCert) || NS_WARN_IF(!voidContext)) {
    return NS_ERROR_INVALID_ARG;
  }
  const VerifyCertificateContext& context =
    *reinterpret_cast<const VerifyCertificateContext*>(voidContext);

  AppTrustDomain trustDomain(context.builtChain, pinArg);
  if (trustDomain.SetTrustedRoot(context.trustedRoot) != SECSuccess) {
    return MapSECStatus(SECFailure);
  }
  Result rv = BuildCertChain(trustDomain, signerCert->derCert, PR_Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::digitalSignature,
                             KeyPurposeId::id_kp_codeSigning,
                             CertPolicyId::anyPolicy,
                             nullptr);
  if (rv != Success) {
    return mozilla::psm::GetXPCOMFromNSSError(MapResultToPRErrorCode(rv));
  }

  return NS_OK;
}

nsresult
VerifySignature(AppTrustedRoot trustedRoot, const SECItem& buffer,
                const SECItem& detachedDigest,
                 ScopedCERTCertList& builtChain)
{
  VerifyCertificateContext context = { trustedRoot, builtChain };
  
  return VerifyCMSDetachedSignatureIncludingCertificate(buffer, detachedDigest,
                                                        VerifyCertificate,
                                                        &context, nullptr);
}

NS_IMETHODIMP
OpenSignedAppFile(AppTrustedRoot aTrustedRoot, nsIFile* aJarFile,
                   nsIZipReader** aZipReader,
                   nsIX509Cert** aSignerCert)
{
  NS_ENSURE_ARG_POINTER(aJarFile);

  if (aZipReader) {
    *aZipReader = nullptr;
  }

  if (aSignerCert) {
    *aSignerCert = nullptr;
  }

  nsresult rv;

  static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);
  nsCOMPtr<nsIZipReader> zip = do_CreateInstance(kZipReaderCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = zip->Open(aJarFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString sigFilename;
  ScopedAutoSECItem sigBuffer;
  rv = FindAndLoadOneEntry(zip, nsLiteralCString(JAR_RSA_SEARCH_STRING),
                           sigFilename, sigBuffer, nullptr);
  if (NS_FAILED(rv)) {
    return NS_ERROR_SIGNED_JAR_NOT_SIGNED;
  }

  
  nsAutoCString sfFilename;
  ScopedAutoSECItem sfBuffer;
  Digest sfCalculatedDigest;
  rv = FindAndLoadOneEntry(zip, NS_LITERAL_CSTRING(JAR_SF_SEARCH_STRING),
                           sfFilename, sfBuffer, &sfCalculatedDigest);
  if (NS_FAILED(rv)) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  sigBuffer.type = siBuffer;
  ScopedCERTCertList builtChain;
  rv = VerifySignature(aTrustedRoot, sigBuffer, sfCalculatedDigest.get(),
                       builtChain);
  if (NS_FAILED(rv)) {
    return rv;
  }

  ScopedAutoSECItem mfDigest;
  rv = ParseSF(char_ptr_cast(sfBuffer.data), mfDigest);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsAutoCString mfFilename;
  ScopedAutoSECItem manifestBuffer;
  Digest mfCalculatedDigest;
  rv = FindAndLoadOneEntry(zip, NS_LITERAL_CSTRING(JAR_MF_SEARCH_STRING),
                           mfFilename, manifestBuffer, &mfCalculatedDigest);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (SECITEM_CompareItem(&mfDigest, &mfCalculatedDigest.get()) != SECEqual) {
    return NS_ERROR_SIGNED_JAR_MANIFEST_INVALID;
  }

  
  
  
  ScopedAutoSECItem buf(128 * 1024);

  nsTHashtable<nsCStringHashKey> items;

  rv = ParseMF(char_ptr_cast(manifestBuffer.data), zip, items, buf);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIUTF8StringEnumerator> entries;
  rv = zip->FindEntries(EmptyCString(), getter_AddRefs(entries));
  if (NS_SUCCEEDED(rv) && !entries) {
    rv = NS_ERROR_UNEXPECTED;
  }
  if (NS_FAILED(rv)) {
    return rv;
  }

  for (;;) {
    bool hasMore;
    rv = entries->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!hasMore) {
      break;
    }

    nsAutoCString entryFilename;
    rv = entries->GetNext(entryFilename);
    NS_ENSURE_SUCCESS(rv, rv);

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Verifying digests for %s",
           entryFilename.get()));

    
    
    
    
    
    
    if (entryFilename == mfFilename ||
        entryFilename == sfFilename ||
        entryFilename == sigFilename) {
      continue;
    }

    if (entryFilename.Length() == 0) {
      return NS_ERROR_SIGNED_JAR_ENTRY_INVALID;
    }

    
    
    
    
    
    
    
    if (entryFilename[entryFilename.Length() - 1] == '/') {
      continue;
    }

    nsCStringHashKey * item = items.GetEntry(entryFilename);
    if (!item) {
      return NS_ERROR_SIGNED_JAR_UNSIGNED_ENTRY;
    }

    
    items.RemoveEntry(entryFilename);
  }

  
  
  
  if (items.Count() != 0) {
    return NS_ERROR_SIGNED_JAR_ENTRY_MISSING;
  }

  
  if (aZipReader) {
    zip.forget(aZipReader);
  }

  
  
  if (aSignerCert) {
    MOZ_ASSERT(CERT_LIST_HEAD(builtChain));
    nsCOMPtr<nsIX509Cert> signerCert =
      nsNSSCertificate::Create(CERT_LIST_HEAD(builtChain)->cert);
    NS_ENSURE_TRUE(signerCert, NS_ERROR_OUT_OF_MEMORY);
    signerCert.forget(aSignerCert);
  }

  return NS_OK;
}

class OpenSignedAppFileTask MOZ_FINAL : public CryptoTask
{
public:
  OpenSignedAppFileTask(AppTrustedRoot aTrustedRoot, nsIFile* aJarFile,
                        nsIOpenSignedAppFileCallback* aCallback)
    : mTrustedRoot(aTrustedRoot)
    , mJarFile(aJarFile)
    , mCallback(new nsMainThreadPtrHolder<nsIOpenSignedAppFileCallback>(aCallback))
  {
  }

private:
  virtual nsresult CalculateResult() MOZ_OVERRIDE
  {
    return OpenSignedAppFile(mTrustedRoot, mJarFile,
                             getter_AddRefs(mZipReader),
                             getter_AddRefs(mSignerCert));
  }

  
  
  virtual void ReleaseNSSResources() { }

  virtual void CallCallback(nsresult rv)
  {
    (void) mCallback->OpenSignedAppFileFinished(rv, mZipReader, mSignerCert);
  }

  const AppTrustedRoot mTrustedRoot;
  const nsCOMPtr<nsIFile> mJarFile;
  nsMainThreadPtrHandle<nsIOpenSignedAppFileCallback> mCallback;
  nsCOMPtr<nsIZipReader> mZipReader; 
  nsCOMPtr<nsIX509Cert> mSignerCert; 
};

} 

NS_IMETHODIMP
nsNSSCertificateDB::OpenSignedAppFileAsync(
  AppTrustedRoot aTrustedRoot, nsIFile* aJarFile,
  nsIOpenSignedAppFileCallback* aCallback)
{
  NS_ENSURE_ARG_POINTER(aJarFile);
  NS_ENSURE_ARG_POINTER(aCallback);
  RefPtr<OpenSignedAppFileTask> task(new OpenSignedAppFileTask(aTrustedRoot,
                                                               aJarFile,
                                                               aCallback));
  return task->Dispatch("SignedJAR");
}
