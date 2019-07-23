










































#include "nsEffectiveTLDService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsFileStreams.h"
#include "nsIFile.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"





#define EFF_TLD_FILENAME NS_LITERAL_CSTRING("effective_tld_names.dat")

NS_IMPL_ISUPPORTS1(nsEffectiveTLDService, nsIEffectiveTLDService)



#define PL_ARENA_CONST_ALIGN_MASK 3
#include "plarena.h"

static PLArenaPool *gArena = nsnull;

#define ARENA_SIZE 512



static char *
ArenaStrDup(const char* str, PLArenaPool* aArena)
{
  void *mem;
  PRUint32 size = strlen(str) + 1;
  PL_ARENA_ALLOCATE(mem, aArena, size);
  if (mem)
    memcpy(mem, str, size);
  return static_cast<char*>(mem);
}

nsDomainEntry::nsDomainEntry(const char *aDomain)
 : mDomain(ArenaStrDup(aDomain, gArena))
 , mIsNormal(PR_FALSE)
 , mIsException(PR_FALSE)
 , mIsWild(PR_FALSE)
{
}



nsresult
nsEffectiveTLDService::Init()
{
  if (!mHash.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  return LoadEffectiveTLDFiles();
}

nsEffectiveTLDService::nsEffectiveTLDService()
{
}

nsEffectiveTLDService::~nsEffectiveTLDService()
{
  if (gArena) {
    PL_FinishArenaPool(gArena);
    delete gArena;
  }
  gArena = nsnull;
}






NS_IMETHODIMP
nsEffectiveTLDService::GetEffectiveTLDLength(const nsACString &aHostname,
                                             PRUint32 *effTLDLength)
{
  
  
  nsCAutoString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv))
    return rv;

  
  PRUint32 trailingDot = normHostname.Last() == '.';
  if (trailingDot)
    normHostname.Truncate(normHostname.Length() - 1);

  
  
  
  const char *prevDomain = nsnull;
  const char *currDomain = normHostname.get();
  const char *nextDot = strchr(currDomain, '.');
  const char *end = currDomain + normHostname.Length();
  while (1) {
    if (!nextDot) {
      
      *effTLDLength = end - currDomain;
      break;
    }

    nsDomainEntry *entry = mHash.GetEntry(currDomain);
    if (entry) {
      if (entry->IsWild() && prevDomain) {
        
        *effTLDLength = end - prevDomain;
        break;

      } else if (entry->IsNormal()) {
        
        *effTLDLength = end - currDomain;
        break;

      } else if (entry->IsException()) {
        
        *effTLDLength = end - nextDot - 1;
        break;
      }
    }

    prevDomain = currDomain;
    currDomain = nextDot + 1;
    nextDot = strchr(currDomain, '.');
  }

  
  *effTLDLength += trailingDot;

  return NS_OK;
}






nsresult
nsEffectiveTLDService::NormalizeHostname(nsCString &aHostname)
{
  if (IsASCII(aHostname)) {
    ToLowerCase(aHostname);
    return NS_OK;
  }

  if (!mIDNService) {
    nsresult rv;
    mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return mIDNService->Normalize(aHostname, aHostname);
}







nsresult
nsEffectiveTLDService::AddEffectiveTLDEntry(nsCString &aDomainName)
{
  
  if (!gArena) {
    gArena = new PLArenaPool;
    NS_ENSURE_TRUE(gArena, NS_ERROR_OUT_OF_MEMORY);
    PL_INIT_ARENA_POOL(gArena, "eTLDArena", ARENA_SIZE);
  }

  PRBool isException = PR_FALSE, isWild = PR_FALSE;

  
  if (aDomainName.First() == '!') {
    isException = PR_TRUE;
    aDomainName.Cut(0, 1);

  
  } else if (StringBeginsWith(aDomainName, NS_LITERAL_CSTRING("*."))) {
    isWild = PR_TRUE;
    aDomainName.Cut(0, 2);

    NS_ASSERTION(!StringBeginsWith(aDomainName, NS_LITERAL_CSTRING("*.")),
                 "only one wildcard level supported!");
  }

  
  nsresult rv = NormalizeHostname(aDomainName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsDomainEntry *entry = mHash.PutEntry(aDomainName.get());
  NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

  
  if (!entry->GetKey()) {
    mHash.RawRemoveEntry(entry);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  entry->IsWild() |= isWild;
  entry->IsException() |= isException;
  
  entry->IsNormal() |= isWild || !isException;

  return NS_OK;
}






nsresult
LocateEffectiveTLDFile(nsCOMPtr<nsIFile>& foundFile, PRBool aUseProfile)
{
  foundFile = nsnull;

  nsCOMPtr<nsIFile> effTLDFile = nsnull;
  nsresult rv = NS_OK;
  PRBool exists = PR_FALSE;
  if (aUseProfile) {
    
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(effTLDFile));
    
    
    if (NS_FAILED(rv))
      return rv;
  }
  else {
    
    rv = NS_GetSpecialDirectory(NS_OS_CURRENT_PROCESS_DIR,
                                getter_AddRefs(effTLDFile));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = effTLDFile->AppendNative(NS_LITERAL_CSTRING("res"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = effTLDFile->AppendNative(EFF_TLD_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = effTLDFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists)
    foundFile = effTLDFile;

  return rv;
}

void
TruncateAtWhitespace(nsCString &aString)
{
  
  
  nsASingleFragmentCString::const_char_iterator begin, iter, end;
  aString.BeginReading(begin);
  aString.EndReading(end);

  for (iter = begin; iter != end; ++iter) {
    if (*iter == ' ' || *iter == '\t') {
      aString.Truncate(iter - begin);
      break;
    }
  }
}





nsresult
nsEffectiveTLDService::LoadOneEffectiveTLDFile(nsCOMPtr<nsIFile>& effTLDFile)
{
  
  nsCOMPtr<nsIInputStream> fileStream;
  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream),
                                           effTLDFile,
                                           0x01, 
                                           -1,   
                                           nsIFileInputStream::CLOSE_ON_EOF);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString lineData;
  PRBool isMore;
  NS_NAMED_LITERAL_CSTRING(kCommentMarker, "//");
  while (NS_SUCCEEDED(lineStream->ReadLine(lineData, &isMore)) && isMore) {
    if (StringBeginsWith(lineData, kCommentMarker))
      continue;

    TruncateAtWhitespace(lineData);
    if (!lineData.IsEmpty()) {
      rv = AddEffectiveTLDEntry(lineData);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Error adding effective TLD to list");
    }
  }

  return NS_OK;
}




nsresult
nsEffectiveTLDService::LoadEffectiveTLDFiles()
{
  nsCOMPtr<nsIFile> effTLDFile;
  nsresult rv = LocateEffectiveTLDFile(effTLDFile, PR_FALSE);

  
  
  if (NS_FAILED(rv) || nsnull == effTLDFile) {
    NS_WARNING("No effective-TLD file found in system res directory");
  }
  else {
    rv = LoadOneEffectiveTLDFile(effTLDFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = LocateEffectiveTLDFile(effTLDFile, PR_TRUE);

  
  
  if (NS_FAILED(rv) || nsnull == effTLDFile)
    return NS_OK;

  return LoadOneEffectiveTLDFile(effTLDFile);
}
