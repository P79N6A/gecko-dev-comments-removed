









































#include "nsEffectiveTLDService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDataHashtable.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsFileStreams.h"
#include "nsIFile.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "nsString.h"





#define EFF_TLD_FILENAME NS_LITERAL_CSTRING("effective_tld_names.dat")



#define EFF_TLD_SERVICE_DEBUG 0










































struct SubdomainNode;
typedef nsDataHashtable<nsCStringHashKey, SubdomainNode *> SubdomainNodeHash;
struct SubdomainNode {
  PRBool exception;
  PRBool stopOK;
  SubdomainNodeHash children;
};

static void EmptyEffectiveTLDTree();
static nsresult LoadEffectiveTLDFiles();
static nsresult NormalizeHostname(nsCString &aHostname);
static PRInt32 FindEarlierDot(const nsACString &aHostname, PRInt32 aDotLoc);
static SubdomainNode *FindMatchingChildNode(SubdomainNode *parent,
                                            const nsCSubstring &aSubname,
                                            PRBool aCreateNewNode);

nsEffectiveTLDService* nsEffectiveTLDService::sInstance = nsnull;
static SubdomainNode sSubdomainTreeHead;

NS_IMPL_ISUPPORTS1(nsEffectiveTLDService, nsIEffectiveTLDService)



#if defined(EFF_TLD_SERVICE_DEBUG) && EFF_TLD_SERVICE_DEBUG




PR_STATIC_CALLBACK(PLDHashOperator)
LOG_EFF_TLD_NODE(const nsACString& aKey, SubdomainNode *aData, void *aLevel)
{
  
  PRInt32 *level = (PRInt32 *) aLevel;
  for (PRInt32 i = 0; i < *level; i++)    printf("-");
  if (aData->exception)                   printf("!");
  if (aData->stopOK)                      printf("^");
  printf("'%s'\n", PromiseFlatCString(aKey).get());

  
  PRInt32 newlevel = (*level) + 1;
  aData->children.EnumerateRead(LOG_EFF_TLD_NODE, &newlevel);

  return PL_DHASH_NEXT;
}
#endif 




void LOG_EFF_TLD_TREE(const char *msg = "",
                  SubdomainNode *head = &sSubdomainTreeHead)
{
#if defined(EFF_TLD_SERVICE_DEBUG) && EFF_TLD_SERVICE_DEBUG
  if (msg && msg != "")
    printf("%s\n", msg);

  PRInt32 level = 0;
  head->children.EnumerateRead(LOG_EFF_TLD_NODE, &level);
#endif 

  return;
}









nsresult
nsEffectiveTLDService::Init()
{
  sSubdomainTreeHead.exception = PR_FALSE;
  sSubdomainTreeHead.stopOK = PR_FALSE;
  nsresult rv = NS_OK;
  if (!sSubdomainTreeHead.children.Init())
    rv = NS_ERROR_OUT_OF_MEMORY;

  if (NS_SUCCEEDED(rv))
    rv = LoadEffectiveTLDFiles();

  
  if (NS_FAILED(rv))
    EmptyEffectiveTLDTree();

  return rv;
}



nsEffectiveTLDService::nsEffectiveTLDService()
{
  NS_ASSERTION(!sInstance, "Multiple effective-TLD services being created");
  sInstance = this;
}



nsEffectiveTLDService::~nsEffectiveTLDService()
{
  NS_ASSERTION(sInstance == this, "Multiple effective-TLD services exist");
  EmptyEffectiveTLDTree();
  sInstance = nsnull;
}






NS_IMETHODIMP
nsEffectiveTLDService::GetEffectiveTLDLength(const nsACString &aHostname,
                                             PRUint32 *effTLDLength)
{
  
  
  
  PRInt32 nameLength = aHostname.Length();
  PRInt32 trailingDotOffset = (nameLength && aHostname.Last() == '.' ? 1 : 0);
  PRInt32 defaultTLDLength;
  PRInt32 dotLoc = FindEarlierDot(aHostname, nameLength - 1 - trailingDotOffset);
  if (dotLoc < 0) {
    defaultTLDLength = nameLength;
  }
  else {
    defaultTLDLength = nameLength - dotLoc - 1;
  }
  *effTLDLength = NS_STATIC_CAST(PRUint32, defaultTLDLength);

  
  
  nsCAutoString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv))
    return rv;

  
  SubdomainNode *node = &sSubdomainTreeHead;
  dotLoc = nameLength - trailingDotOffset;
  while (dotLoc > 0) {
    PRInt32 nextDotLoc = FindEarlierDot(normHostname, dotLoc - 1);
    const nsCSubstring &subname = Substring(normHostname, nextDotLoc + 1,
                                            dotLoc - nextDotLoc - 1);
    SubdomainNode *child = FindMatchingChildNode(node, subname, false);
    if (nsnull == child)
      break;
    if (child->exception) {
      
      *effTLDLength = NS_STATIC_CAST(PRUint32, nameLength - dotLoc - 1);
      node = child;
      break;
    }
    
    
    if (child->stopOK)
      *effTLDLength = NS_STATIC_CAST(PRUint32, nameLength - nextDotLoc - 1);
    node = child;
    dotLoc = nextDotLoc;
  }

  return NS_OK;
}



PR_STATIC_CALLBACK(PLDHashOperator)
DeleteSubdomainNode(const nsACString& aKey, SubdomainNode *aData, void *aUser)
{
  
  
  
  if (nsnull != aData && aData->children.IsInitialized()) {
    
    aData->children.EnumerateRead(DeleteSubdomainNode, nsnull);

    
    aData->children.Clear();
  }

  return PL_DHASH_NEXT;
}





void
EmptyEffectiveTLDTree()
{
  if (sSubdomainTreeHead.children.IsInitialized()) {
    sSubdomainTreeHead.children.EnumerateRead(DeleteSubdomainNode, nsnull);
    sSubdomainTreeHead.children.Clear();
  }
  return;
}






nsresult NormalizeHostname(nsCString &aHostname)
{
  nsresult rv = NS_OK;

  if (IsASCII(aHostname)) {
    ToLowerCase(aHostname);
  }
  else {
    nsCOMPtr<nsIIDNService> idnServ = do_GetService(NS_IDNSERVICE_CONTRACTID);
    if (idnServ)
      rv = idnServ->Normalize(aHostname, aHostname);
  }

  return rv;
}






void
FillStringInformation(const nsACString &aString, const char **start,
                      PRUint32 *length)
{
  nsACString::const_iterator iter;
  aString.BeginReading(iter);
  *start = iter.get();
  *length = iter.size_forward();
  return;
}





PRInt32
FindEarlierDot(const nsACString &aHostname, PRInt32 aDotLoc)
{
  if (aDotLoc < 0)
    return -1;

  
  
  const char *start;
  PRUint32 length;
  FillStringInformation(aHostname, &start, &length);
  PRInt32 endLoc = length - 1;

  if (aDotLoc < endLoc)
    endLoc = aDotLoc;
  for (const char *where = start + endLoc; where >= start; --where) {
    if (*where == '.')
      return (where - start);
  }

  return -1;
}





PRInt32
FindEndOfName(const nsACString &aHostname) {
  
  
  const char *start;
  PRUint32 length;
  FillStringInformation(aHostname, &start, &length);

  for (const char *where = start; where < start + length; ++where) {
    if (*where == ' ' || *where == '\t')
      return (where - start);
  }

  return length;
}

















SubdomainNode *
FindMatchingChildNode(SubdomainNode *parent, const nsCSubstring &aSubname,
                      PRBool aCreateNewNode)
{
  
  nsCAutoString name(aSubname);
  PRBool exception = PR_FALSE;

  
  if (name.Length() > 0 && name.First() == '!') {
    exception = PR_TRUE;
    name.Cut(0, 1);
  }

  
  SubdomainNode *result = nsnull;
  SubdomainNode *match;
  if (parent->children.Get(name, &match)) {
    result = match;
  }

  
  else if (aCreateNewNode) {
    SubdomainNode *node = new SubdomainNode;
    if (node) {
      node->exception = exception;
      node->stopOK = PR_FALSE;
      if (node->children.Init() && parent->children.Put(name, node))
        result = node;
    }
  }

  
  else if (parent->children.Get(NS_LITERAL_CSTRING("*"), &match)) {
    result = match;
  }

  return result;
}








nsresult
AddEffectiveTLDEntry(nsCString &aDomainName) {
  SubdomainNode *node = &sSubdomainTreeHead;

  
  nsresult rv = NormalizeHostname(aDomainName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dotLoc = FindEndOfName(aDomainName);
  while (dotLoc > 0) {
    PRInt32 nextDotLoc = FindEarlierDot(aDomainName, dotLoc - 1);
    const nsCSubstring &subname = Substring(aDomainName, nextDotLoc + 1,
                                            dotLoc - nextDotLoc - 1);
    dotLoc = nextDotLoc;
    node = FindMatchingChildNode(node, subname, true);
    if (!node)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  node->stopOK = true;

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





nsresult
LoadOneEffectiveTLDFile(nsCOMPtr<nsIFile>& effTLDFile)
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
  PRBool moreData = PR_TRUE;
  NS_NAMED_LITERAL_CSTRING(commentMarker, "//");
  while (moreData) {
    rv = lineStream->ReadLine(lineData, &moreData);
    if (NS_SUCCEEDED(rv)) {
      if (! lineData.IsEmpty() &&
          ! StringBeginsWith(lineData, commentMarker)) {
        rv = AddEffectiveTLDEntry(lineData);
      }
    }
    else {
      NS_WARNING("Error reading effective-TLD file; attempting to continue");
    }
  }

  LOG_EFF_TLD_TREE("Effective-TLD tree:");

  return rv;
}




nsresult
LoadEffectiveTLDFiles()
{
  nsCOMPtr<nsIFile> effTLDFile;
  nsresult rv = LocateEffectiveTLDFile(effTLDFile, false);

  
  
  if (NS_FAILED(rv) || nsnull == effTLDFile) {
    NS_WARNING("No effective-TLD file found in system res directory");
  }
  else {
    rv = LoadOneEffectiveTLDFile(effTLDFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = LocateEffectiveTLDFile(effTLDFile, true);

  
  
  if (NS_FAILED(rv) || nsnull == effTLDFile)
    return NS_OK;

  return LoadOneEffectiveTLDFile(effTLDFile);
}
