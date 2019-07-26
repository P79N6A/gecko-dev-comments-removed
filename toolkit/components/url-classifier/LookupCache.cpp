




#include "LookupCache.h"
#include "HashStore.h"
#include "nsISeekableStream.h"
#include "mozilla/Telemetry.h"
#include "prlog.h"
#include "prprf.h"


















#define CACHE_SUFFIX ".cache"


#define PREFIXSET_SUFFIX  ".pset"


extern PRLogModuleInfo *gUrlClassifierDbServiceLog;
#if defined(PR_LOGGING)
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierDbServiceLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (false)
#endif

namespace mozilla {
namespace safebrowsing {

const uint32_t LOOKUPCACHE_MAGIC = 0x1231af3e;
const uint32_t CURRENT_VERSION = 2;

LookupCache::LookupCache(const nsACString& aTableName, nsIFile* aStoreDir)
  : mPrimed(false)
  , mTableName(aTableName)
  , mStoreDirectory(aStoreDir)
{
}

nsresult
LookupCache::Init()
{
  mPrefixSet = new nsUrlClassifierPrefixSet();
  nsresult rv = mPrefixSet->Init(mTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

LookupCache::~LookupCache()
{
}

nsresult
LookupCache::Open()
{
  nsCOMPtr<nsIFile> storeFile;

  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(CACHE_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), storeFile,
                                  PR_RDONLY | nsIFile::OS_READAHEAD);

  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND) {
    Reset();
    return rv;
  }

  if (rv == NS_ERROR_FILE_NOT_FOUND) {
    
    
    
    ClearCompleteCache();
  } else {
    
    rv = ReadHeader(inputStream);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("ReadCompletions"));
    rv = ReadCompletions(inputStream);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = inputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  LOG(("Loading PrefixSet"));
  rv = LoadPrefixSet();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
LookupCache::UpdateDirHandle(nsIFile* aStoreDirectory)
{
  return aStoreDirectory->Clone(getter_AddRefs(mStoreDirectory));
}

nsresult
LookupCache::Reset()
{
  LOG(("LookupCache resetting"));

  nsCOMPtr<nsIFile> storeFile;
  nsCOMPtr<nsIFile> prefixsetFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStoreDirectory->Clone(getter_AddRefs(prefixsetFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(CACHE_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prefixsetFile->AppendNative(mTableName + NS_LITERAL_CSTRING(PREFIXSET_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prefixsetFile->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  ClearAll();

  return NS_OK;
}


nsresult
LookupCache::Build(AddPrefixArray& aAddPrefixes,
                   AddCompleteArray& aAddCompletes)
{
  Telemetry::Accumulate(Telemetry::URLCLASSIFIER_LC_COMPLETIONS,
                        static_cast<uint32_t>(aAddCompletes.Length()));

  mCompletions.Clear();
  mCompletions.SetCapacity(aAddCompletes.Length());
  for (uint32_t i = 0; i < aAddCompletes.Length(); i++) {
    mCompletions.AppendElement(aAddCompletes[i].CompleteHash());
  }
  aAddCompletes.Clear();
  mCompletions.Sort();

  Telemetry::Accumulate(Telemetry::URLCLASSIFIER_LC_PREFIXES,
                        static_cast<uint32_t>(aAddPrefixes.Length()));

  nsresult rv = ConstructPrefixSet(aAddPrefixes);
  NS_ENSURE_SUCCESS(rv, rv);
  mPrimed = true;

  return NS_OK;
}

#if defined(DEBUG) && defined(PR_LOGGING)
void
LookupCache::Dump()
{
  if (!LOG_ENABLED())
    return;

  for (uint32_t i = 0; i < mCompletions.Length(); i++) {
    nsAutoCString str;
    mCompletions[i].ToHexString(str);
    LOG(("Completion: %s", str.get()));
  }
}
#endif

nsresult
LookupCache::Has(const Completion& aCompletion,
                 bool* aHas, bool* aComplete)
{
  *aHas = *aComplete = false;

  uint32_t prefix = aCompletion.ToUint32();

  bool found;
  nsresult rv = mPrefixSet->Contains(prefix, &found);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Probe in %s: %X, found %d", mTableName.get(), prefix, found));

  if (found) {
    *aHas = true;
  }

  if (mCompletions.BinaryIndexOf(aCompletion) != nsTArray<Completion>::NoIndex) {
    LOG(("Complete in %s", mTableName.get()));
    *aComplete = true;
    *aHas = true;
  }

  return NS_OK;
}

nsresult
LookupCache::WriteFile()
{
  nsCOMPtr<nsIFile> storeFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(CACHE_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> out;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out), storeFile,
                                       PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE);
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateHeader();
  LOG(("Writing %d completions", mHeader.numCompletions));

  uint32_t written;
  rv = out->Write(reinterpret_cast<char*>(&mHeader), sizeof(mHeader), &written);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WriteTArray(out, mCompletions);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISafeOutputStream> safeOut = do_QueryInterface(out);
  rv = safeOut->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureSizeConsistent();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> psFile;
  rv = mStoreDirectory->Clone(getter_AddRefs(psFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = psFile->AppendNative(mTableName + NS_LITERAL_CSTRING(PREFIXSET_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefixSet->StoreToFile(psFile);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to store the prefixset");

  return NS_OK;
}

void
LookupCache::ClearAll()
{
  ClearCompleteCache();
  mPrefixSet->SetPrefixes(nullptr, 0);
  mPrimed = false;
}

void
LookupCache::ClearCompleteCache()
{
  mCompletions.Clear();
  UpdateHeader();
}

void
LookupCache::UpdateHeader()
{
  mHeader.magic = LOOKUPCACHE_MAGIC;
  mHeader.version = CURRENT_VERSION;
  mHeader.numCompletions = mCompletions.Length();
}

nsresult
LookupCache::EnsureSizeConsistent()
{
  nsCOMPtr<nsIFile> storeFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(CACHE_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t fileSize;
  rv = storeFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  if (fileSize < 0) {
    return NS_ERROR_FAILURE;
  }

  int64_t expectedSize = sizeof(mHeader)
                        + mHeader.numCompletions*sizeof(Completion);
  if (expectedSize != fileSize) {
    NS_WARNING("File length does not match. Probably corrupted.");
    Reset();
    return NS_ERROR_FILE_CORRUPTED;
  }

  return NS_OK;
}

nsresult
LookupCache::ReadHeader(nsIInputStream* aInputStream)
{
  if (!aInputStream) {
    ClearCompleteCache();
    return NS_OK;
  }

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(aInputStream);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  void *buffer = &mHeader;
  rv = NS_ReadInputStreamToBuffer(aInputStream,
                                  &buffer,
                                  sizeof(Header));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mHeader.magic != LOOKUPCACHE_MAGIC || mHeader.version != CURRENT_VERSION) {
    NS_WARNING("Unexpected header data in the store.");
    Reset();
    return NS_ERROR_FILE_CORRUPTED;
  }
  LOG(("%d completions present", mHeader.numCompletions));

  rv = EnsureSizeConsistent();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
LookupCache::ReadCompletions(nsIInputStream* aInputStream)
{
  if (!mHeader.numCompletions) {
    mCompletions.Clear();
    return NS_OK;
  }

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(aInputStream);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, sizeof(Header));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadTArray(aInputStream, &mCompletions, mHeader.numCompletions);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Read %d completions", mCompletions.Length()));

  return NS_OK;
}

 bool
LookupCache::IsCanonicalizedIP(const nsACString& aHost)
{
  
  
  uint32_t i1, i2, i3, i4;
  char c;
  if (PR_sscanf(PromiseFlatCString(aHost).get(), "%u.%u.%u.%u%c",
                &i1, &i2, &i3, &i4, &c) == 4) {
    return (i1 <= 0xFF && i2 <= 0xFF && i3 <= 0xFF && i4 <= 0xFF);
  }

  return false;
}

 nsresult
LookupCache::GetKey(const nsACString& aSpec,
                    Completion* aHash,
                    nsCOMPtr<nsICryptoHash>& aCryptoHash)
{
  nsACString::const_iterator begin, end, iter;
  aSpec.BeginReading(begin);
  aSpec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
   return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter);

  if (IsCanonicalizedIP(host)) {
    nsAutoCString key;
    key.Assign(host);
    key.Append('/');
    return aHash->FromPlaintext(key, aCryptoHash);
  }

  nsTArray<nsCString> hostComponents;
  ParseString(PromiseFlatCString(host), '.', hostComponents);

  if (hostComponents.Length() < 2)
    return NS_ERROR_FAILURE;

  int32_t last = int32_t(hostComponents.Length()) - 1;
  nsAutoCString lookupHost;

  if (hostComponents.Length() > 2) {
    lookupHost.Append(hostComponents[last - 2]);
    lookupHost.Append('.');
  }

  lookupHost.Append(hostComponents[last - 1]);
  lookupHost.Append('.');
  lookupHost.Append(hostComponents[last]);
  lookupHost.Append('/');

  return aHash->FromPlaintext(lookupHost, aCryptoHash);
}

 nsresult
LookupCache::GetLookupFragments(const nsACString& aSpec,
                                nsTArray<nsCString>* aFragments)

{
  aFragments->Clear();

  nsACString::const_iterator begin, end, iter;
  aSpec.BeginReading(begin);
  aSpec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter++);
  nsAutoCString path;
  path.Assign(Substring(iter, end));

  








  nsTArray<nsCString> hosts;
  hosts.AppendElement(host);

  if (!IsCanonicalizedIP(host)) {
    host.BeginReading(begin);
    host.EndReading(end);
    int numHostComponents = 0;
    while (RFindInReadable(NS_LITERAL_CSTRING("."), begin, end) &&
           numHostComponents < MAX_HOST_COMPONENTS) {
      
      if (++numHostComponents >= 2) {
        host.EndReading(iter);
        hosts.AppendElement(Substring(end, iter));
      }
      end = begin;
      host.BeginReading(begin);
    }
  }

  











  nsTArray<nsCString> paths;
  nsAutoCString pathToAdd;

  path.BeginReading(begin);
  path.EndReading(end);
  iter = begin;
  if (FindCharInReadable('?', iter, end)) {
    pathToAdd = Substring(begin, iter);
    paths.AppendElement(pathToAdd);
    end = iter;
  }

  int numPathComponents = 1;
  iter = begin;
  while (FindCharInReadable('/', iter, end) &&
         numPathComponents < MAX_PATH_COMPONENTS) {
    iter++;
    pathToAdd.Assign(Substring(begin, iter));
    paths.AppendElement(pathToAdd);
    numPathComponents++;
  }

  
  if (!pathToAdd.Equals(path)) {
    paths.AppendElement(path);
  }
  
  paths.AppendElement(EmptyCString());

  for (uint32_t hostIndex = 0; hostIndex < hosts.Length(); hostIndex++) {
    for (uint32_t pathIndex = 0; pathIndex < paths.Length(); pathIndex++) {
      nsCString key;
      key.Assign(hosts[hostIndex]);
      key.Append('/');
      key.Append(paths[pathIndex]);
      LOG(("Checking fragment %s", key.get()));

      aFragments->AppendElement(key);
    }
  }

  return NS_OK;
}

 nsresult
LookupCache::GetHostKeys(const nsACString& aSpec,
                         nsTArray<nsCString>* aHostKeys)
{
  nsACString::const_iterator begin, end, iter;
  aSpec.BeginReading(begin);
  aSpec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter);

  if (IsCanonicalizedIP(host)) {
    nsCString *key = aHostKeys->AppendElement();
    if (!key)
      return NS_ERROR_OUT_OF_MEMORY;

    key->Assign(host);
    key->Append("/");
    return NS_OK;
  }

  nsTArray<nsCString> hostComponents;
  ParseString(PromiseFlatCString(host), '.', hostComponents);

  if (hostComponents.Length() < 2) {
    
    return NS_OK;
  }

  
  int32_t last = int32_t(hostComponents.Length()) - 1;
  nsCString *lookupHost = aHostKeys->AppendElement();
  if (!lookupHost)
    return NS_ERROR_OUT_OF_MEMORY;

  lookupHost->Assign(hostComponents[last - 1]);
  lookupHost->Append(".");
  lookupHost->Append(hostComponents[last]);
  lookupHost->Append("/");

  
  if (hostComponents.Length() > 2) {
    nsCString *lookupHost2 = aHostKeys->AppendElement();
    if (!lookupHost2)
      return NS_ERROR_OUT_OF_MEMORY;
    lookupHost2->Assign(hostComponents[last - 2]);
    lookupHost2->Append(".");
    lookupHost2->Append(*lookupHost);
  }

  return NS_OK;
}

bool LookupCache::IsPrimed()
{
  return mPrimed;
}

#ifdef DEBUG
template <class T>
static void EnsureSorted(T* aArray)
{
  typename T::elem_type* start = aArray->Elements();
  typename T::elem_type* end = aArray->Elements() + aArray->Length();
  typename T::elem_type* iter = start;
  typename T::elem_type* previous = start;

  while (iter != end) {
    previous = iter;
    ++iter;
    if (iter != end) {
      MOZ_ASSERT(*previous <= *iter);
    }
  }
  return;
}
#endif

nsresult
LookupCache::ConstructPrefixSet(AddPrefixArray& aAddPrefixes)
{
  Telemetry::AutoTimer<Telemetry::URLCLASSIFIER_PS_CONSTRUCT_TIME> timer;

  nsTArray<uint32_t> array;
  array.SetCapacity(aAddPrefixes.Length());

  for (uint32_t i = 0; i < aAddPrefixes.Length(); i++) {
    array.AppendElement(aAddPrefixes[i].PrefixHash().ToUint32());
  }
  aAddPrefixes.Clear();

#ifdef DEBUG
  
  EnsureSorted(&array);
#endif

  
  nsresult rv = mPrefixSet->SetPrefixes(array.Elements(), array.Length());
  if (NS_FAILED(rv)) {
    goto error_bailout;
  }

#ifdef DEBUG
  uint32_t size;
  size = mPrefixSet->SizeOfIncludingThis(moz_malloc_size_of);
  LOG(("SB tree done, size = %d bytes\n", size));
#endif

  mPrimed = true;

  return NS_OK;

 error_bailout:
  Telemetry::Accumulate(Telemetry::URLCLASSIFIER_PS_FAILURE, 1);
  return rv;
}

nsresult
LookupCache::LoadPrefixSet()
{
  nsCOMPtr<nsIFile> psFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(psFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = psFile->AppendNative(mTableName + NS_LITERAL_CSTRING(PREFIXSET_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = psFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    LOG(("stored PrefixSet exists, loading from disk"));
    rv = mPrefixSet->LoadFromFile(psFile);
    if (NS_FAILED(rv)) {
      if (rv == NS_ERROR_FILE_CORRUPTED) {
        Reset();
      }
      return rv;
    }
    mPrimed = true;
  } else {
    LOG(("no (usable) stored PrefixSet found"));
  }

#ifdef DEBUG
  if (mPrimed) {
    uint32_t size = mPrefixSet->SizeOfIncludingThis(moz_malloc_size_of);
    LOG(("SB tree done, size = %d bytes\n", size));
  }
#endif

  return NS_OK;
}

nsresult
LookupCache::GetPrefixes(nsTArray<uint32_t>* aAddPrefixes)
{
  if (!mPrimed) {
    
    LOG(("GetPrefixes from empty LookupCache"));
    return NS_OK;
  }
  uint32_t cnt;
  uint32_t *arr;
  nsresult rv = mPrefixSet->GetPrefixes(&cnt, &arr);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!aAddPrefixes->AppendElements(arr, cnt))
    return NS_ERROR_FAILURE;
  nsMemory::Free(arr);
  return NS_OK;
}


}
}
