






























#include "HashStore.h"
#include "nsICryptoHash.h"
#include "nsISeekableStream.h"
#include "nsIStreamConverterService.h"
#include "nsNetUtil.h"
#include "nsCheckSummedOutputStream.h"
#include "prlog.h"
#include "zlib.h"




















































#define STORE_SUFFIX ".sbstore"


extern PRLogModuleInfo *gUrlClassifierDbServiceLog;
#if defined(PR_LOGGING)
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierDbServiceLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (false)
#endif



#define SUCCESS_OR_RESET(res)                                             \
  do {                                                                    \
    nsresult __rv = res; /* Don't evaluate |res| more than once */        \
    if (__rv == NS_ERROR_OUT_OF_MEMORY) {                                 \
      NS_WARNING("SafeBrowsing OOM.");                                    \
      return __rv;                                                        \
    }                                                                     \
    if (NS_FAILED(__rv)) {                                                \
      NS_WARNING("SafeBrowsing store corrupted or out of date.");         \
      Reset();                                                            \
      return __rv;                                                        \
    }                                                                     \
  } while(0)

namespace mozilla {
namespace safebrowsing {

const uint32_t STORE_MAGIC = 0x1231af3b;
const uint32_t CURRENT_VERSION = 3;

nsresult
TableUpdate::NewAddPrefix(uint32_t aAddChunk, const Prefix& aHash)
{
  AddPrefix *add = mAddPrefixes.AppendElement();
  if (!add) return NS_ERROR_OUT_OF_MEMORY;
  add->addChunk = aAddChunk;
  add->prefix = aHash;
  return NS_OK;
}

nsresult
TableUpdate::NewSubPrefix(uint32_t aAddChunk, const Prefix& aHash, uint32_t aSubChunk)
{
  SubPrefix *sub = mSubPrefixes.AppendElement();
  if (!sub) return NS_ERROR_OUT_OF_MEMORY;
  sub->addChunk = aAddChunk;
  sub->prefix = aHash;
  sub->subChunk = aSubChunk;
  return NS_OK;
}

nsresult
TableUpdate::NewAddComplete(uint32_t aAddChunk, const Completion& aHash)
{
  AddComplete *add = mAddCompletes.AppendElement();
  if (!add) return NS_ERROR_OUT_OF_MEMORY;
  add->addChunk = aAddChunk;
  add->complete = aHash;
  return NS_OK;
}

nsresult
TableUpdate::NewSubComplete(uint32_t aAddChunk, const Completion& aHash, uint32_t aSubChunk)
{
  SubComplete *sub = mSubCompletes.AppendElement();
  if (!sub) return NS_ERROR_OUT_OF_MEMORY;
  sub->addChunk = aAddChunk;
  sub->complete = aHash;
  sub->subChunk = aSubChunk;
  return NS_OK;
}

HashStore::HashStore(const nsACString& aTableName, nsIFile* aStoreDir)
  : mTableName(aTableName)
  , mStoreDirectory(aStoreDir)
  , mInUpdate(false)
{
}

HashStore::~HashStore()
{
}

nsresult
HashStore::Reset()
{
  LOG(("HashStore resetting"));

  nsCOMPtr<nsIFile> storeFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(STORE_SUFFIX));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::CheckChecksum(nsIFile* aStoreFile,
                         uint32_t aFileSize)
{
  
  
  nsAutoCString hash;
  nsAutoCString compareHash;
  char *data;
  uint32_t read;

  nsresult rv = CalculateChecksum(hash, aFileSize, true);
  NS_ENSURE_SUCCESS(rv, rv);

  compareHash.GetMutableData(&data, hash.Length());

  if (hash.Length() > aFileSize) {
    NS_WARNING("SafeBrowing file not long enough to store its hash");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsISeekableStream> seekIn = do_QueryInterface(mInputStream);
  rv = seekIn->Seek(nsISeekableStream::NS_SEEK_SET, aFileSize - hash.Length());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mInputStream->Read(data, hash.Length(), &read);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(read == hash.Length(), "Could not read hash bytes");

  if (!hash.Equals(compareHash)) {
    NS_WARNING("Safebrowing file failed checksum.");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
HashStore::Open()
{
  nsCOMPtr<nsIFile> storeFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(".sbstore"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> origStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(origStream), storeFile,
                                  PR_RDONLY | nsIFile::OS_READAHEAD);

  if (rv == NS_ERROR_FILE_NOT_FOUND) {
    UpdateHeader();
    return NS_OK;
  } else {
    SUCCESS_OR_RESET(rv);
  }

  int64_t fileSize;
  rv = storeFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  if (fileSize < 0 || fileSize > UINT32_MAX) {
    return NS_ERROR_FAILURE;
  }

  uint32_t fileSize32 = static_cast<uint32_t>(fileSize);

  rv = NS_NewBufferedInputStream(getter_AddRefs(mInputStream), origStream,
                                 fileSize32);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckChecksum(storeFile, fileSize32);
  SUCCESS_OR_RESET(rv);

  rv = ReadHeader();
  SUCCESS_OR_RESET(rv);

  rv = SanityCheck();
  SUCCESS_OR_RESET(rv);

  rv = ReadChunkNumbers();
  SUCCESS_OR_RESET(rv);

  return NS_OK;
}

nsresult
HashStore::ReadHeader()
{
  if (!mInputStream) {
    UpdateHeader();
    return NS_OK;
  }

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mInputStream);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  void *buffer = &mHeader;
  rv = NS_ReadInputStreamToBuffer(mInputStream,
                                  &buffer,
                                  sizeof(Header));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::SanityCheck()
{
  if (mHeader.magic != STORE_MAGIC || mHeader.version != CURRENT_VERSION) {
    NS_WARNING("Unexpected header data in the store.");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
HashStore::CalculateChecksum(nsAutoCString& aChecksum,
                             uint32_t aFileSize,
                             bool aChecksumPresent)
{
  aChecksum.Truncate();

  
  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mInputStream);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

  nsCOMPtr<nsICryptoHash> hash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  const uint32_t CHECKSUM_SIZE = 16;

  
  
  rv = hash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aChecksumPresent) {
    
    rv = hash->UpdateFromStream(mInputStream, UINT32_MAX);
  } else {
    
    if (aFileSize < CHECKSUM_SIZE) {
      NS_WARNING("SafeBrowsing file isn't long enough to store its checksum");
      return NS_ERROR_FAILURE;
    }
    rv = hash->UpdateFromStream(mInputStream, aFileSize - CHECKSUM_SIZE);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = hash->Finish(false, aChecksum);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
HashStore::UpdateHeader()
{
  mHeader.magic = STORE_MAGIC;
  mHeader.version = CURRENT_VERSION;

  mHeader.numAddChunks = mAddChunks.Length();
  mHeader.numSubChunks = mSubChunks.Length();
  mHeader.numAddPrefixes = mAddPrefixes.Length();
  mHeader.numSubPrefixes = mSubPrefixes.Length();
  mHeader.numAddCompletes = mAddCompletes.Length();
  mHeader.numSubCompletes = mSubCompletes.Length();
}

nsresult
HashStore::ReadChunkNumbers()
{
  NS_ENSURE_STATE(mInputStream);

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mInputStream);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                               sizeof(Header));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mAddChunks.Read(mInputStream, mHeader.numAddChunks);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mAddChunks.Length() == mHeader.numAddChunks, "Read the right amount of add chunks.");

  rv = mSubChunks.Read(mInputStream, mHeader.numSubChunks);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mSubChunks.Length() == mHeader.numSubChunks, "Read the right amount of sub chunks.");

  return NS_OK;
}

nsresult
HashStore::ReadHashes()
{
  if (!mInputStream) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mInputStream);

  uint32_t offset = sizeof(Header);
  offset += (mHeader.numAddChunks + mHeader.numSubChunks) * sizeof(uint32_t);
  nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadAddPrefixes();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadSubPrefixes();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadTArray(mInputStream, &mAddCompletes, mHeader.numAddCompletes);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadTArray(mInputStream, &mSubCompletes, mHeader.numSubCompletes);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::BeginUpdate()
{
  
  nsresult rv = ReadHashes();
  SUCCESS_OR_RESET(rv);

  
  
  if (mInputStream) {
    rv = mInputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mInUpdate = true;

  return NS_OK;
}

template<class T>
static nsresult
Merge(ChunkSet* aStoreChunks,
      FallibleTArray<T>* aStorePrefixes,
      ChunkSet& aUpdateChunks,
      FallibleTArray<T>& aUpdatePrefixes,
      bool aAllowMerging = false)
{
  EntrySort(aUpdatePrefixes);

  T* updateIter = aUpdatePrefixes.Elements();
  T* updateEnd = aUpdatePrefixes.Elements() + aUpdatePrefixes.Length();

  T* storeIter = aStorePrefixes->Elements();
  T* storeEnd = aStorePrefixes->Elements() + aStorePrefixes->Length();

  
  
  nsTArray<T> adds;

  for (; updateIter != updateEnd; updateIter++) {
    
    
    
    if (aStoreChunks->Has(updateIter->Chunk()))
      if (!aAllowMerging)
        continue;
    
    
    while (storeIter < storeEnd && (storeIter->Compare(*updateIter) < 0)) {
      
      storeIter++;
    }
    
    if (storeIter == storeEnd
        || storeIter->Compare(*updateIter) != 0) {
      if (!adds.AppendElement(*updateIter))
        return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  aStoreChunks->Merge(aUpdateChunks);

  aStorePrefixes->AppendElements(adds);
  EntrySort(*aStorePrefixes);

  return NS_OK;
}

nsresult
HashStore::ApplyUpdate(TableUpdate &update)
{
  nsresult rv = mAddExpirations.Merge(update.AddExpirations());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSubExpirations.Merge(update.SubExpirations());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Expire();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Merge(&mAddChunks, &mAddPrefixes,
             update.AddChunks(), update.AddPrefixes());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Merge(&mAddChunks, &mAddCompletes,
             update.AddChunks(), update.AddCompletes(), true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Merge(&mSubChunks, &mSubPrefixes,
             update.SubChunks(), update.SubPrefixes());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Merge(&mSubChunks, &mSubCompletes,
             update.SubChunks(), update.SubCompletes(), true);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::Rebuild()
{
  NS_ASSERTION(mInUpdate, "Must be in update to rebuild.");

  nsresult rv = ProcessSubs();
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateHeader();

  return NS_OK;
}

void
HashStore::ClearCompletes()
{
  NS_ASSERTION(mInUpdate, "Must be in update to clear completes.");

  mAddCompletes.Clear();
  mSubCompletes.Clear();

  UpdateHeader();
}

template<class T>
static void
ExpireEntries(FallibleTArray<T>* aEntries, ChunkSet& aExpirations)
{
  T* addIter = aEntries->Elements();
  T* end = aEntries->Elements() + aEntries->Length();

  for (T *iter = addIter; iter != end; iter++) {
    if (!aExpirations.Has(iter->Chunk())) {
      *addIter = *iter;
      addIter++;
    }
  }

  aEntries->SetLength(addIter - aEntries->Elements());
}

nsresult
HashStore::Expire()
{
  ExpireEntries(&mAddPrefixes, mAddExpirations);
  ExpireEntries(&mAddCompletes, mAddExpirations);
  ExpireEntries(&mSubPrefixes, mSubExpirations);
  ExpireEntries(&mSubCompletes, mSubExpirations);

  mAddChunks.Remove(mAddExpirations);
  mSubChunks.Remove(mSubExpirations);

  mAddExpirations.Clear();
  mSubExpirations.Clear();

  return NS_OK;
}

template<class T>
nsresult DeflateWriteTArray(nsIOutputStream* aStream, nsTArray<T>& aIn)
{
  uLongf insize = aIn.Length() * sizeof(T);
  uLongf outsize = compressBound(insize);
  FallibleTArray<char> outBuff;
  if (!outBuff.SetLength(outsize)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  int zerr = compress(reinterpret_cast<Bytef*>(outBuff.Elements()),
                      &outsize,
                      reinterpret_cast<const Bytef*>(aIn.Elements()),
                      insize);
  if (zerr != Z_OK) {
    return NS_ERROR_FAILURE;
  }
  LOG(("DeflateWriteTArray: %d in %d out", insize, outsize));

  outBuff.TruncateLength(outsize);

  
  uint32_t dataLen = outBuff.Length();
  uint32_t written;
  nsresult rv = aStream->Write(reinterpret_cast<char*>(&dataLen), sizeof(dataLen), &written);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(written == sizeof(dataLen), "Error writing deflate length");

  
  rv = WriteTArray(aStream, outBuff);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

template<class T>
nsresult InflateReadTArray(nsIInputStream* aStream, FallibleTArray<T>* aOut,
                           uint32_t aExpectedSize)
{

  uint32_t inLen;
  uint32_t read;
  nsresult rv = aStream->Read(reinterpret_cast<char*>(&inLen), sizeof(inLen), &read);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(read == sizeof(inLen), "Error reading inflate length");

  FallibleTArray<char> inBuff;
  if (!inBuff.SetLength(inLen)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = ReadTArray(aStream, &inBuff, inLen);
  NS_ENSURE_SUCCESS(rv, rv);

  uLongf insize = inLen;
  uLongf outsize = aExpectedSize * sizeof(T);
  if (!aOut->SetLength(aExpectedSize)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  int zerr = uncompress(reinterpret_cast<Bytef*>(aOut->Elements()),
                        &outsize,
                        reinterpret_cast<const Bytef*>(inBuff.Elements()),
                        insize);
  if (zerr != Z_OK) {
    return NS_ERROR_FAILURE;
  }
  LOG(("InflateReadTArray: %d in %d out", insize, outsize));

  NS_ASSERTION(outsize == aExpectedSize * sizeof(T), "Decompression size mismatch");

  return NS_OK;
}

static nsresult
ByteSliceWrite(nsIOutputStream* aOut, nsTArray<uint32_t>& aData)
{
  nsTArray<uint8_t> slice1;
  nsTArray<uint8_t> slice2;
  nsTArray<uint8_t> slice3;
  nsTArray<uint8_t> slice4;
  uint32_t count = aData.Length();

  slice1.SetCapacity(count);
  slice2.SetCapacity(count);
  slice3.SetCapacity(count);
  slice4.SetCapacity(count);

  for (uint32_t i = 0; i < count; i++) {
    slice1.AppendElement( aData[i] >> 24);
    slice2.AppendElement((aData[i] >> 16) & 0xFF);
    slice3.AppendElement((aData[i] >>  8) & 0xFF);
    slice4.AppendElement( aData[i]        & 0xFF);
  }

  nsresult rv = DeflateWriteTArray(aOut, slice1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = DeflateWriteTArray(aOut, slice2);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = DeflateWriteTArray(aOut, slice3);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  rv = WriteTArray(aOut, slice4);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static nsresult
ByteSliceRead(nsIInputStream* aInStream, FallibleTArray<uint32_t>* aData, uint32_t count)
{
  FallibleTArray<uint8_t> slice1;
  FallibleTArray<uint8_t> slice2;
  FallibleTArray<uint8_t> slice3;
  FallibleTArray<uint8_t> slice4;

  nsresult rv = InflateReadTArray(aInStream, &slice1, count);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InflateReadTArray(aInStream, &slice2, count);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InflateReadTArray(aInStream, &slice3, count);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadTArray(aInStream, &slice4, count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aData->SetCapacity(count)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < count; i++) {
    aData->AppendElement((slice1[i] << 24) | (slice2[i] << 16)
                         | (slice3[i] << 8) | (slice4[i]));
  }

  return NS_OK;
}

nsresult
HashStore::ReadAddPrefixes()
{
  FallibleTArray<uint32_t> chunks;
  uint32_t count = mHeader.numAddPrefixes;

  nsresult rv = ByteSliceRead(mInputStream, &chunks, count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mAddPrefixes.SetCapacity(count)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  for (uint32_t i = 0; i < count; i++) {
    AddPrefix *add = mAddPrefixes.AppendElement();
    add->prefix.FromUint32(0);
    add->addChunk = chunks[i];
  }

  return NS_OK;
}

nsresult
HashStore::ReadSubPrefixes()
{
  FallibleTArray<uint32_t> addchunks;
  FallibleTArray<uint32_t> subchunks;
  FallibleTArray<uint32_t> prefixes;
  uint32_t count = mHeader.numSubPrefixes;

  nsresult rv = ByteSliceRead(mInputStream, &addchunks, count);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ByteSliceRead(mInputStream, &subchunks, count);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ByteSliceRead(mInputStream, &prefixes, count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mSubPrefixes.SetCapacity(count)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  for (uint32_t i = 0; i < count; i++) {
    SubPrefix *sub = mSubPrefixes.AppendElement();
    sub->addChunk = addchunks[i];
    sub->prefix.FromUint32(prefixes[i]);
    sub->subChunk = subchunks[i];
  }

  return NS_OK;
}


nsresult
HashStore::WriteAddPrefixes(nsIOutputStream* aOut)
{
  nsTArray<uint32_t> chunks;
  uint32_t count = mAddPrefixes.Length();
  chunks.SetCapacity(count);

  for (uint32_t i = 0; i < count; i++) {
    chunks.AppendElement(mAddPrefixes[i].Chunk());
  }

  nsresult rv = ByteSliceWrite(aOut, chunks);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::WriteSubPrefixes(nsIOutputStream* aOut)
{
  nsTArray<uint32_t> addchunks;
  nsTArray<uint32_t> subchunks;
  nsTArray<uint32_t> prefixes;
  uint32_t count = mSubPrefixes.Length();
  addchunks.SetCapacity(count);
  subchunks.SetCapacity(count);
  prefixes.SetCapacity(count);

  for (uint32_t i = 0; i < count; i++) {
    addchunks.AppendElement(mSubPrefixes[i].AddChunk());
    prefixes.AppendElement(mSubPrefixes[i].PrefixHash().ToUint32());
    subchunks.AppendElement(mSubPrefixes[i].Chunk());
  }

  nsresult rv = ByteSliceWrite(aOut, addchunks);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ByteSliceWrite(aOut, subchunks);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ByteSliceWrite(aOut, prefixes);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
HashStore::WriteFile()
{
  NS_ASSERTION(mInUpdate, "Must be in update to write database.");

  nsCOMPtr<nsIFile> storeFile;
  nsresult rv = mStoreDirectory->Clone(getter_AddRefs(storeFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storeFile->AppendNative(mTableName + NS_LITERAL_CSTRING(".sbstore"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> out;
  rv = NS_NewCheckSummedOutputStream(getter_AddRefs(out), storeFile,
                                     PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t written;
  rv = out->Write(reinterpret_cast<char*>(&mHeader), sizeof(mHeader), &written);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mAddChunks.Write(out);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSubChunks.Write(out);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteAddPrefixes(out);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WriteSubPrefixes(out);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WriteTArray(out, mAddCompletes);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WriteTArray(out, mSubCompletes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISafeOutputStream> safeOut = do_QueryInterface(out, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = safeOut->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

template <class T>
static void
Erase(FallibleTArray<T>* array, T* iterStart, T* iterEnd)
{
  uint32_t start = iterStart - array->Elements();
  uint32_t count = iterEnd - iterStart;

  if (count > 0) {
    array->RemoveElementsAt(start, count);
  }
}









template<class TSub, class TAdd>
static void
KnockoutSubs(FallibleTArray<TSub>* aSubs, FallibleTArray<TAdd>* aAdds)
{
  
  
  
  
  TAdd* addOut = aAdds->Elements();
  TAdd* addIter = aAdds->Elements();

  TSub* subOut = aSubs->Elements();
  TSub* subIter = aSubs->Elements();

  TAdd* addEnd = addIter + aAdds->Length();
  TSub* subEnd = subIter + aSubs->Length();

  while (addIter != addEnd && subIter != subEnd) {
    
    int32_t cmp = addIter->Compare(*subIter);
    if (cmp > 0) {
      
      *subOut = *subIter;
      ++subOut;
      ++subIter;
    } else if (cmp < 0) {
      
      *addOut = *addIter;
      ++addOut;
      ++addIter;
    } else {
      
      ++addIter;
      ++subIter;
    }
  }

  Erase(aAdds, addOut, addIter);
  Erase(aSubs, subOut, subIter);
}



template <class T>
static void
RemoveMatchingPrefixes(const SubPrefixArray& aSubs, FallibleTArray<T>* aFullHashes)
{
  
  T* out = aFullHashes->Elements();
  T* hashIter = out;
  T* hashEnd = aFullHashes->Elements() + aFullHashes->Length();

  SubPrefix const * removeIter = aSubs.Elements();
  SubPrefix const * removeEnd = aSubs.Elements() + aSubs.Length();

  while (hashIter != hashEnd && removeIter != removeEnd) {
    int32_t cmp = removeIter->CompareAlt(*hashIter);
    if (cmp > 0) {
      
      *out = *hashIter;
      ++out;
      ++hashIter;
    } else if (cmp < 0) {
      
      ++removeIter;
    } else {
      
      do {
        ++hashIter;
      } while (hashIter != hashEnd &&
               !(removeIter->CompareAlt(*hashIter) < 0));
      ++removeIter;
    }
  }
  Erase(aFullHashes, out, hashIter);
}

static void
RemoveDeadSubPrefixes(SubPrefixArray& aSubs, ChunkSet& aAddChunks)
{
  SubPrefix * subIter = aSubs.Elements();
  SubPrefix * subEnd = aSubs.Elements() + aSubs.Length();

  for (SubPrefix * iter = subIter; iter != subEnd; iter++) {
    bool hasChunk = aAddChunks.Has(iter->AddChunk());
    
    
    if (!hasChunk) {
      *subIter = *iter;
      subIter++;
    }
  }

  LOG(("Removed %u dead SubPrefix entries.", subEnd - subIter));
  aSubs.SetLength(subIter - aSubs.Elements());
}

#ifdef DEBUG
template <class T>
static void EnsureSorted(FallibleTArray<T>* aArray)
{
  T* start = aArray->Elements();
  T* end = aArray->Elements() + aArray->Length();
  T* iter = start;
  T* previous = start;

  while (iter != end) {
    previous = iter;
    ++iter;
    if (iter != end) {
      MOZ_ASSERT(iter->Compare(*previous) >= 0);
    }
  }

  return;
}
#endif

nsresult
HashStore::ProcessSubs()
{
#ifdef DEBUG
  EnsureSorted(&mAddPrefixes);
  EnsureSorted(&mSubPrefixes);
  EnsureSorted(&mAddCompletes);
  EnsureSorted(&mSubCompletes);
  LOG(("All databases seem to have a consistent sort order."));
#endif

  RemoveMatchingPrefixes(mSubPrefixes, &mAddCompletes);
  RemoveMatchingPrefixes(mSubPrefixes, &mSubCompletes);

  
  
  KnockoutSubs(&mSubPrefixes, &mAddPrefixes);
  KnockoutSubs(&mSubCompletes, &mAddCompletes);

  
  
  RemoveDeadSubPrefixes(mSubPrefixes, mAddChunks);

#ifdef DEBUG
  EnsureSorted(&mAddPrefixes);
  EnsureSorted(&mSubPrefixes);
  EnsureSorted(&mAddCompletes);
  EnsureSorted(&mSubCompletes);
  LOG(("All databases seem to have a consistent sort order."));
#endif

  return NS_OK;
}

nsresult
HashStore::AugmentAdds(const nsTArray<uint32_t>& aPrefixes)
{
  uint32_t cnt = aPrefixes.Length();
  if (cnt != mAddPrefixes.Length()) {
    LOG(("Amount of prefixes in cache not consistent with store (%d vs %d)",
         aPrefixes.Length(), mAddPrefixes.Length()));
    return NS_ERROR_FAILURE;
  }
  for (uint32_t i = 0; i < cnt; i++) {
    mAddPrefixes[i].prefix.FromUint32(aPrefixes[i]);
  }
  return NS_OK;
}

}
}
