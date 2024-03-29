





#include "ActorsParent.h"

#include <algorithm>
#include "FileInfo.h"
#include "FileManager.h"
#include "IDBObjectStore.h"
#include "IDBTransaction.h"
#include "IndexedDatabase.h"
#include "IndexedDatabaseInlines.h"
#include "IndexedDatabaseManager.h"
#include "js/StructuredClone.h"
#include "js/Value.h"
#include "jsapi.h"
#include "KeyPath.h"
#include "mozilla/Attributes.h"
#include "mozilla/AppProcessChecker.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Endian.h"
#include "mozilla/Hal.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/Maybe.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/storage.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/FileService.h"
#include "mozilla/dom/StructuredCloneTags.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBCursorParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBDatabaseParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBDatabaseFileParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBFactoryParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBFactoryRequestParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBRequestParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBTransactionParent.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBVersionChangeTransactionParent.h"
#include "mozilla/dom/indexedDB/PIndexedDBPermissionRequestParent.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/dom/quota/Client.h"
#include "mozilla/dom/quota/FileStreams.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/quota/UsageInfo.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/InputStreamParams.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/PBackground.h"
#include "mozilla/storage/Variant.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsClassHashtable.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsEscape.h"
#include "nsHashKeys.h"
#include "nsNetUtil.h"
#include "nsISimpleEnumerator.h"
#include "nsIAppsService.h"
#include "nsIEventTarget.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIIdleService.h"
#include "nsIInputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsInterfaceHashtable.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIOutputStream.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsISupportsPriority.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "nsThreadPool.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCID.h"
#include "PermissionRequestBase.h"
#include "ProfilerHelpers.h"
#include "prsystem.h"
#include "prtime.h"
#include "ReportInternalError.h"
#include "snappy/snappy.h"

#define DISABLE_ASSERTS_FOR_FUZZING 0

#if DISABLE_ASSERTS_FOR_FUZZING
#define ASSERT_UNLESS_FUZZING(...) do { } while (0)
#else
#define ASSERT_UNLESS_FUZZING(...) MOZ_ASSERT(false, __VA_ARGS__)
#endif

#define IDB_DEBUG_LOG(_args)                                                   \
  MOZ_LOG(IndexedDatabaseManager::GetLoggingModule(),                           \
         LogLevel::Debug,                                                         \
         _args )

#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_WIDGET_GONK)
#define IDB_MOBILE
#endif

namespace mozilla {
namespace dom {
namespace indexedDB {

using namespace mozilla::dom::quota;
using namespace mozilla::ipc;

namespace {

class ConnectionPool;
class Cursor;
class Database;
struct DatabaseActorInfo;
class DatabaseLoggingInfo;
class DatabaseFile;
class Factory;
class OpenDatabaseOp;
class TransactionBase;
class TransactionDatabaseOperationBase;
class VersionChangeTransaction;







static_assert(JS_STRUCTURED_CLONE_VERSION == 5,
              "Need to update the major schema version.");


const uint32_t kMajorSchemaVersion = 18;



const uint32_t kMinorSchemaVersion = 0;




static_assert(kMajorSchemaVersion <= 0xFFFFFFF,
              "Major version needs to fit in 28 bits.");
static_assert(kMinorSchemaVersion <= 0xF,
              "Minor version needs to fit in 4 bits.");

const int32_t kSQLiteSchemaVersion =
  int32_t((kMajorSchemaVersion << 4) + kMinorSchemaVersion);

const int32_t kStorageProgressGranularity = 1000;




const uint32_t kSQLitePageSizeOverride =
#ifdef IDB_MOBILE
  2048;
#else
  4096;
#endif

static_assert(kSQLitePageSizeOverride ==  0 ||
              (kSQLitePageSizeOverride % 2 == 0 &&
               kSQLitePageSizeOverride >= 512  &&
               kSQLitePageSizeOverride <= 65536),
              "Must be 0 (disabled) or a power of 2 between 512 and 65536!");



const int32_t kMaxWALPages = 5000; 


const uint32_t kSQLiteGrowthIncrement = kSQLitePageSizeOverride * 2;

static_assert(kSQLiteGrowthIncrement >= 0 &&
              kSQLiteGrowthIncrement % kSQLitePageSizeOverride == 0 &&
              kSQLiteGrowthIncrement < uint32_t(INT32_MAX),
              "Must be 0 (disabled) or a positive multiple of the page size!");



const uint32_t kMaxConnectionThreadCount = 20;

static_assert(kMaxConnectionThreadCount, "Must have at least one thread!");



const uint32_t kMaxIdleConnectionThreadCount = 2;

static_assert(kMaxConnectionThreadCount >= kMaxIdleConnectionThreadCount,
              "Idle thread limit must be less than total thread limit!");



const uint32_t kConnectionIdleMaintenanceMS = 2 * 1000; 



const uint32_t kConnectionIdleCloseMS = 10 * 1000; 


const uint32_t kConnectionThreadIdleMS = 30 * 1000; 

#define SAVEPOINT_CLAUSE "SAVEPOINT sp;"

const uint32_t kFileCopyBufferSize = 32768;

#define JOURNAL_DIRECTORY_NAME "journals"

const char kFileManagerDirectoryNameSuffix[] = ".files";
const char kSQLiteJournalSuffix[] = ".sqlite-journal";
const char kSQLiteSHMSuffix[] = ".sqlite-shm";
const char kSQLiteWALSuffix[] = ".sqlite-wal";

const char kPrefIndexedDBEnabled[] = "dom.indexedDB.enabled";

#define IDB_PREFIX "indexedDB"

#define PERMISSION_STRING_CHROME_BASE IDB_PREFIX "-chrome-"
#define PERMISSION_STRING_CHROME_READ_SUFFIX "-read"
#define PERMISSION_STRING_CHROME_WRITE_SUFFIX "-write"

enum AppId {
  kNoAppId = nsIScriptSecurityManager::NO_APP_ID,
  kUnknownAppId = nsIScriptSecurityManager::UNKNOWN_APP_ID
};

const char kIdleServiceContractId[] = "@mozilla.org/widget/idleservice;1";

#ifdef DEBUG

const int32_t kDEBUGThreadPriority = nsISupportsPriority::PRIORITY_NORMAL;
const uint32_t kDEBUGThreadSleepMS = 0;

const int32_t kDEBUGTransactionThreadPriority =
  nsISupportsPriority::PRIORITY_NORMAL;
const uint32_t kDEBUGTransactionThreadSleepMS = 0;

#endif

const bool kRunningXPCShellTests =
#ifdef ENABLE_TESTS
  !!PR_GetEnv("XPCSHELL_TEST_PROFILE_DIR")
#else
  false
#endif
  ;

struct FreeDeleter
{
  void
  operator()(void* aPtr) const
  {
    free(aPtr);
  }
};

template <typename T>
using UniqueFreePtr = UniquePtr<T, FreeDeleter>;

template <size_t N>
MOZ_CONSTEXPR size_t
LiteralStringLength(const char (&aArr)[N])
{
  static_assert(N, "Zero-length string literal?!");

  
  return N - 1;
}





struct FullIndexMetadata
{
  IndexMetadata mCommonMetadata;

  bool mDeleted;

public:
  FullIndexMetadata()
    : mCommonMetadata(0, nsString(), KeyPath(0), false, false)
    , mDeleted(false)
  {
    
    
    
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FullIndexMetadata)

private:
  ~FullIndexMetadata()
  { }
};

typedef nsRefPtrHashtable<nsUint64HashKey, FullIndexMetadata> IndexTable;

struct FullObjectStoreMetadata
{
  ObjectStoreMetadata mCommonMetadata;
  IndexTable mIndexes;

  
  int64_t mNextAutoIncrementId;
  int64_t mComittedAutoIncrementId;

  bool mDeleted;

public:
  FullObjectStoreMetadata()
    : mCommonMetadata(0, nsString(), KeyPath(0), false)
    , mNextAutoIncrementId(0)
    , mComittedAutoIncrementId(0)
    , mDeleted(false)
  {
    
    
    
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FullObjectStoreMetadata);

  bool
  HasLiveIndexes() const;

private:
  ~FullObjectStoreMetadata()
  { }
};

typedef nsRefPtrHashtable<nsUint64HashKey, FullObjectStoreMetadata>
  ObjectStoreTable;

struct FullDatabaseMetadata
{
  DatabaseMetadata mCommonMetadata;
  nsCString mDatabaseId;
  nsString mFilePath;
  ObjectStoreTable mObjectStores;

  int64_t mNextObjectStoreId;
  int64_t mNextIndexId;

public:
  explicit FullDatabaseMetadata(const DatabaseMetadata& aCommonMetadata)
    : mCommonMetadata(aCommonMetadata)
    , mNextObjectStoreId(0)
    , mNextIndexId(0)
  {
    AssertIsOnBackgroundThread();
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FullDatabaseMetadata)

  already_AddRefed<FullDatabaseMetadata>
  Duplicate() const;

private:
  ~FullDatabaseMetadata()
  { }
};

template <class MetadataType>
class MOZ_STACK_CLASS MetadataNameOrIdMatcher final
{
  typedef MetadataNameOrIdMatcher<MetadataType> SelfType;

  const int64_t mId;
  const nsString mName;
  nsRefPtr<MetadataType> mMetadata;
  bool mCheckName;

public:
  template <class Enumerable>
  static MetadataType*
  Match(const Enumerable& aEnumerable, uint64_t aId, const nsAString& aName)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aId);

    SelfType closure(aId, aName);
    aEnumerable.EnumerateRead(Enumerate, &closure);

    return closure.mMetadata;
  }

  template <class Enumerable>
  static MetadataType*
  Match(const Enumerable& aEnumerable, uint64_t aId)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aId);

    SelfType closure(aId);
    aEnumerable.EnumerateRead(Enumerate, &closure);

    return closure.mMetadata;
  }

private:
  MetadataNameOrIdMatcher(const int64_t& aId, const nsAString& aName)
    : mId(aId)
    , mName(PromiseFlatString(aName))
    , mMetadata(nullptr)
    , mCheckName(true)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aId);
  }

  explicit MetadataNameOrIdMatcher(const int64_t& aId)
    : mId(aId)
    , mMetadata(nullptr)
    , mCheckName(false)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aId);
  }

  static PLDHashOperator
  Enumerate(const uint64_t& aKey, MetadataType* aValue, void* aClosure)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aKey);
    MOZ_ASSERT(aValue);
    MOZ_ASSERT(aClosure);

    auto* closure = static_cast<SelfType*>(aClosure);

    if (!aValue->mDeleted &&
        (closure->mId == aValue->mCommonMetadata.id() ||
         (closure->mCheckName &&
          closure->mName == aValue->mCommonMetadata.name()))) {
      closure->mMetadata = aValue;
      return PL_DHASH_STOP;
    }

    return PL_DHASH_NEXT;
  }
};

struct IndexDataValue final
{
  int64_t mIndexId;
  Key mKey;
  bool mUnique;

  IndexDataValue()
    : mIndexId(0)
    , mUnique(false)
  {
    MOZ_COUNT_CTOR(IndexDataValue);
  }

  explicit
  IndexDataValue(const IndexDataValue& aOther)
    : mIndexId(aOther.mIndexId)
    , mKey(aOther.mKey)
    , mUnique(aOther.mUnique)
  {
    MOZ_ASSERT(!aOther.mKey.IsUnset());

    MOZ_COUNT_CTOR(IndexDataValue);
  }

  IndexDataValue(int64_t aIndexId, bool aUnique, const Key& aKey)
    : mIndexId(aIndexId)
    , mKey(aKey)
    , mUnique(aUnique)
  {
    MOZ_ASSERT(!aKey.IsUnset());

    MOZ_COUNT_CTOR(IndexDataValue);
  }

  ~IndexDataValue()
  {
    MOZ_COUNT_DTOR(IndexDataValue);
  }

  bool
  operator==(const IndexDataValue& aOther) const
  {
    return mIndexId == aOther.mIndexId &&
           mKey == aOther.mKey;
  }

  bool
  operator<(const IndexDataValue& aOther) const
  {
    if (mIndexId == aOther.mIndexId) {
      return mKey < aOther.mKey;
    }

    return mIndexId < aOther.mIndexId;
  }
};





int32_t
MakeSchemaVersion(uint32_t aMajorSchemaVersion,
                  uint32_t aMinorSchemaVersion)
{
  return int32_t((aMajorSchemaVersion << 4) + aMinorSchemaVersion);
}

uint32_t
HashName(const nsAString& aName)
{
  struct Helper
  {
    static uint32_t
    RotateBitsLeft32(uint32_t aValue, uint8_t aBits)
    {
      MOZ_ASSERT(aBits < 32);
      return (aValue << aBits) | (aValue >> (32 - aBits));
    }
  };

  static const uint32_t kGoldenRatioU32 = 0x9e3779b9u;

  const char16_t* str = aName.BeginReading();
  size_t length = aName.Length();

  uint32_t hash = 0;
  for (size_t i = 0; i < length; i++) {
    hash = kGoldenRatioU32 * (Helper::RotateBitsLeft32(hash, 5) ^ str[i]);
  }

  return hash;
}

nsresult
ClampResultCode(nsresult aResultCode)
{
  if (NS_SUCCEEDED(aResultCode) ||
      NS_ERROR_GET_MODULE(aResultCode) == NS_ERROR_MODULE_DOM_INDEXEDDB) {
    return aResultCode;
  }

  switch (aResultCode) {
    case NS_ERROR_FILE_NO_DEVICE_SPACE:
      return NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
    case NS_ERROR_STORAGE_CONSTRAINT:
      return NS_ERROR_DOM_INDEXEDDB_CONSTRAINT_ERR;
    default:
#ifdef DEBUG
      nsPrintfCString message("Converting non-IndexedDB error code (0x%X) to "
                              "NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR",
                              aResultCode);
      NS_WARNING(message.get());
#else
      ;
#endif
  }

  IDB_REPORT_INTERNAL_ERR();
  return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
}

void
GetDatabaseFilename(const nsAString& aName,
                    nsAutoString& aDatabaseFilename)
{
  MOZ_ASSERT(aDatabaseFilename.IsEmpty());

  aDatabaseFilename.AppendInt(HashName(aName));

  nsCString escapedName;
  if (!NS_Escape(NS_ConvertUTF16toUTF8(aName), escapedName, url_XPAlphas)) {
    MOZ_CRASH("Can't escape database name!");
  }

  const char* forwardIter = escapedName.BeginReading();
  const char* backwardIter = escapedName.EndReading() - 1;

  nsAutoCString substring;
  while (forwardIter <= backwardIter && substring.Length() < 21) {
    if (substring.Length() % 2) {
      substring.Append(*backwardIter--);
    } else {
      substring.Append(*forwardIter++);
    }
  }

  aDatabaseFilename.AppendASCII(substring.get(), substring.Length());
}

uint32_t
CompressedByteCountForNumber(uint64_t aNumber)
{
  MOZ_ASSERT(aNumber);

  
  uint32_t count = 1;
  while ((aNumber >>= 7)) {
    count++;
  }

  return count;
}

uint32_t
CompressedByteCountForIndexId(int64_t aIndexId)
{
  MOZ_ASSERT(aIndexId);
  MOZ_ASSERT(UINT64_MAX - uint64_t(aIndexId) >= uint64_t(aIndexId),
              "Overflow!");

  return CompressedByteCountForNumber(uint64_t(aIndexId * 2));
}

void
WriteCompressedNumber(uint64_t aNumber, uint8_t** aIterator)
{
  MOZ_ASSERT(aIterator);
  MOZ_ASSERT(*aIterator);

  uint8_t*& buffer = *aIterator;

#ifdef DEBUG
  const uint8_t* bufferStart = buffer;
  const uint64_t originalNumber = aNumber;
#endif

  while (true) {
    uint64_t shiftedNumber = aNumber >> 7;
    if (shiftedNumber) {
      *buffer++ = uint8_t(0x80 | (aNumber & 0x7f));
      aNumber = shiftedNumber;
    } else {
      *buffer++ = uint8_t(aNumber);
      break;
    }
  }

  MOZ_ASSERT(buffer > bufferStart);
  MOZ_ASSERT(uint32_t(buffer - bufferStart) ==
               CompressedByteCountForNumber(originalNumber));
}

uint64_t
ReadCompressedNumber(const uint8_t** aIterator, const uint8_t* aEnd)
{
  MOZ_ASSERT(aIterator);
  MOZ_ASSERT(*aIterator);
  MOZ_ASSERT(aEnd);
  MOZ_ASSERT(*aIterator < aEnd);

  const uint8_t*& buffer = *aIterator;

  uint8_t shiftCounter = 0;
  uint64_t result = 0;

  while (true) {
    MOZ_ASSERT(shiftCounter <= 56, "Shifted too many bits!");

    result += (uint64_t(*buffer & 0x7f) << shiftCounter);
    shiftCounter += 7;

    if (!(*buffer++ & 0x80)) {
      break;
    }

    if (NS_WARN_IF(buffer == aEnd)) {
      MOZ_ASSERT(false);
      break;
    }
  }

  return result;
}

void
WriteCompressedIndexId(int64_t aIndexId, bool aUnique, uint8_t** aIterator)
{
  MOZ_ASSERT(aIndexId);
  MOZ_ASSERT(UINT64_MAX - uint64_t(aIndexId) >= uint64_t(aIndexId),
             "Overflow!");
  MOZ_ASSERT(aIterator);
  MOZ_ASSERT(*aIterator);

  const uint64_t indexId = (uint64_t(aIndexId * 2) | (aUnique ? 1 : 0));
  WriteCompressedNumber(indexId, aIterator);
}

void
ReadCompressedIndexId(const uint8_t** aIterator,
                      const uint8_t* aEnd,
                      int64_t* aIndexId,
                      bool* aUnique)
{
  MOZ_ASSERT(aIterator);
  MOZ_ASSERT(*aIterator);
  MOZ_ASSERT(aIndexId);
  MOZ_ASSERT(aUnique);

  uint64_t indexId = ReadCompressedNumber(aIterator, aEnd);

  if (indexId % 2) {
    *aUnique = true;
    indexId--;
  } else {
    *aUnique = false;
  }

  MOZ_ASSERT(UINT64_MAX / 2 >= uint64_t(indexId), "Bad index id!");

  *aIndexId = int64_t(indexId / 2);
}


nsresult
MakeCompressedIndexDataValues(
                             const FallibleTArray<IndexDataValue>& aIndexValues,
                             UniqueFreePtr<uint8_t>& aCompressedIndexDataValues,
                             uint32_t* aCompressedIndexDataValuesLength)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(!aCompressedIndexDataValues);
  MOZ_ASSERT(aCompressedIndexDataValuesLength);

  PROFILER_LABEL("IndexedDB",
                 "MakeCompressedIndexDataValues",
                 js::ProfileEntry::Category::STORAGE);

  const uint32_t arrayLength = aIndexValues.Length();
  if (!arrayLength) {
    *aCompressedIndexDataValuesLength = 0;
    return NS_OK;
  }

  
  uint32_t blobDataLength = 0;

  for (uint32_t arrayIndex = 0; arrayIndex < arrayLength; arrayIndex++) {
    const IndexDataValue& info = aIndexValues[arrayIndex];
    const nsCString& keyBuffer = info.mKey.GetBuffer();
    const uint32_t keyBufferLength = keyBuffer.Length();

    MOZ_ASSERT(!keyBuffer.IsEmpty());

    
    if (NS_WARN_IF(UINT32_MAX - keyBuffer.Length() <
                   CompressedByteCountForIndexId(info.mIndexId) +
                   CompressedByteCountForNumber(keyBufferLength))) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    const uint32_t infoLength =
      CompressedByteCountForIndexId(info.mIndexId) +
      CompressedByteCountForNumber(keyBufferLength) +
      keyBufferLength;

    
    if (NS_WARN_IF(UINT32_MAX - infoLength < blobDataLength)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    blobDataLength += infoLength;
  }

  UniqueFreePtr<uint8_t> blobData(
    static_cast<uint8_t*>(malloc(blobDataLength)));
  if (NS_WARN_IF(!blobData)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  uint8_t* blobDataIter = blobData.get();

  for (uint32_t arrayIndex = 0; arrayIndex < arrayLength; arrayIndex++) {
    const IndexDataValue& info = aIndexValues[arrayIndex];
    const nsCString& keyBuffer = info.mKey.GetBuffer();
    const uint32_t keyBufferLength = keyBuffer.Length();

    WriteCompressedIndexId(info.mIndexId, info.mUnique, &blobDataIter);
    WriteCompressedNumber(keyBuffer.Length(), &blobDataIter);

    memcpy(blobDataIter, keyBuffer.get(), keyBufferLength);
    blobDataIter += keyBufferLength;
  }

  MOZ_ASSERT(blobDataIter == blobData.get() + blobDataLength);

  aCompressedIndexDataValues.swap(blobData);
  *aCompressedIndexDataValuesLength = uint32_t(blobDataLength);

  return NS_OK;
}

nsresult
ReadCompressedIndexDataValuesFromBlob(
                                   const uint8_t* aBlobData,
                                   uint32_t aBlobDataLength,
                                   FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aBlobData);
  MOZ_ASSERT(aBlobDataLength);
  MOZ_ASSERT(aIndexValues.IsEmpty());

  PROFILER_LABEL("IndexedDB",
                 "ReadCompressedIndexDataValuesFromBlob",
                 js::ProfileEntry::Category::STORAGE);

  const uint8_t* blobDataIter = aBlobData;
  const uint8_t* blobDataEnd = aBlobData + aBlobDataLength;

  while (blobDataIter < blobDataEnd) {
    int64_t indexId;
    bool unique;
    ReadCompressedIndexId(&blobDataIter, blobDataEnd, &indexId, &unique);

    if (NS_WARN_IF(blobDataIter == blobDataEnd)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_FILE_CORRUPTED;
    }

    
    const uint64_t keyBufferLength =
      ReadCompressedNumber(&blobDataIter, blobDataEnd);

    if (NS_WARN_IF(blobDataIter == blobDataEnd) ||
        NS_WARN_IF(keyBufferLength > uint64_t(UINT32_MAX)) ||
        NS_WARN_IF(blobDataIter + keyBufferLength > blobDataEnd)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_FILE_CORRUPTED;
    }

    nsCString keyBuffer(reinterpret_cast<const char*>(blobDataIter),
                        uint32_t(keyBufferLength));
    blobDataIter += keyBufferLength;

    if (NS_WARN_IF(!aIndexValues.InsertElementSorted(
                      IndexDataValue(indexId, unique, Key(keyBuffer)),
                      fallible))) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  MOZ_ASSERT(blobDataIter == blobDataEnd);

  return NS_OK;
}


template <typename T>
nsresult
ReadCompressedIndexDataValuesFromSource(
                                   T* aSource,
                                   uint32_t aColumnIndex,
                                   FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aSource);
  MOZ_ASSERT(aIndexValues.IsEmpty());

  int32_t columnType;
  nsresult rv = aSource->GetTypeOfIndex(aColumnIndex, &columnType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (columnType == mozIStorageStatement::VALUE_TYPE_NULL) {
    return NS_OK;
  }

  MOZ_ASSERT(columnType == mozIStorageStatement::VALUE_TYPE_BLOB);

  const uint8_t* blobData;
  uint32_t blobDataLength;
  rv = aSource->GetSharedBlob(aColumnIndex, &blobDataLength, &blobData);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!blobDataLength)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_FILE_CORRUPTED;
  }

  rv = ReadCompressedIndexDataValuesFromBlob(blobData,
                                             blobDataLength,
                                             aIndexValues);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
ReadCompressedIndexDataValues(mozIStorageStatement* aStatement,
                              uint32_t aColumnIndex,
                              FallibleTArray<IndexDataValue>& aIndexValues)
{
  return ReadCompressedIndexDataValuesFromSource(aStatement,
                                                 aColumnIndex,
                                                 aIndexValues);
}

nsresult
ReadCompressedIndexDataValues(mozIStorageValueArray* aValues,
                              uint32_t aColumnIndex,
                              FallibleTArray<IndexDataValue>& aIndexValues)
{
  return ReadCompressedIndexDataValuesFromSource(aValues,
                                                 aColumnIndex,
                                                 aIndexValues);
}

nsresult
CreateFileTables(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "CreateFileTables",
                 js::ProfileEntry::Category::STORAGE);

  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE file ("
      "id INTEGER PRIMARY KEY, "
      "refcount INTEGER NOT NULL"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_insert_trigger "
    "AFTER INSERT ON object_data "
    "FOR EACH ROW "
    "WHEN NEW.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(NULL, NEW.file_ids); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_update_trigger "
    "AFTER UPDATE OF file_ids ON object_data "
    "FOR EACH ROW "
    "WHEN OLD.file_ids IS NOT NULL OR NEW.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(OLD.file_ids, NEW.file_ids); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_delete_trigger "
    "AFTER DELETE ON object_data "
    "FOR EACH ROW WHEN OLD.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(OLD.file_ids, NULL); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER file_update_trigger "
    "AFTER UPDATE ON file "
    "FOR EACH ROW WHEN NEW.refcount = 0 "
    "BEGIN "
      "DELETE FROM file WHERE id = OLD.id; "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
CreateTables(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "CreateTables",
                 js::ProfileEntry::Category::STORAGE);

  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE database"
      "( name TEXT PRIMARY KEY"
      ", origin TEXT NOT NULL"
      ", version INTEGER NOT NULL DEFAULT 0"
      ", last_vacuum_time INTEGER NOT NULL DEFAULT 0"
      ", last_analyze_time INTEGER NOT NULL DEFAULT 0"
      ", last_vacuum_size INTEGER NOT NULL DEFAULT 0"
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store"
      "( id INTEGER PRIMARY KEY"
      ", auto_increment INTEGER NOT NULL DEFAULT 0"
      ", name TEXT NOT NULL"
      ", key_path TEXT"
      ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store_index"
      "( id INTEGER PRIMARY KEY"
      ", object_store_id INTEGER NOT NULL"
      ", name TEXT NOT NULL"
      ", key_path TEXT NOT NULL"
      ", unique_index INTEGER NOT NULL"
      ", multientry INTEGER NOT NULL"
      ", FOREIGN KEY (object_store_id) "
          "REFERENCES object_store(id) "
      ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_data"
      "( object_store_id INTEGER NOT NULL"
      ", key BLOB NOT NULL"
      ", index_data_values BLOB DEFAULT NULL"
      ", file_ids TEXT"
      ", data BLOB NOT NULL"
      ", PRIMARY KEY (object_store_id, key)"
      ", FOREIGN KEY (object_store_id) "
          "REFERENCES object_store(id) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE index_data"
      "( index_id INTEGER NOT NULL"
      ", value BLOB NOT NULL"
      ", object_data_key BLOB NOT NULL"
      ", object_store_id INTEGER NOT NULL"
      ", PRIMARY KEY (index_id, value, object_data_key)"
      ", FOREIGN KEY (index_id) "
          "REFERENCES object_store_index(id) "
      ", FOREIGN KEY (object_store_id, object_data_key) "
          "REFERENCES object_data(object_store_id, key) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE unique_index_data"
      "( index_id INTEGER NOT NULL"
      ", value BLOB NOT NULL"
      ", object_store_id INTEGER NOT NULL"
      ", object_data_key BLOB NOT NULL"
      ", PRIMARY KEY (index_id, value)"
      ", FOREIGN KEY (index_id) "
          "REFERENCES object_store_index(id) "
      ", FOREIGN KEY (object_store_id, object_data_key) "
          "REFERENCES object_data(object_store_id, key) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = CreateFileTables(aConnection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(kSQLiteSchemaVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom4To5(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom4To5",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT name, version, dataVersion "
    "FROM database"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsString name;
  int32_t intVersion;
  int64_t dataVersion;

  {
    mozStorageStatementScoper scoper(stmt);

    bool hasResults;
    rv = stmt->ExecuteStep(&hasResults);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    if (NS_WARN_IF(!hasResults)) {
      return NS_ERROR_FAILURE;
    }

    nsString version;
    rv = stmt->GetString(1, version);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    intVersion = version.ToInteger(&rv);
    if (NS_FAILED(rv)) {
      intVersion = 0;
    }

    rv = stmt->GetString(0, name);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->GetInt64(2, &dataVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE database"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE database ("
      "name TEXT NOT NULL, "
      "version INTEGER NOT NULL DEFAULT 0, "
      "dataVersion INTEGER NOT NULL"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO database (name, version, dataVersion) "
    "VALUES (:name, :version, :dataVersion)"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  {
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindStringParameter(0, name);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt32Parameter(1, intVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64Parameter(2, dataVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = aConnection->SetSchemaVersion(5);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom5To6(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom5To6",
                 js::ProfileEntry::Category::STORAGE);

  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP INDEX key_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP INDEX ai_key_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP INDEX value_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP INDEX ai_value_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id, "
      "key_value, "
      "data "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, object_store_id, key_value, data "
      "FROM object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_data ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id INTEGER NOT NULL, "
      "key_value DEFAULT NULL, "
      "data BLOB NOT NULL, "
      "UNIQUE (object_store_id, key_value), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_data "
      "SELECT id, object_store_id, key_value, data "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id, "
      "data "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, object_store_id, data "
      "FROM ai_object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_object_data ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "object_store_id INTEGER NOT NULL, "
      "data BLOB NOT NULL, "
      "UNIQUE (object_store_id, id), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO ai_object_data "
      "SELECT id, object_store_id, data "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "object_data_key, "
      "object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE index_data ("
      "index_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "object_data_key NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "PRIMARY KEY (index_id, value, object_data_key), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT OR IGNORE INTO index_data "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX index_data_object_data_id_index "
    "ON index_data (object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "object_data_key, "
      "object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE unique_index_data ("
      "index_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "object_data_key NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "PRIMARY KEY (index_id, value, object_data_key), "
      "UNIQUE (index_id, value), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO unique_index_data "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX unique_index_data_object_data_id_index "
    "ON unique_index_data (object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "ai_object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, value, ai_object_data_id "
      "FROM ai_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_index_data ("
      "index_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "ai_object_data_id INTEGER NOT NULL, "
      "PRIMARY KEY (index_id, value, ai_object_data_id), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (ai_object_data_id) REFERENCES ai_object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT OR IGNORE INTO ai_index_data "
      "SELECT index_id, value, ai_object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX ai_index_data_ai_object_data_id_index "
    "ON ai_index_data (ai_object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "ai_object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, value, ai_object_data_id "
      "FROM ai_unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_unique_index_data ("
      "index_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "ai_object_data_id INTEGER NOT NULL, "
      "UNIQUE (index_id, value), "
      "PRIMARY KEY (index_id, value, ai_object_data_id), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (ai_object_data_id) REFERENCES ai_object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO ai_unique_index_data "
      "SELECT index_id, value, ai_object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX ai_unique_index_data_ai_object_data_id_index "
    "ON ai_unique_index_data (ai_object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(6);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom6To7(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom6To7",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id, "
      "name, "
      "key_path, "
      "auto_increment"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, name, key_path, auto_increment "
      "FROM object_store;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_store;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store ("
      "id INTEGER PRIMARY KEY, "
      "auto_increment INTEGER NOT NULL DEFAULT 0, "
      "name TEXT NOT NULL, "
      "key_path TEXT, "
      "UNIQUE (name)"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_store "
      "SELECT id, auto_increment, name, nullif(key_path, '') "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(7);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom7To8(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom7To8",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id, "
      "object_store_id, "
      "name, "
      "key_path, "
      "unique_index, "
      "object_store_autoincrement"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, object_store_id, name, key_path, "
      "unique_index, object_store_autoincrement "
      "FROM object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store_index ("
      "id INTEGER, "
      "object_store_id INTEGER NOT NULL, "
      "name TEXT NOT NULL, "
      "key_path TEXT NOT NULL, "
      "unique_index INTEGER NOT NULL, "
      "multientry INTEGER NOT NULL, "
      "object_store_autoincrement INTERGER NOT NULL, "
      "PRIMARY KEY (id), "
      "UNIQUE (object_store_id, name), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_index "
      "SELECT id, object_store_id, name, key_path, "
      "unique_index, 0, object_store_autoincrement "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(8);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

class CompressDataBlobsFunction final
  : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS

private:
  ~CompressDataBlobsFunction()
  { }

  NS_IMETHOD
  OnFunctionCall(mozIStorageValueArray* aArguments,
                 nsIVariant** aResult) override
  {
    MOZ_ASSERT(aArguments);
    MOZ_ASSERT(aResult);

    PROFILER_LABEL("IndexedDB",
                   "CompressDataBlobsFunction::OnFunctionCall",
                   js::ProfileEntry::Category::STORAGE);

    uint32_t argc;
    nsresult rv = aArguments->GetNumEntries(&argc);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (argc != 1) {
      NS_WARNING("Don't call me with the wrong number of arguments!");
      return NS_ERROR_UNEXPECTED;
    }

    int32_t type;
    rv = aArguments->GetTypeOfIndex(0, &type);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (type != mozIStorageStatement::VALUE_TYPE_BLOB) {
      NS_WARNING("Don't call me with the wrong type of arguments!");
      return NS_ERROR_UNEXPECTED;
    }

    const uint8_t* uncompressed;
    uint32_t uncompressedLength;
    rv = aArguments->GetSharedBlob(0, &uncompressedLength, &uncompressed);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    size_t compressedLength = snappy::MaxCompressedLength(uncompressedLength);
    nsAutoArrayPtr<char> compressed(new (fallible) char[compressedLength]);
    if (NS_WARN_IF(!compressed)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    snappy::RawCompress(reinterpret_cast<const char*>(uncompressed),
                        uncompressedLength, compressed.get(),
                        &compressedLength);

    std::pair<const void *, int> data(static_cast<void*>(compressed.get()),
                                      int(compressedLength));

    
    
    nsCOMPtr<nsIVariant> result = new mozilla::storage::BlobVariant(data);

    result.forget(aResult);
    return NS_OK;
  }
};

nsresult
UpgradeSchemaFrom8To9_0(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom8To9_0",
                 js::ProfileEntry::Category::STORAGE);

  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE database SET dataVersion = 0;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageFunction> compressor = new CompressDataBlobsFunction();

  NS_NAMED_LITERAL_CSTRING(compressorName, "compress");

  rv = aConnection->CreateFunction(compressorName, 1, compressor);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE object_data SET data = compress(data);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE ai_object_data SET data = compress(data);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->RemoveFunction(compressorName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(9, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom9_0To10_0(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom9_0To10_0",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE object_data ADD COLUMN file_ids TEXT;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE ai_object_data ADD COLUMN file_ids TEXT;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = CreateFileTables(aConnection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(10, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom10_0To11_0(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom10_0To11_0",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id, "
      "object_store_id, "
      "name, "
      "key_path, "
      "unique_index, "
      "multientry"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, object_store_id, name, key_path, "
      "unique_index, multientry "
      "FROM object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store_index ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id INTEGER NOT NULL, "
      "name TEXT NOT NULL, "
      "key_path TEXT NOT NULL, "
      "unique_index INTEGER NOT NULL, "
      "multientry INTEGER NOT NULL, "
      "UNIQUE (object_store_id, name), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_index "
      "SELECT id, object_store_id, name, key_path, "
      "unique_index, multientry "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TRIGGER object_data_insert_trigger;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_data (object_store_id, key_value, data, file_ids) "
      "SELECT object_store_id, id, data, file_ids "
      "FROM ai_object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_insert_trigger "
    "AFTER INSERT ON object_data "
    "FOR EACH ROW "
    "WHEN NEW.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(NULL, NEW.file_ids); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO index_data (index_id, value, object_data_key, object_data_id) "
      "SELECT ai_index_data.index_id, ai_index_data.value, ai_index_data.ai_object_data_id, object_data.id "
      "FROM ai_index_data "
      "INNER JOIN object_store_index ON "
        "object_store_index.id = ai_index_data.index_id "
      "INNER JOIN object_data ON "
        "object_data.object_store_id = object_store_index.object_store_id AND "
        "object_data.key_value = ai_index_data.ai_object_data_id;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO unique_index_data (index_id, value, object_data_key, object_data_id) "
      "SELECT ai_unique_index_data.index_id, ai_unique_index_data.value, ai_unique_index_data.ai_object_data_id, object_data.id "
      "FROM ai_unique_index_data "
      "INNER JOIN object_store_index ON "
        "object_store_index.id = ai_unique_index_data.index_id "
      "INNER JOIN object_data ON "
        "object_data.object_store_id = object_store_index.object_store_id AND "
        "object_data.key_value = ai_unique_index_data.ai_object_data_id;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE object_store "
      "SET auto_increment = (SELECT max(id) FROM ai_object_data) + 1 "
      "WHERE auto_increment;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE ai_object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(11, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

class EncodeKeysFunction final
  : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS

private:
  ~EncodeKeysFunction()
  { }

  NS_IMETHOD
  OnFunctionCall(mozIStorageValueArray* aArguments,
                 nsIVariant** aResult) override
  {
    MOZ_ASSERT(aArguments);
    MOZ_ASSERT(aResult);

    PROFILER_LABEL("IndexedDB",
                   "EncodeKeysFunction::OnFunctionCall",
                   js::ProfileEntry::Category::STORAGE);

    uint32_t argc;
    nsresult rv = aArguments->GetNumEntries(&argc);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (argc != 1) {
      NS_WARNING("Don't call me with the wrong number of arguments!");
      return NS_ERROR_UNEXPECTED;
    }

    int32_t type;
    rv = aArguments->GetTypeOfIndex(0, &type);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    Key key;
    if (type == mozIStorageStatement::VALUE_TYPE_INTEGER) {
      int64_t intKey;
      aArguments->GetInt64(0, &intKey);
      key.SetFromInteger(intKey);
    } else if (type == mozIStorageStatement::VALUE_TYPE_TEXT) {
      nsString stringKey;
      aArguments->GetString(0, stringKey);
      key.SetFromString(stringKey);
    } else {
      NS_WARNING("Don't call me with the wrong type of arguments!");
      return NS_ERROR_UNEXPECTED;
    }

    const nsCString& buffer = key.GetBuffer();

    std::pair<const void *, int> data(static_cast<const void*>(buffer.get()),
                                      int(buffer.Length()));

    nsCOMPtr<nsIVariant> result = new mozilla::storage::BlobVariant(data);

    result.forget(aResult);
    return NS_OK;
  }
};

nsresult
UpgradeSchemaFrom11_0To12_0(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom11_0To12_0",
                 js::ProfileEntry::Category::STORAGE);

  NS_NAMED_LITERAL_CSTRING(encoderName, "encode");

  nsCOMPtr<mozIStorageFunction> encoder = new EncodeKeysFunction();

  nsresult rv = aConnection->CreateFunction(encoderName, 1, encoder);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id, "
      "key_value, "
      "data, "
      "file_ids "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT id, object_store_id, encode(key_value), data, file_ids "
      "FROM object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_data ("
      "id INTEGER PRIMARY KEY, "
      "object_store_id INTEGER NOT NULL, "
      "key_value BLOB DEFAULT NULL, "
      "file_ids TEXT, "
      "data BLOB NOT NULL, "
      "UNIQUE (object_store_id, key_value), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_data "
      "SELECT id, object_store_id, key_value, file_ids, data "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_insert_trigger "
    "AFTER INSERT ON object_data "
    "FOR EACH ROW "
    "WHEN NEW.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(NULL, NEW.file_ids); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_update_trigger "
    "AFTER UPDATE OF file_ids ON object_data "
    "FOR EACH ROW "
    "WHEN OLD.file_ids IS NOT NULL OR NEW.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(OLD.file_ids, NEW.file_ids); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_delete_trigger "
    "AFTER DELETE ON object_data "
    "FOR EACH ROW WHEN OLD.file_ids IS NOT NULL "
    "BEGIN "
      "SELECT update_refcount(OLD.file_ids, NULL); "
    "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "object_data_key, "
      "object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, encode(value), encode(object_data_key), object_data_id "
      "FROM index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE index_data ("
      "index_id INTEGER NOT NULL, "
      "value BLOB NOT NULL, "
      "object_data_key BLOB NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "PRIMARY KEY (index_id, value, object_data_key), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO index_data "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX index_data_object_data_id_index "
    "ON index_data (object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TABLE temp_upgrade ("
      "index_id, "
      "value, "
      "object_data_key, "
      "object_data_id "
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO temp_upgrade "
      "SELECT index_id, encode(value), encode(object_data_key), object_data_id "
      "FROM unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE unique_index_data ("
      "index_id INTEGER NOT NULL, "
      "value BLOB NOT NULL, "
      "object_data_key BLOB NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "PRIMARY KEY (index_id, value, object_data_key), "
      "UNIQUE (index_id, value), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO unique_index_data "
      "SELECT index_id, value, object_data_key, object_data_id "
      "FROM temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE temp_upgrade;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX unique_index_data_object_data_id_index "
    "ON unique_index_data (object_data_id);"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->RemoveFunction(encoderName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(12, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom12_0To13_0(mozIStorageConnection* aConnection,
                            bool* aVacuumNeeded)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom12_0To13_0",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

#ifdef IDB_MOBILE
  int32_t defaultPageSize;
  rv = aConnection->GetDefaultPageSize(&defaultPageSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  nsAutoCString upgradeQuery("PRAGMA auto_vacuum = FULL; PRAGMA page_size = ");
  upgradeQuery.AppendInt(defaultPageSize);

  rv = aConnection->ExecuteSimpleSQL(upgradeQuery);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  *aVacuumNeeded = true;
#endif

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(13, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom13_0To14_0(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);

  
  
  nsresult rv = aConnection->SetSchemaVersion(MakeSchemaVersion(14, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom14_0To15_0(mozIStorageConnection* aConnection)
{
  
  
  nsresult rv = aConnection->SetSchemaVersion(MakeSchemaVersion(15, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom15_0To16_0(mozIStorageConnection* aConnection)
{
  
  
  nsresult rv = aConnection->SetSchemaVersion(MakeSchemaVersion(16, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom16_0To17_0(mozIStorageConnection* aConnection)
{
  
  
  nsresult rv = aConnection->SetSchemaVersion(MakeSchemaVersion(17, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

class UpgradeSchemaFrom17_0To18_0Helper final
{
  class InsertIndexDataValuesFunction;
  class UpgradeKeyFunction;

public:
  static nsresult
  DoUpgrade(mozIStorageConnection* aConnection, const nsACString& aOrigin);

private:
  static nsresult
  DoUpgradeInternal(mozIStorageConnection* aConnection,
                    const nsACString& aOrigin);

  UpgradeSchemaFrom17_0To18_0Helper()
  {
    MOZ_ASSERT_UNREACHABLE("Don't create instances of this class!");
  }

  ~UpgradeSchemaFrom17_0To18_0Helper()
  {
    MOZ_ASSERT_UNREACHABLE("Don't create instances of this class!");
  }
};

class UpgradeSchemaFrom17_0To18_0Helper::InsertIndexDataValuesFunction final
  : public mozIStorageFunction
{
public:
  InsertIndexDataValuesFunction()
  { }

  NS_DECL_ISUPPORTS

private:
  ~InsertIndexDataValuesFunction()
  { }

  NS_DECL_MOZISTORAGEFUNCTION
};

NS_IMPL_ISUPPORTS(UpgradeSchemaFrom17_0To18_0Helper::
                    InsertIndexDataValuesFunction,
                  mozIStorageFunction);

NS_IMETHODIMP
UpgradeSchemaFrom17_0To18_0Helper::
InsertIndexDataValuesFunction::OnFunctionCall(mozIStorageValueArray* aValues,
                                              nsIVariant** _retval)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aValues);
  MOZ_ASSERT(_retval);

#ifdef DEBUG
  {
    uint32_t argCount;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetNumEntries(&argCount)));
    MOZ_ASSERT(argCount == 4);

    int32_t valueType;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(0, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_NULL ||
               valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(1, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_INTEGER);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(2, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_INTEGER);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(3, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);
  }
#endif

  
  
  AutoFallibleTArray<IndexDataValue, 32> indexValues;
  nsresult rv = ReadCompressedIndexDataValues(aValues, 0, indexValues);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int64_t indexId;
  rv = aValues->GetInt64(1, &indexId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int32_t unique;
  rv = aValues->GetInt32(2, &unique);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  Key value;
  rv = value.SetFromValueArray(aValues, 3);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (NS_WARN_IF(!indexValues.SetCapacity(indexValues.Length() + 1,
                                          fallible))) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  MOZ_ALWAYS_TRUE(
    indexValues.InsertElementSorted(IndexDataValue(indexId, !!unique, value),
                                    fallible));

  
  UniqueFreePtr<uint8_t> indexValuesBlob;
  uint32_t indexValuesBlobLength;
  rv = MakeCompressedIndexDataValues(indexValues,
                                     indexValuesBlob,
                                     &indexValuesBlobLength);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  std::pair<uint8_t *, int> indexValuesBlobPair(indexValuesBlob.release(),
                                                indexValuesBlobLength);

  nsCOMPtr<nsIVariant> result =
    new storage::AdoptedBlobVariant(indexValuesBlobPair);

  result.forget(_retval);
  return NS_OK;
}

class UpgradeSchemaFrom17_0To18_0Helper::UpgradeKeyFunction final
  : public mozIStorageFunction
{
public:
  UpgradeKeyFunction()
  { }

  static nsresult
  CopyAndUpgradeKeyBuffer(const uint8_t* aSource,
                          const uint8_t* aSourceEnd,
                          uint8_t* aDestination)
  {
    return CopyAndUpgradeKeyBufferInternal(aSource,
                                           aSourceEnd,
                                           aDestination,
                                           0 ,
                                           0 );
  }

  NS_DECL_ISUPPORTS

private:
  ~UpgradeKeyFunction()
  { }

  static nsresult
  CopyAndUpgradeKeyBufferInternal(const uint8_t*& aSource,
                                  const uint8_t* aSourceEnd,
                                  uint8_t*& aDestination,
                                  uint8_t aTagOffset,
                                  uint8_t aRecursionDepth);

  static uint32_t
  AdjustedSize(uint32_t aMaxSize,
               const uint8_t* aSource,
               const uint8_t* aSourceEnd)
  {
    MOZ_ASSERT(aMaxSize);
    MOZ_ASSERT(aSource);
    MOZ_ASSERT(aSourceEnd);
    MOZ_ASSERT(aSource <= aSourceEnd);

    return std::min(aMaxSize, uint32_t(aSourceEnd - aSource));
  }

  NS_DECL_MOZISTORAGEFUNCTION
};


nsresult
UpgradeSchemaFrom17_0To18_0Helper::
UpgradeKeyFunction::CopyAndUpgradeKeyBufferInternal(const uint8_t*& aSource,
                                                    const uint8_t* aSourceEnd,
                                                    uint8_t*& aDestination,
                                                    uint8_t aTagOffset,
                                                    uint8_t aRecursionDepth)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aSource);
  MOZ_ASSERT(*aSource);
  MOZ_ASSERT(aSourceEnd);
  MOZ_ASSERT(aSource < aSourceEnd);
  MOZ_ASSERT(aDestination);
  MOZ_ASSERT(aTagOffset <=  Key::kMaxArrayCollapse);

  static MOZ_CONSTEXPR_VAR uint8_t kOldNumberTag = 0x1;
  static MOZ_CONSTEXPR_VAR uint8_t kOldDateTag = 0x2;
  static MOZ_CONSTEXPR_VAR uint8_t kOldStringTag = 0x3;
  static MOZ_CONSTEXPR_VAR uint8_t kOldArrayTag = 0x4;
  static MOZ_CONSTEXPR_VAR uint8_t kOldMaxType = kOldArrayTag;

  if (NS_WARN_IF(aRecursionDepth > Key::kMaxRecursionDepth)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_FILE_CORRUPTED;
  }

  const uint8_t sourceTag = *aSource - (aTagOffset * kOldMaxType);
  MOZ_ASSERT(sourceTag);

  if (NS_WARN_IF(sourceTag > kOldMaxType * Key::kMaxArrayCollapse)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_FILE_CORRUPTED;
  }

  if (sourceTag == kOldNumberTag || sourceTag == kOldDateTag) {
    
    *aDestination++ =
      (sourceTag == kOldNumberTag ? Key::eFloat : Key::eDate) +
      (aTagOffset * Key::eMaxType);
    aSource++;

    
    
    const uint32_t byteCount =
      AdjustedSize(sizeof(uint64_t), aSource, aSourceEnd);

    for (uint32_t count = 0; count < byteCount; count++) {
      *aDestination++ = *aSource++;
    }

    return NS_OK;
  }

  if (sourceTag == kOldStringTag) {
    
    *aDestination++ = Key::eString + (aTagOffset * Key::eMaxType);
    aSource++;

    while (aSource < aSourceEnd) {
      const uint8_t byte = *aSource++;
      *aDestination++ = byte;

      if (!byte) {
        
        break;
      }

      
      
      if (byte & 0x80) {
        const uint32_t byteCount =
          AdjustedSize((byte & 0x40) ? 2 : 1, aSource, aSourceEnd);

        for (uint32_t count = 0; count < byteCount; count++) {
          *aDestination++ = *aSource++;
        }
      }
    }

    return NS_OK;
  }

  if (NS_WARN_IF(sourceTag < kOldArrayTag)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_FILE_CORRUPTED;
  }

  aTagOffset++;

  if (aTagOffset == Key::kMaxArrayCollapse) {
    MOZ_ASSERT(sourceTag == kOldArrayTag);

    *aDestination++ = (aTagOffset * Key::eMaxType);
    aSource++;

    aTagOffset = 0;
  }

  while (aSource < aSourceEnd &&
         (*aSource - (aTagOffset * kOldMaxType)) != Key::eTerminator) {
    nsresult rv = CopyAndUpgradeKeyBufferInternal(aSource,
                                                  aSourceEnd,
                                                  aDestination,
                                                  aTagOffset,
                                                  aRecursionDepth + 1);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    aTagOffset = 0;
  }

  if (aSource < aSourceEnd) {
    MOZ_ASSERT((*aSource - (aTagOffset * kOldMaxType)) == Key::eTerminator);
    *aDestination++ = Key::eTerminator + (aTagOffset * Key::eMaxType);
    aSource++;
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS(UpgradeSchemaFrom17_0To18_0Helper::UpgradeKeyFunction,
                  mozIStorageFunction);

NS_IMETHODIMP
UpgradeSchemaFrom17_0To18_0Helper::
UpgradeKeyFunction::OnFunctionCall(mozIStorageValueArray* aValues,
                                   nsIVariant** _retval)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aValues);
  MOZ_ASSERT(_retval);

#ifdef DEBUG
  {
    uint32_t argCount;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetNumEntries(&argCount)));
    MOZ_ASSERT(argCount == 1);

    int32_t valueType;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(0, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);
  }
#endif

  
  const uint8_t* blobData;
  uint32_t blobDataLength;
  nsresult rv = aValues->GetSharedBlob(0, &blobDataLength, &blobData);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  UniqueFreePtr<uint8_t> upgradedBlobData(
    static_cast<uint8_t*>(malloc(blobDataLength)));
  if (NS_WARN_IF(!upgradedBlobData)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = CopyAndUpgradeKeyBuffer(blobData,
                               blobData + blobDataLength,
                               upgradedBlobData.get());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  std::pair<uint8_t*, int> data(upgradedBlobData.release(),
                                int(blobDataLength));

  nsCOMPtr<nsIVariant> result = new mozilla::storage::AdoptedBlobVariant(data);

  upgradedBlobData.release();

  result.forget(_retval);
  return NS_OK;
}


nsresult
UpgradeSchemaFrom17_0To18_0Helper::DoUpgrade(mozIStorageConnection* aConnection,
                                             const nsACString& aOrigin)
{
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(!aOrigin.IsEmpty());

  
  nsRefPtr<UpgradeKeyFunction> updateFunction = new UpgradeKeyFunction();

  NS_NAMED_LITERAL_CSTRING(upgradeKeyFunctionName, "upgrade_key");

  nsresult rv =
    aConnection->CreateFunction(upgradeKeyFunctionName, 1, updateFunction);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  nsRefPtr<InsertIndexDataValuesFunction> insertIDVFunction =
    new InsertIndexDataValuesFunction();

  NS_NAMED_LITERAL_CSTRING(insertIDVFunctionName, "insert_idv");

  rv = aConnection->CreateFunction(insertIDVFunctionName, 4, insertIDVFunction);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->RemoveFunction(upgradeKeyFunctionName)));
    return rv;
  }

  rv = DoUpgradeInternal(aConnection, aOrigin);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aConnection->RemoveFunction(upgradeKeyFunctionName)));
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aConnection->RemoveFunction(insertIDVFunctionName)));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}


nsresult
UpgradeSchemaFrom17_0To18_0Helper::DoUpgradeInternal(
                                             mozIStorageConnection* aConnection,
                                             const nsACString& aOrigin)
{
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(!aOrigin.IsEmpty());

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TRIGGER object_data_insert_trigger;"
    "DROP TRIGGER object_data_update_trigger;"
    "DROP TRIGGER object_data_delete_trigger;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP INDEX index_data_object_data_id_index;"
    "DROP INDEX unique_index_data_object_data_id_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "CREATE TABLE database_upgrade "
      "( name TEXT PRIMARY KEY"
      ", origin TEXT NOT NULL"
      ", version INTEGER NOT NULL DEFAULT 0"
      ", last_vacuum_time INTEGER NOT NULL DEFAULT 0"
      ", last_analyze_time INTEGER NOT NULL DEFAULT 0"
      ", last_vacuum_size INTEGER NOT NULL DEFAULT 0"
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
     
    "CREATE TABLE object_store_upgrade"
      "( id INTEGER PRIMARY KEY"
      ", auto_increment INTEGER NOT NULL DEFAULT 0"
      ", name TEXT NOT NULL"
      ", key_path TEXT"
      ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "CREATE TABLE object_store_index_upgrade"
      "( id INTEGER PRIMARY KEY"
      ", object_store_id INTEGER NOT NULL"
      ", name TEXT NOT NULL"
      ", key_path TEXT NOT NULL"
      ", unique_index INTEGER NOT NULL"
      ", multientry INTEGER NOT NULL"
      ", FOREIGN KEY (object_store_id) "
          "REFERENCES object_store(id) "
      ");"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "CREATE TABLE object_data_upgrade"
      "( object_store_id INTEGER NOT NULL"
      ", key BLOB NOT NULL"
      ", index_data_values BLOB DEFAULT NULL"
      ", file_ids TEXT"
      ", data BLOB NOT NULL"
      ", PRIMARY KEY (object_store_id, key)"
      ", FOREIGN KEY (object_store_id) "
          "REFERENCES object_store(id) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "CREATE TABLE index_data_upgrade"
      "( index_id INTEGER NOT NULL"
      ", value BLOB NOT NULL"
      ", object_data_key BLOB NOT NULL"
      ", object_store_id INTEGER NOT NULL"
      ", PRIMARY KEY (index_id, value, object_data_key)"
      ", FOREIGN KEY (index_id) "
          "REFERENCES object_store_index(id) "
      ", FOREIGN KEY (object_store_id, object_data_key) "
          "REFERENCES object_data(object_store_id, key) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "CREATE TABLE unique_index_data_upgrade"
      "( index_id INTEGER NOT NULL"
      ", value BLOB NOT NULL"
      ", object_store_id INTEGER NOT NULL"
      ", object_data_key BLOB NOT NULL"
      ", PRIMARY KEY (index_id, value)"
      ", FOREIGN KEY (index_id) "
          "REFERENCES object_store_index(id) "
      ", FOREIGN KEY (object_store_id, object_data_key) "
          "REFERENCES object_data(object_store_id, key) "
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    
    "CREATE TEMPORARY TABLE temp_index_data_values "
      "( object_store_id INTEGER NOT NULL"
      ", key BLOB NOT NULL"
      ", index_data_values BLOB DEFAULT NULL"
      ", PRIMARY KEY (object_store_id, key)"
      ") WITHOUT ROWID;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    
    "CREATE TEMPORARY TRIGGER unique_index_data_upgrade_insert_trigger "
      "AFTER INSERT ON unique_index_data_upgrade "
      "BEGIN "
        "INSERT OR REPLACE INTO temp_index_data_values "
          "VALUES "
          "( NEW.object_store_id"
          ", NEW.object_data_key"
          ", insert_idv("
              "( SELECT index_data_values "
                  "FROM temp_index_data_values "
                  "WHERE object_store_id = NEW.object_store_id "
                  "AND key = NEW.object_data_key "
              "), NEW.index_id"
               ", 1" 
               ", NEW.value"
            ")"
          ");"
      "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TEMPORARY TRIGGER index_data_upgrade_insert_trigger "
      "AFTER INSERT ON index_data_upgrade "
      "BEGIN "
        "INSERT OR REPLACE INTO temp_index_data_values "
          "VALUES "
          "( NEW.object_store_id"
          ", NEW.object_data_key"
          ", insert_idv("
              "("
                "SELECT index_data_values "
                  "FROM temp_index_data_values "
                  "WHERE object_store_id = NEW.object_store_id "
                  "AND key = NEW.object_data_key "
              "), NEW.index_id"
               ", 0" 
               ", NEW.value"
            ")"
          ");"
      "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "INSERT INTO unique_index_data_upgrade "
      "SELECT "
        "unique_index_data.index_id, "
        "upgrade_key(unique_index_data.value), "
        "object_data.object_store_id, "
        "upgrade_key(unique_index_data.object_data_key) "
        "FROM unique_index_data "
        "JOIN object_data "
        "ON unique_index_data.object_data_id = object_data.id;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TRIGGER unique_index_data_upgrade_insert_trigger;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TABLE unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "ALTER TABLE unique_index_data_upgrade "
      "RENAME TO unique_index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "INSERT INTO index_data_upgrade "
      "SELECT "
        "index_data.index_id, "
        "upgrade_key(index_data.value), "
        "upgrade_key(index_data.object_data_key), "
        "object_data.object_store_id "
        "FROM index_data "
        "JOIN object_data "
        "ON index_data.object_data_id = object_data.id;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TRIGGER index_data_upgrade_insert_trigger;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TABLE index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "ALTER TABLE index_data_upgrade "
      "RENAME TO index_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "INSERT INTO object_data_upgrade "
      "SELECT "
        "object_data.object_store_id, "
        "upgrade_key(object_data.key_value), "
        "temp_index_data_values.index_data_values, "
        "object_data.file_ids, "
        "object_data.data "
        "FROM object_data "
        "LEFT JOIN temp_index_data_values "
        "ON object_data.object_store_id = "
          "temp_index_data_values.object_store_id "
        "AND upgrade_key(object_data.key_value) = "
          "temp_index_data_values.key;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TABLE temp_index_data_values;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "DROP TABLE object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    
    "ALTER TABLE object_data_upgrade "
      "RENAME TO object_data;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_index_upgrade "
      "SELECT * "
        "FROM object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE object_store_index_upgrade "
      "RENAME TO object_store_index;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_upgrade "
      "SELECT * "
        "FROM object_store;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE object_store;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE object_store_upgrade "
      "RENAME TO object_store;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO database_upgrade "
      "SELECT name, :origin, version, 0, 0, 0 "
        "FROM database;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("origin"), aOrigin);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE database;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE database_upgrade "
      "RENAME TO database;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef DEBUG
  {
    
    nsCOMPtr<mozIStorageStatement> stmt;
    MOZ_ASSERT(NS_SUCCEEDED(
      aConnection->CreateStatement(
        NS_LITERAL_CSTRING("SELECT COUNT(*) "
                             "FROM database;"),
        getter_AddRefs(stmt))));

    bool hasResult;
    MOZ_ASSERT(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));

    int64_t count;
    MOZ_ASSERT(NS_SUCCEEDED(stmt->GetInt64(0, &count)));

    MOZ_ASSERT(count == 1);
  }
#endif

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_insert_trigger "
      "AFTER INSERT ON object_data "
      "WHEN NEW.file_ids IS NOT NULL "
      "BEGIN "
        "SELECT update_refcount(NULL, NEW.file_ids);"
      "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_update_trigger "
      "AFTER UPDATE OF file_ids ON object_data "
      "WHEN OLD.file_ids IS NOT NULL OR NEW.file_ids IS NOT NULL "
      "BEGIN "
        "SELECT update_refcount(OLD.file_ids, NEW.file_ids);"
      "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TRIGGER object_data_delete_trigger "
      "AFTER DELETE ON object_data "
      "WHEN OLD.file_ids IS NOT NULL "
      "BEGIN "
        "SELECT update_refcount(OLD.file_ids, NULL);"
      "END;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  rv = aConnection->ExecuteSimpleSQL(
#ifdef IDB_MOBILE
    NS_LITERAL_CSTRING("PRAGMA auto_vacuum = FULL;")
#else
    NS_LITERAL_CSTRING("PRAGMA auto_vacuum = INCREMENTAL;")
#endif
  );
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aConnection->SetSchemaVersion(MakeSchemaVersion(18, 0));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UpgradeSchemaFrom17_0To18_0(mozIStorageConnection* aConnection,
                            const nsACString& aOrigin)
{
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(!aOrigin.IsEmpty());

  PROFILER_LABEL("IndexedDB",
                 "UpgradeSchemaFrom17_0To18_0",
                 js::ProfileEntry::Category::STORAGE);

  return UpgradeSchemaFrom17_0To18_0Helper::DoUpgrade(aConnection, aOrigin);
}

nsresult
GetDatabaseFileURL(nsIFile* aDatabaseFile,
                   PersistenceType aPersistenceType,
                   const nsACString& aGroup,
                   const nsACString& aOrigin,
                   uint32_t aTelemetryId,
                   nsIFileURL** aResult)
{
  MOZ_ASSERT(aDatabaseFile);
  MOZ_ASSERT(aResult);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewFileURI(getter_AddRefs(uri), aDatabaseFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri);
  MOZ_ASSERT(fileUrl);

  nsAutoCString type;
  PersistenceTypeToText(aPersistenceType, type);

  nsAutoCString telemetryFilenameClause;
  if (aTelemetryId) {
    telemetryFilenameClause.AssignLiteral("&telemetryFilename=indexedDB-");
    telemetryFilenameClause.AppendInt(aTelemetryId);
    telemetryFilenameClause.AppendLiteral(".sqlite");
  }

  rv = fileUrl->SetQuery(NS_LITERAL_CSTRING("persistenceType=") + type +
                         NS_LITERAL_CSTRING("&group=") + aGroup +
                         NS_LITERAL_CSTRING("&origin=") + aOrigin +
                         NS_LITERAL_CSTRING("&cache=private") +
                         telemetryFilenameClause);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  fileUrl.forget(aResult);
  return NS_OK;
}

nsresult
SetDefaultPragmas(mozIStorageConnection* aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConnection);

  static const char kBuiltInPragmas[] =
    
    
   "PRAGMA foreign_keys = "
#ifdef DEBUG
     "ON"
#else
     "OFF"
#endif
     ";"

    
    
    
    
    
    "PRAGMA recursive_triggers = ON;"

    
    
    "PRAGMA secure_delete = OFF;"
  ;

  nsresult rv =
    aConnection->ExecuteSimpleSQL(
      nsDependentCString(kBuiltInPragmas,
                         LiteralStringLength(kBuiltInPragmas)));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString pragmaStmt;
  pragmaStmt.AssignLiteral("PRAGMA synchronous = ");

  if (IndexedDatabaseManager::FullSynchronous()) {
    pragmaStmt.AppendLiteral("FULL");
  } else {
    pragmaStmt.AppendLiteral("NORMAL");
  }
  pragmaStmt.Append(';');

  rv = aConnection->ExecuteSimpleSQL(pragmaStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifndef IDB_MOBILE
  if (kSQLiteGrowthIncrement) {
    
    
    rv = aConnection->SetGrowthIncrement(kSQLiteGrowthIncrement,
                                         EmptyCString());
    if (rv != NS_ERROR_FILE_TOO_BIG && NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
#endif 

  return NS_OK;
}

nsresult
SetJournalMode(mozIStorageConnection* aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConnection);

  
  
  NS_NAMED_LITERAL_CSTRING(journalModeQueryStart, "PRAGMA journal_mode = ");
  NS_NAMED_LITERAL_CSTRING(journalModeWAL, "wal");

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv =
    aConnection->CreateStatement(journalModeQueryStart + journalModeWAL,
                                 getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  nsCString journalMode;
  rv = stmt->GetUTF8String(0, journalMode);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (journalMode.Equals(journalModeWAL)) {
    
    if (kMaxWALPages >= 0) {
      nsAutoCString pageCount;
      pageCount.AppendInt(kMaxWALPages);

      rv = aConnection->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("PRAGMA wal_autocheckpoint = ") + pageCount);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  } else {
    NS_WARNING("Failed to set WAL mode, falling back to normal journal mode.");
#ifdef IDB_MOBILE
    rv = aConnection->ExecuteSimpleSQL(journalModeQueryStart +
                                       NS_LITERAL_CSTRING("truncate"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
#endif
  }

  return NS_OK;
}

template <class FileOrURLType>
struct StorageOpenTraits;

template <>
struct StorageOpenTraits<nsIFileURL*>
{
  static nsresult
  Open(mozIStorageService* aStorageService,
       nsIFileURL* aFileURL,
       mozIStorageConnection** aConnection)
  {
    return aStorageService->OpenDatabaseWithFileURL(aFileURL, aConnection);
  }

#ifdef DEBUG
  static void
  GetPath(nsIFileURL* aFileURL, nsCString& aPath)
  {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aFileURL->GetFileName(aPath)));
  }
#endif
};

template <>
struct StorageOpenTraits<nsIFile*>
{
  static nsresult
  Open(mozIStorageService* aStorageService,
       nsIFile* aFile,
       mozIStorageConnection** aConnection)
  {
    return aStorageService->OpenUnsharedDatabase(aFile, aConnection);
  }

#ifdef DEBUG
  static void
  GetPath(nsIFile* aFile, nsCString& aPath)
  {
    nsString path;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aFile->GetPath(path)));

    aPath.AssignWithConversion(path);
  }
#endif
};

template <template <class> class SmartPtr, class FileOrURLType>
struct StorageOpenTraits<SmartPtr<FileOrURLType>>
  : public StorageOpenTraits<FileOrURLType*>
{ };

template <class FileOrURLType>
nsresult
OpenDatabaseAndHandleBusy(mozIStorageService* aStorageService,
                          FileOrURLType aFileOrURL,
                          mozIStorageConnection** aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aStorageService);
  MOZ_ASSERT(aFileOrURL);
  MOZ_ASSERT(aConnection);

  nsCOMPtr<mozIStorageConnection> connection;
  nsresult rv =
    StorageOpenTraits<FileOrURLType>::Open(aStorageService,
                                           aFileOrURL,
                                           getter_AddRefs(connection));

  if (rv == NS_ERROR_STORAGE_BUSY) {
#ifdef DEBUG
    {
      nsCString path;
      StorageOpenTraits<FileOrURLType>::GetPath(aFileOrURL, path);

      nsPrintfCString message("Received NS_ERROR_STORAGE_BUSY when attempting "
                              "to open database '%s', retrying for up to 10 "
                              "seconds",
                              path.get());
      NS_WARNING(message.get());
    }
#endif

    
    
    TimeStamp start = TimeStamp::NowLoRes();

    while (true) {
      PR_Sleep(PR_MillisecondsToInterval(100));

      rv = StorageOpenTraits<FileOrURLType>::Open(aStorageService,
                                                  aFileOrURL,
                                                  getter_AddRefs(connection));
      if (rv != NS_ERROR_STORAGE_BUSY ||
          TimeStamp::NowLoRes() - start > TimeDuration::FromSeconds(10)) {
        break;
      }
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  connection.forget(aConnection);
  return NS_OK;
}

nsresult
CreateStorageConnection(nsIFile* aDBFile,
                        nsIFile* aFMDirectory,
                        const nsAString& aName,
                        PersistenceType aPersistenceType,
                        const nsACString& aGroup,
                        const nsACString& aOrigin,
                        uint32_t aTelemetryId,
                        mozIStorageConnection** aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDBFile);
  MOZ_ASSERT(aFMDirectory);
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "CreateStorageConnection",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;
  bool exists;

  if (IndexedDatabaseManager::InLowDiskSpaceMode()) {
    rv = aDBFile->Exists(&exists);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!exists) {
      NS_WARNING("Refusing to create database because disk space is low!");
      return NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
    }
  }

  nsCOMPtr<nsIFileURL> dbFileUrl;
  rv = GetDatabaseFileURL(aDBFile,
                          aPersistenceType,
                          aGroup,
                          aOrigin,
                          aTelemetryId,
                          getter_AddRefs(dbFileUrl));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageService> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageConnection> connection;
  rv = OpenDatabaseAndHandleBusy(ss, dbFileUrl, getter_AddRefs(connection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    
    
    if (aName.IsVoid()) {
      return rv;
    }

    
    rv = aDBFile->Remove(false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aFMDirectory->Exists(&exists);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (exists) {
      bool isDirectory;
      rv = aFMDirectory->IsDirectory(&isDirectory);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
      if (NS_WARN_IF(!isDirectory)) {
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      rv = aFMDirectory->Remove(true);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = OpenDatabaseAndHandleBusy(ss, dbFileUrl, getter_AddRefs(connection));
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = SetDefaultPragmas(connection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = connection->EnableModule(NS_LITERAL_CSTRING("filesystem"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  int32_t schemaVersion;
  rv = connection->GetSchemaVersion(&schemaVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (!schemaVersion && aName.IsVoid()) {
    IDB_WARNING("Unable to open IndexedDB database, schema is not set!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  if (schemaVersion > kSQLiteSchemaVersion) {
    IDB_WARNING("Unable to open IndexedDB database, schema is too high!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  bool journalModeSet = false;

  if (schemaVersion != kSQLiteSchemaVersion) {
    const bool newDatabase = !schemaVersion;

    if (newDatabase) {
      
      if (kSQLitePageSizeOverride) {
        rv = connection->ExecuteSimpleSQL(
          nsPrintfCString("PRAGMA page_size = %lu;", kSQLitePageSizeOverride)
        );
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      
      rv = connection->ExecuteSimpleSQL(
#ifdef IDB_MOBILE
        
        
        NS_LITERAL_CSTRING("PRAGMA auto_vacuum = FULL;")
#else
        
        NS_LITERAL_CSTRING("PRAGMA auto_vacuum = INCREMENTAL;")
#endif
      );
      if (rv == NS_ERROR_FILE_NO_DEVICE_SPACE) {
        
        
        rv = NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
      }
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = SetJournalMode(connection);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      journalModeSet = true;
    } else {
#ifdef DEBUG
    
    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      connection->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("PRAGMA foreign_keys = OFF;"))));
#endif
    }

    bool vacuumNeeded = false;

    mozStorageTransaction transaction(connection, false,
                                  mozIStorageConnection::TRANSACTION_IMMEDIATE);

    if (newDatabase) {
      rv = CreateTables(connection);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(NS_SUCCEEDED(connection->GetSchemaVersion(&schemaVersion)));
      MOZ_ASSERT(schemaVersion == kSQLiteSchemaVersion);

      nsCOMPtr<mozIStorageStatement> stmt;
      nsresult rv = connection->CreateStatement(NS_LITERAL_CSTRING(
        "INSERT INTO database (name, origin) "
        "VALUES (:name, :origin)"
      ), getter_AddRefs(stmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), aName);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("origin"), aOrigin);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->Execute();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else  {
      
      static_assert(kSQLiteSchemaVersion == int32_t((18 << 4) + 0),
                    "Upgrade function needed due to schema version increase.");

      while (schemaVersion != kSQLiteSchemaVersion) {
        if (schemaVersion == 4) {
          rv = UpgradeSchemaFrom4To5(connection);
        } else if (schemaVersion == 5) {
          rv = UpgradeSchemaFrom5To6(connection);
        } else if (schemaVersion == 6) {
          rv = UpgradeSchemaFrom6To7(connection);
        } else if (schemaVersion == 7) {
          rv = UpgradeSchemaFrom7To8(connection);
        } else if (schemaVersion == 8) {
          rv = UpgradeSchemaFrom8To9_0(connection);
          vacuumNeeded = true;
        } else if (schemaVersion == MakeSchemaVersion(9, 0)) {
          rv = UpgradeSchemaFrom9_0To10_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(10, 0)) {
          rv = UpgradeSchemaFrom10_0To11_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(11, 0)) {
          rv = UpgradeSchemaFrom11_0To12_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(12, 0)) {
          rv = UpgradeSchemaFrom12_0To13_0(connection, &vacuumNeeded);
        } else if (schemaVersion == MakeSchemaVersion(13, 0)) {
          rv = UpgradeSchemaFrom13_0To14_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(14, 0)) {
          rv = UpgradeSchemaFrom14_0To15_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(15, 0)) {
          rv = UpgradeSchemaFrom15_0To16_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(16, 0)) {
          rv = UpgradeSchemaFrom16_0To17_0(connection);
        } else if (schemaVersion == MakeSchemaVersion(17, 0)) {
          rv = UpgradeSchemaFrom17_0To18_0(connection, aOrigin);
          vacuumNeeded = true;
        } else {
          IDB_WARNING("Unable to open IndexedDB database, no upgrade path is "
                      "available!");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }

        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        rv = connection->GetSchemaVersion(&schemaVersion);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      MOZ_ASSERT(schemaVersion == kSQLiteSchemaVersion);
    }

    rv = transaction.Commit();
    if (rv == NS_ERROR_FILE_NO_DEVICE_SPACE) {
      
      
      rv = NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
    }
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

#ifdef DEBUG
    if (!newDatabase) {
      
      nsCOMPtr<mozIStorageStatement> checkStmt;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        connection->CreateStatement(
          NS_LITERAL_CSTRING("PRAGMA foreign_key_check;"),
          getter_AddRefs(checkStmt))));

      bool hasResult;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(checkStmt->ExecuteStep(&hasResult)));
      MOZ_ASSERT(!hasResult, "Database has inconsisistent foreign keys!");

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        connection->ExecuteSimpleSQL(
          NS_LITERAL_CSTRING("PRAGMA foreign_keys = OFF;"))));
    }
#endif

    if (kSQLitePageSizeOverride && !newDatabase) {
      nsCOMPtr<mozIStorageStatement> stmt;
      rv = connection->CreateStatement(NS_LITERAL_CSTRING(
        "PRAGMA page_size;"
      ), getter_AddRefs(stmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      bool hasResult;
      rv = stmt->ExecuteStep(&hasResult);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(hasResult);

      int32_t pageSize;
      rv = stmt->GetInt32(0, &pageSize);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(pageSize >= 512 && pageSize <= 65536);

      if (kSQLitePageSizeOverride != uint32_t(pageSize)) {
        
        rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "PRAGMA journal_mode = DELETE;"
        ));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        rv = connection->CreateStatement(NS_LITERAL_CSTRING(
          "PRAGMA journal_mode;"
        ), getter_AddRefs(stmt));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        rv = stmt->ExecuteStep(&hasResult);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        MOZ_ASSERT(hasResult);

        nsCString journalMode;
        rv = stmt->GetUTF8String(0, journalMode);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (journalMode.EqualsLiteral("delete")) {
          
          
          rv = connection->ExecuteSimpleSQL(
            nsPrintfCString("PRAGMA page_size = %lu;", kSQLitePageSizeOverride)
          );
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          
          vacuumNeeded = true;
        } else {
          NS_WARNING("Failed to set journal_mode for database, unable to "
                     "change the page size!");
        }
      }
    }

    if (vacuumNeeded) {
      rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("VACUUM;"));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    if (newDatabase || vacuumNeeded) {
      if (journalModeSet) {
        
        rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "PRAGMA wal_checkpoint(FULL);"
        ));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      int64_t fileSize;
      rv = aDBFile->GetFileSize(&fileSize);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(fileSize > 0);

      PRTime vacuumTime = PR_Now();
      MOZ_ASSERT(vacuumTime);

      nsCOMPtr<mozIStorageStatement> vacuumTimeStmt;
      rv = connection->CreateStatement(NS_LITERAL_CSTRING(
        "UPDATE database "
          "SET last_vacuum_time = :time"
            ", last_vacuum_size = :size;"
      ), getter_AddRefs(vacuumTimeStmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = vacuumTimeStmt->BindInt64ByName(NS_LITERAL_CSTRING("time"),
                                           vacuumTime);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = vacuumTimeStmt->BindInt64ByName(NS_LITERAL_CSTRING("size"),
                                           fileSize);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = vacuumTimeStmt->Execute();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  if (!journalModeSet) {
    rv = SetJournalMode(connection);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  connection.forget(aConnection);
  return NS_OK;
}

already_AddRefed<nsIFile>
GetFileForPath(const nsAString& aPath)
{
  MOZ_ASSERT(!aPath.IsEmpty());

  nsCOMPtr<nsIFile> file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
  if (NS_WARN_IF(!file)) {
    return nullptr;
  }

  if (NS_WARN_IF(NS_FAILED(file->InitWithPath(aPath)))) {
    return nullptr;
  }

  return file.forget();
}

nsresult
GetStorageConnection(nsIFile* aDatabaseFile,
                     PersistenceType aPersistenceType,
                     const nsACString& aGroup,
                     const nsACString& aOrigin,
                     uint32_t aTelemetryId,
                     mozIStorageConnection** aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aDatabaseFile);
  MOZ_ASSERT(aConnection);

  PROFILER_LABEL("IndexedDB",
                 "GetStorageConnection",
                 js::ProfileEntry::Category::STORAGE);

  bool exists;
  nsresult rv = aDatabaseFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!exists)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  nsCOMPtr<nsIFileURL> dbFileUrl;
  rv = GetDatabaseFileURL(aDatabaseFile,
                          aPersistenceType,
                          aGroup,
                          aOrigin,
                          aTelemetryId,
                          getter_AddRefs(dbFileUrl));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageService> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageConnection> connection;
  rv = OpenDatabaseAndHandleBusy(ss, dbFileUrl, getter_AddRefs(connection));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = SetDefaultPragmas(connection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = SetJournalMode(connection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  connection.forget(aConnection);
  return NS_OK;
}

nsresult
GetStorageConnection(const nsAString& aDatabaseFilePath,
                     PersistenceType aPersistenceType,
                     const nsACString& aGroup,
                     const nsACString& aOrigin,
                     uint32_t aTelemetryId,
                     mozIStorageConnection** aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(!aDatabaseFilePath.IsEmpty());
  MOZ_ASSERT(StringEndsWith(aDatabaseFilePath, NS_LITERAL_STRING(".sqlite")));
  MOZ_ASSERT(aConnection);

  nsCOMPtr<nsIFile> dbFile = GetFileForPath(aDatabaseFilePath);
  if (NS_WARN_IF(!dbFile)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  return GetStorageConnection(dbFile,
                              aPersistenceType,
                              aGroup,
                              aOrigin,
                              aTelemetryId,
                              aConnection);
}





class DatabaseConnection final
{
  friend class ConnectionPool;

  enum CheckpointMode
  {
    CheckpointMode_Full,
    CheckpointMode_Restart,
    CheckpointMode_Truncate
  };

public:
  class AutoSavepoint;
  class CachedStatement;
  class UpdateRefcountFunction;

private:
  nsCOMPtr<mozIStorageConnection> mStorageConnection;
  nsRefPtr<FileManager> mFileManager;
  nsInterfaceHashtable<nsCStringHashKey, mozIStorageStatement>
    mCachedStatements;
  nsRefPtr<UpdateRefcountFunction> mUpdateRefcountFunction;
  bool mInReadTransaction;
  bool mInWriteTransaction;

#ifdef DEBUG
  uint32_t mDEBUGSavepointCount;
  PRThread* mDEBUGThread;
#endif

public:
  void
  AssertIsOnConnectionThread() const
  {
    MOZ_ASSERT(mDEBUGThread);
    MOZ_ASSERT(PR_GetCurrentThread() == mDEBUGThread);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DatabaseConnection)

  mozIStorageConnection*
  GetStorageConnection() const
  {
    if (mStorageConnection) {
      AssertIsOnConnectionThread();
      return mStorageConnection;
    }

    return nullptr;
  }

  UpdateRefcountFunction*
  GetUpdateRefcountFunction() const
  {
    AssertIsOnConnectionThread();

    return mUpdateRefcountFunction;
  }

  nsresult
  GetCachedStatement(const nsACString& aQuery,
                     CachedStatement* aCachedStatement);

  nsresult
  BeginWriteTransaction();

  nsresult
  CommitWriteTransaction();

  void
  RollbackWriteTransaction();

  void
  FinishWriteTransaction();

  nsresult
  StartSavepoint();

  nsresult
  ReleaseSavepoint();

  nsresult
  RollbackSavepoint();

  nsresult
  Checkpoint()
  {
    AssertIsOnConnectionThread();

    return CheckpointInternal(CheckpointMode_Full);
  }

  void
  DoIdleProcessing(bool aNeedsCheckpoint);

  void
  Close();

private:
  DatabaseConnection(mozIStorageConnection* aStorageConnection,
                     FileManager* aFileManager);

  ~DatabaseConnection();

  nsresult
  Init();

  nsresult
  CheckpointInternal(CheckpointMode aMode);

  nsresult
  GetFreelistCount(CachedStatement& aCachedStatement, uint32_t* aFreelistCount);

  nsresult
  ReclaimFreePagesWhileIdle(CachedStatement& aFreelistStatement,
                            CachedStatement& aRollbackStatement,
                            uint32_t aFreelistCount,
                            bool aNeedsCheckpoint,
                            bool* aFreedSomePages);
};

class MOZ_STACK_CLASS DatabaseConnection::AutoSavepoint final
{
  DatabaseConnection* mConnection;
#ifdef DEBUG
  const TransactionBase* mDEBUGTransaction;
#endif

public:
  AutoSavepoint();
  ~AutoSavepoint();

  nsresult
  Start(const TransactionBase* aConnection);

  nsresult
  Commit();
};

class DatabaseConnection::CachedStatement final
{
  friend class DatabaseConnection;

  nsCOMPtr<mozIStorageStatement> mStatement;
  Maybe<mozStorageStatementScoper> mScoper;

#ifdef DEBUG
  DatabaseConnection* mDEBUGConnection;
#endif

public:
  CachedStatement();
  ~CachedStatement();

  void
  AssertIsOnConnectionThread() const
  {
#ifdef DEBUG
    if (mDEBUGConnection) {
      mDEBUGConnection->AssertIsOnConnectionThread();
    }
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsOnBackgroundThread());
#endif
  }

  operator mozIStorageStatement*() const;

  mozIStorageStatement*
  operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN;

  void
  Reset();

private:
  
  void
  Assign(DatabaseConnection* aConnection,
         already_AddRefed<mozIStorageStatement> aStatement);

  
  CachedStatement(const CachedStatement&) = delete;
  CachedStatement& operator=(const CachedStatement&) = delete;
};

class DatabaseConnection::UpdateRefcountFunction final
  : public mozIStorageFunction
{
  class DatabaseUpdateFunction;
  class FileInfoEntry;

  enum UpdateType
  {
    eIncrement,
    eDecrement
  };

  DatabaseConnection* mConnection;
  FileManager* mFileManager;
  nsClassHashtable<nsUint64HashKey, FileInfoEntry> mFileInfoEntries;
  nsDataHashtable<nsUint64HashKey, FileInfoEntry*> mSavepointEntriesIndex;

  nsTArray<int64_t> mJournalsToCreateBeforeCommit;
  nsTArray<int64_t> mJournalsToRemoveAfterCommit;
  nsTArray<int64_t> mJournalsToRemoveAfterAbort;

  bool mInSavepoint;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  UpdateRefcountFunction(DatabaseConnection* aConnection,
                         FileManager* aFileManager);

  nsresult
  WillCommit();

  void
  DidCommit();

  void
  DidAbort();

  void
  StartSavepoint();

  void
  ReleaseSavepoint();

  void
  RollbackSavepoint();

  void
  Reset();

private:
  ~UpdateRefcountFunction()
  { }

  nsresult
  ProcessValue(mozIStorageValueArray* aValues,
               int32_t aIndex,
               UpdateType aUpdateType);

  nsresult
  CreateJournals();

  nsresult
  RemoveJournals(const nsTArray<int64_t>& aJournals);
};

class DatabaseConnection::UpdateRefcountFunction::DatabaseUpdateFunction final
{
  CachedStatement mUpdateStatement;
  CachedStatement mSelectStatement;
  CachedStatement mInsertStatement;

  UpdateRefcountFunction* mFunction;

  nsresult mErrorCode;

public:
  explicit
  DatabaseUpdateFunction(UpdateRefcountFunction* aFunction)
    : mFunction(aFunction)
    , mErrorCode(NS_OK)
  {
    MOZ_COUNT_CTOR(
      DatabaseConnection::UpdateRefcountFunction::DatabaseUpdateFunction);
  }

  ~DatabaseUpdateFunction()
  {
    MOZ_COUNT_DTOR(
      DatabaseConnection::UpdateRefcountFunction::DatabaseUpdateFunction);
  }

  bool
  Update(int64_t aId, int32_t aDelta);

  nsresult
  ErrorCode() const
  {
    return mErrorCode;
  }

private:
  nsresult
  UpdateInternal(int64_t aId, int32_t aDelta);
};

class DatabaseConnection::UpdateRefcountFunction::FileInfoEntry final
{
  friend class UpdateRefcountFunction;

  nsRefPtr<FileInfo> mFileInfo;
  int32_t mDelta;
  int32_t mSavepointDelta;

public:
  explicit
  FileInfoEntry(FileInfo* aFileInfo)
    : mFileInfo(aFileInfo)
    , mDelta(0)
    , mSavepointDelta(0)
  {
    MOZ_COUNT_CTOR(DatabaseConnection::UpdateRefcountFunction::FileInfoEntry);
  }

  ~FileInfoEntry()
  {
    MOZ_COUNT_DTOR(DatabaseConnection::UpdateRefcountFunction::FileInfoEntry);
  }
};

class ConnectionPool final
{
public:
  class FinishCallback;

private:
  class ConnectionRunnable;
  class CloseConnectionRunnable;
  struct DatabaseInfo;
  struct DatabasesCompleteCallback;
  class FinishCallbackWrapper;
  class IdleConnectionRunnable;
  struct IdleDatabaseInfo;
  struct IdleResource;
  struct IdleThreadInfo;
  struct ThreadInfo;
  class ThreadRunnable;
  struct TransactionInfo;
  struct TransactionInfoPair;

  
  Mutex mDatabasesMutex;

  nsTArray<IdleThreadInfo> mIdleThreads;
  nsTArray<IdleDatabaseInfo> mIdleDatabases;
  nsTArray<DatabaseInfo*> mDatabasesPerformingIdleMaintenance;
  nsCOMPtr<nsITimer> mIdleTimer;
  TimeStamp mTargetIdleTime;

  
  
  
  nsClassHashtable<nsCStringHashKey, DatabaseInfo> mDatabases;

  nsClassHashtable<nsUint64HashKey, TransactionInfo> mTransactions;
  nsTArray<TransactionInfo*> mQueuedTransactions;

  nsTArray<nsAutoPtr<DatabasesCompleteCallback>> mCompleteCallbacks;

  uint64_t mNextTransactionId;
  uint32_t mTotalThreadCount;
  bool mShutdownRequested;
  bool mShutdownComplete;

#ifdef DEBUG
  PRThread* mDEBUGOwningThread;
#endif

public:
  ConnectionPool();

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  nsresult
  GetOrCreateConnection(const Database* aDatabase,
                        DatabaseConnection** aConnection);

  uint64_t
  Start(const nsID& aBackgroundChildLoggingId,
        const nsACString& aDatabaseId,
        int64_t aLoggingSerialNumber,
        const nsTArray<nsString>& aObjectStoreNames,
        bool aIsWriteTransaction,
        TransactionDatabaseOperationBase* aTransactionOp);

  void
  Dispatch(uint64_t aTransactionId, nsIRunnable* aRunnable);

  void
  Finish(uint64_t aTransactionId, FinishCallback* aCallback);

  void
  CloseDatabaseWhenIdle(const nsACString& aDatabaseId)
  {
    unused << CloseDatabaseWhenIdleInternal(aDatabaseId);
  }

  void
  WaitForDatabasesToComplete(nsTArray<nsCString>&& aDatabaseIds,
                             nsIRunnable* aCallback);

  void
  Shutdown();

  NS_INLINE_DECL_REFCOUNTING(ConnectionPool)

private:
  ~ConnectionPool();

  static void
  IdleTimerCallback(nsITimer* aTimer, void* aClosure);

  void
  Cleanup();

  void
  AdjustIdleTimer();

  void
  CancelIdleTimer();

  void
  ShutdownThread(ThreadInfo& aThreadInfo);

  void
  CloseIdleDatabases();

  void
  ShutdownIdleThreads();

  bool
  ScheduleTransaction(TransactionInfo* aTransactionInfo,
                      bool aFromQueuedTransactions);

  void
  NoteFinishedTransaction(uint64_t aTransactionId);

  void
  ScheduleQueuedTransactions(ThreadInfo& aThreadInfo);

  void
  NoteIdleDatabase(DatabaseInfo* aDatabaseInfo);

  void
  NoteClosedDatabase(DatabaseInfo* aDatabaseInfo);

  bool
  MaybeFireCallback(DatabasesCompleteCallback* aCallback);

  void
  PerformIdleDatabaseMaintenance(DatabaseInfo* aDatabaseInfo);

  void
  CloseDatabase(DatabaseInfo* aDatabaseInfo);

  bool
  CloseDatabaseWhenIdleInternal(const nsACString& aDatabaseId);
};

class ConnectionPool::ConnectionRunnable
  : public nsRunnable
{
protected:
  DatabaseInfo* mDatabaseInfo;
  nsCOMPtr<nsIEventTarget> mOwningThread;

  explicit
  ConnectionRunnable(DatabaseInfo* aDatabaseInfo);

  virtual
  ~ConnectionRunnable()
  { }
};

class ConnectionPool::IdleConnectionRunnable final
  : public ConnectionRunnable
{
  bool mNeedsCheckpoint;

public:
  IdleConnectionRunnable(DatabaseInfo* aDatabaseInfo, bool aNeedsCheckpoint)
    : ConnectionRunnable(aDatabaseInfo)
    , mNeedsCheckpoint(aNeedsCheckpoint)
  { }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~IdleConnectionRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

class ConnectionPool::CloseConnectionRunnable final
  : public ConnectionRunnable
{
public:
  explicit
  CloseConnectionRunnable(DatabaseInfo* aDatabaseInfo)
    : ConnectionRunnable(aDatabaseInfo)
  { }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~CloseConnectionRunnable()
  { }

  NS_DECL_NSIRUNNABLE
};

struct ConnectionPool::ThreadInfo
{
  nsCOMPtr<nsIThread> mThread;
  nsRefPtr<ThreadRunnable> mRunnable;

  ThreadInfo();

  explicit
  ThreadInfo(const ThreadInfo& aOther);

  ~ThreadInfo();
};

struct ConnectionPool::DatabaseInfo final
{
  friend class nsAutoPtr<DatabaseInfo>;

  nsRefPtr<ConnectionPool> mConnectionPool;
  const nsCString mDatabaseId;
  nsRefPtr<DatabaseConnection> mConnection;
  nsClassHashtable<nsStringHashKey, TransactionInfoPair> mBlockingTransactions;
  nsTArray<TransactionInfo*> mTransactionsScheduledDuringClose;
  nsTArray<TransactionInfo*> mScheduledWriteTransactions;
  TransactionInfo* mRunningWriteTransaction;
  ThreadInfo mThreadInfo;
  uint32_t mReadTransactionCount;
  uint32_t mWriteTransactionCount;
  bool mNeedsCheckpoint;
  bool mIdle;
  bool mCloseOnIdle;
  bool mClosing;

#ifdef DEBUG
  PRThread* mDEBUGConnectionThread;
#endif

  DatabaseInfo(ConnectionPool* aConnectionPool,
               const nsACString& aDatabaseId);

  void
  AssertIsOnConnectionThread() const
  {
    MOZ_ASSERT(mDEBUGConnectionThread);
    MOZ_ASSERT(PR_GetCurrentThread() == mDEBUGConnectionThread);
  }

  uint64_t
  TotalTransactionCount() const
  {
    return mReadTransactionCount + mWriteTransactionCount;
  }

private:
  ~DatabaseInfo();

  DatabaseInfo(const DatabaseInfo&) = delete;
  DatabaseInfo& operator=(const DatabaseInfo&) = delete;
};

struct ConnectionPool::DatabasesCompleteCallback final
{
  friend class nsAutoPtr<DatabasesCompleteCallback>;

  nsTArray<nsCString> mDatabaseIds;
  nsCOMPtr<nsIRunnable> mCallback;

  DatabasesCompleteCallback(nsTArray<nsCString>&& aDatabaseIds,
                            nsIRunnable* aCallback);

private:
  ~DatabasesCompleteCallback();
};

class NS_NO_VTABLE ConnectionPool::FinishCallback
  : public nsIRunnable
{
public:
  
  
  virtual void
  TransactionFinishedBeforeUnblock() = 0;

  
  
  virtual void
  TransactionFinishedAfterUnblock() = 0;

protected:
  FinishCallback()
  { }

  virtual ~FinishCallback()
  { }
};

class ConnectionPool::FinishCallbackWrapper final
  : public nsRunnable
{
  nsRefPtr<ConnectionPool> mConnectionPool;
  nsRefPtr<FinishCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mOwningThread;
  uint64_t mTransactionId;
  bool mHasRunOnce;

public:
  FinishCallbackWrapper(ConnectionPool* aConnectionPool,
                        uint64_t aTransactionId,
                        FinishCallback* aCallback);

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~FinishCallbackWrapper();

  NS_DECL_NSIRUNNABLE
};

struct ConnectionPool::IdleResource
{
  TimeStamp mIdleTime;

protected:
  explicit
  IdleResource(const TimeStamp& aIdleTime);

  explicit
  IdleResource(const IdleResource& aOther);

  ~IdleResource();
};

struct ConnectionPool::IdleDatabaseInfo final
  : public IdleResource
{
  DatabaseInfo* mDatabaseInfo;

public:
  MOZ_IMPLICIT
  IdleDatabaseInfo(DatabaseInfo* aDatabaseInfo);

  explicit
  IdleDatabaseInfo(const IdleDatabaseInfo& aOther);

  ~IdleDatabaseInfo();

  bool
  operator==(const IdleDatabaseInfo& aOther) const
  {
    return mDatabaseInfo == aOther.mDatabaseInfo;
  }

  bool
  operator<(const IdleDatabaseInfo& aOther) const
  {
    return mIdleTime < aOther.mIdleTime;
  }
};

struct ConnectionPool::IdleThreadInfo final
  : public IdleResource
{
  ThreadInfo mThreadInfo;

public:
  
  
  MOZ_IMPLICIT
  IdleThreadInfo(const ThreadInfo& aThreadInfo);

  explicit
  IdleThreadInfo(const IdleThreadInfo& aOther);

  ~IdleThreadInfo();

  bool
  operator==(const IdleThreadInfo& aOther) const
  {
    return mThreadInfo.mRunnable == aOther.mThreadInfo.mRunnable &&
           mThreadInfo.mThread == aOther.mThreadInfo.mThread;
  }

  bool
  operator<(const IdleThreadInfo& aOther) const
  {
    return mIdleTime < aOther.mIdleTime;
  }
};

class ConnectionPool::ThreadRunnable final
  : public nsRunnable
{
  
  static uint32_t sNextSerialNumber;

  
  const uint32_t mSerialNumber;

  
  bool mFirstRun;
  bool mContinueRunning;

public:
  ThreadRunnable();

  NS_DECL_ISUPPORTS_INHERITED

  uint32_t
  SerialNumber() const
  {
    return mSerialNumber;
  }

private:
  ~ThreadRunnable();

  NS_DECL_NSIRUNNABLE
};

struct ConnectionPool::TransactionInfo final
{
  friend class nsAutoPtr<TransactionInfo>;

  DatabaseInfo* mDatabaseInfo;
  const nsID mBackgroundChildLoggingId;
  const nsCString mDatabaseId;
  const uint64_t mTransactionId;
  const int64_t mLoggingSerialNumber;
  const nsTArray<nsString> mObjectStoreNames;
  nsTHashtable<nsPtrHashKey<TransactionInfo>> mBlockedOn;
  nsTHashtable<nsPtrHashKey<TransactionInfo>> mBlocking;
  nsTArray<nsCOMPtr<nsIRunnable>> mQueuedRunnables;
  const bool mIsWriteTransaction;
  bool mRunning;

  DebugOnly<bool> mFinished;

  TransactionInfo(DatabaseInfo* aDatabaseInfo,
                  const nsID& aBackgroundChildLoggingId,
                  const nsACString& aDatabaseId,
                  uint64_t aTransactionId,
                  int64_t aLoggingSerialNumber,
                  const nsTArray<nsString>& aObjectStoreNames,
                  bool aIsWriteTransaction,
                  TransactionDatabaseOperationBase* aTransactionOp);

  void
  Schedule();

private:
  ~TransactionInfo();
};

struct ConnectionPool::TransactionInfoPair final
{
  friend class nsAutoPtr<TransactionInfoPair>;

  
  nsTArray<TransactionInfo*> mLastBlockingWrites;
  
  TransactionInfo* mLastBlockingReads;

  TransactionInfoPair();

private:
  ~TransactionInfoPair();
};





class DatabaseOperationBase
  : public nsRunnable
  , public mozIStorageProgressHandler
{
protected:
  class AutoSetProgressHandler;

  typedef nsDataHashtable<nsUint64HashKey, bool> UniqueIndexTable;

  nsCOMPtr<nsIEventTarget> mOwningThread;
  const nsID mBackgroundChildLoggingId;
  const uint64_t mLoggingSerialNumber;
  nsresult mResultCode;

private:
  Atomic<bool> mOperationMayProceed;
  bool mActorDestroyed;

public:
  NS_DECL_ISUPPORTS_INHERITED

  void
  AssertIsOnOwningThread() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mOwningThread);
    DebugOnly<bool> current;
    MOZ_ASSERT(NS_SUCCEEDED(mOwningThread->IsOnCurrentThread(&current)));
    MOZ_ASSERT(current);
  }

  void
  NoteActorDestroyed()
  {
    AssertIsOnOwningThread();

    mActorDestroyed = true;
    mOperationMayProceed = false;
  }

  bool
  IsActorDestroyed() const
  {
    AssertIsOnOwningThread();

    return mActorDestroyed;
  }

  
  
  bool
  OperationMayProceed() const
  {
    return mOperationMayProceed;
  }

  const nsID&
  BackgroundChildLoggingId() const
  {
    return mBackgroundChildLoggingId;
  }

  uint64_t
  LoggingSerialNumber() const
  {
    return mLoggingSerialNumber;
  }

  nsresult
  ResultCode() const
  {
    return mResultCode;
  }

  void
  SetFailureCode(nsresult aErrorCode)
  {
    MOZ_ASSERT(NS_SUCCEEDED(mResultCode));
    MOZ_ASSERT(NS_FAILED(aErrorCode));

    mResultCode = aErrorCode;
  }

protected:
  DatabaseOperationBase(const nsID& aBackgroundChildLoggingId,
                        uint64_t aLoggingSerialNumber)
    : mOwningThread(NS_GetCurrentThread())
    , mBackgroundChildLoggingId(aBackgroundChildLoggingId)
    , mLoggingSerialNumber(aLoggingSerialNumber)
    , mResultCode(NS_OK)
    , mOperationMayProceed(true)
    , mActorDestroyed(false)
  {
    AssertIsOnOwningThread();
  }

  virtual
  ~DatabaseOperationBase()
  {
    MOZ_ASSERT(mActorDestroyed);
  }

  static void
  GetBindingClauseForKeyRange(const SerializedKeyRange& aKeyRange,
                              const nsACString& aKeyColumnName,
                              nsAutoCString& aBindingClause);

  static uint64_t
  ReinterpretDoubleAsUInt64(double aDouble);

  static nsresult
  GetStructuredCloneReadInfoFromStatement(mozIStorageStatement* aStatement,
                                          uint32_t aDataIndex,
                                          uint32_t aFileIdsIndex,
                                          FileManager* aFileManager,
                                          StructuredCloneReadInfo* aInfo)
  {
    return GetStructuredCloneReadInfoFromSource(aStatement,
                                                aDataIndex,
                                                aFileIdsIndex,
                                                aFileManager,
                                                aInfo);
  }

  static nsresult
  GetStructuredCloneReadInfoFromValueArray(mozIStorageValueArray* aValues,
                                           uint32_t aDataIndex,
                                           uint32_t aFileIdsIndex,
                                           FileManager* aFileManager,
                                           StructuredCloneReadInfo* aInfo)
  {
    return GetStructuredCloneReadInfoFromSource(aValues,
                                                aDataIndex,
                                                aFileIdsIndex,
                                                aFileManager,
                                                aInfo);
  }

  static nsresult
  BindKeyRangeToStatement(const SerializedKeyRange& aKeyRange,
                          mozIStorageStatement* aStatement);

  static void
  AppendConditionClause(const nsACString& aColumnName,
                        const nsACString& aArgName,
                        bool aLessThan,
                        bool aEquals,
                        nsAutoCString& aResult);

  static nsresult
  GetUniqueIndexTableForObjectStore(
                               TransactionBase* aTransaction,
                               int64_t aObjectStoreId,
                               Maybe<UniqueIndexTable>& aMaybeUniqueIndexTable);

  static nsresult
  IndexDataValuesFromUpdateInfos(
                              const nsTArray<IndexUpdateInfo>& aUpdateInfos,
                              const UniqueIndexTable& aUniqueIndexTable,
                              FallibleTArray<IndexDataValue>& aIndexValues);

  static nsresult
  InsertIndexTableRows(DatabaseConnection* aConnection,
                       const int64_t aObjectStoreId,
                       const Key& aObjectStoreKey,
                       const FallibleTArray<IndexDataValue>& aIndexValues);

  static nsresult
  DeleteIndexDataTableRows(DatabaseConnection* aConnection,
                           const Key& aObjectStoreKey,
                           const FallibleTArray<IndexDataValue>& aIndexValues);

  static nsresult
  DeleteObjectStoreDataTableRowsWithIndexes(DatabaseConnection* aConnection,
                                            const int64_t aObjectStoreId,
                                            const OptionalKeyRange& aKeyRange);

  static nsresult
  UpdateIndexValues(DatabaseConnection* aConnection,
                    const int64_t aObjectStoreId,
                    const Key& aObjectStoreKey,
                    const FallibleTArray<IndexDataValue>& aIndexValues);

  static nsresult
  ObjectStoreHasIndexes(DatabaseConnection* aConnection,
                        const int64_t aObjectStoreId,
                        bool* aHasIndexes);

private:
  template <typename T>
  static nsresult
  GetStructuredCloneReadInfoFromSource(T* aSource,
                                       uint32_t aDataIndex,
                                       uint32_t aFileIdsIndex,
                                       FileManager* aFileManager,
                                       StructuredCloneReadInfo* aInfo);

  static nsresult
  GetStructuredCloneReadInfoFromBlob(const uint8_t* aBlobData,
                                     uint32_t aBlobDataLength,
                                     const nsAString& aFileIds,
                                     FileManager* aFileManager,
                                     StructuredCloneReadInfo* aInfo);

  
  NS_DECL_MOZISTORAGEPROGRESSHANDLER
};

class MOZ_STACK_CLASS DatabaseOperationBase::AutoSetProgressHandler final
{
  mozIStorageConnection* mConnection;
  DebugOnly<DatabaseOperationBase*> mDEBUGDatabaseOp;

public:
  AutoSetProgressHandler();

  ~AutoSetProgressHandler();

  nsresult
  Register(mozIStorageConnection* aConnection,
           DatabaseOperationBase* aDatabaseOp);
};

class TransactionDatabaseOperationBase
  : public DatabaseOperationBase
{
  nsRefPtr<TransactionBase> mTransaction;
  const int64_t mTransactionLoggingSerialNumber;
  const bool mTransactionIsAborted;

public:
  void
  AssertIsOnConnectionThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  void
  DispatchToConnectionPool();

  TransactionBase*
  Transaction() const
  {
    MOZ_ASSERT(mTransaction);

    return mTransaction;
  }

  
  
  
  virtual bool
  Init(TransactionBase* aTransaction);

  
  
  
  virtual void
  Cleanup();

protected:
  explicit
  TransactionDatabaseOperationBase(TransactionBase* aTransaction);

  TransactionDatabaseOperationBase(TransactionBase* aTransaction,
                                   uint64_t aLoggingSerialNumber);

  virtual
  ~TransactionDatabaseOperationBase();

  virtual void
  RunOnConnectionThread();

  
  
  
  
  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) = 0;

  
  
  
  virtual nsresult
  SendSuccessResult() = 0;

  
  
  
  
  virtual bool
  SendFailureResult(nsresult aResultCode) = 0;

private:
  void
  RunOnOwningThread();

  
  NS_DECL_NSIRUNNABLE
};

class Factory final
  : public PBackgroundIDBFactoryParent
{
  
  
  static uint64_t sFactoryInstanceCount;

  nsRefPtr<DatabaseLoggingInfo> mLoggingInfo;

  DebugOnly<bool> mActorDestroyed;

public:
  static already_AddRefed<Factory>
  Create(const LoggingInfo& aLoggingInfo);

  DatabaseLoggingInfo*
  GetLoggingInfo() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mLoggingInfo);

    return mLoggingInfo;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::indexedDB::Factory)

private:
  
  explicit
  Factory(already_AddRefed<DatabaseLoggingInfo> aLoggingInfo);

  
  ~Factory();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvDeleteMe() override;

  virtual bool
  RecvIncrementLoggingRequestSerialNumber() override;

  virtual PBackgroundIDBFactoryRequestParent*
  AllocPBackgroundIDBFactoryRequestParent(const FactoryRequestParams& aParams)
                                          override;

  virtual bool
  RecvPBackgroundIDBFactoryRequestConstructor(
                                     PBackgroundIDBFactoryRequestParent* aActor,
                                     const FactoryRequestParams& aParams)
                                     override;

  virtual bool
  DeallocPBackgroundIDBFactoryRequestParent(
                                     PBackgroundIDBFactoryRequestParent* aActor)
                                     override;

  virtual PBackgroundIDBDatabaseParent*
  AllocPBackgroundIDBDatabaseParent(
                                   const DatabaseSpec& aSpec,
                                   PBackgroundIDBFactoryRequestParent* aRequest)
                                   override;

  virtual bool
  DeallocPBackgroundIDBDatabaseParent(PBackgroundIDBDatabaseParent* aActor)
                                      override;
};

class WaitForTransactionsHelper final
  : public nsRunnable
{
  nsCOMPtr<nsIEventTarget> mOwningThread;
  const nsCString mDatabaseId;
  nsCOMPtr<nsIRunnable> mCallback;

  enum
  {
    State_Initial = 0,
    State_WaitingForTransactions,
    State_DispatchToMainThread,
    State_WaitingForFileHandles,
    State_DispatchToOwningThread,
    State_Complete
  } mState;

public:
  WaitForTransactionsHelper(const nsCString& aDatabaseId,
                            nsIRunnable* aCallback)
    : mOwningThread(NS_GetCurrentThread())
    , mDatabaseId(aDatabaseId)
    , mCallback(aCallback)
    , mState(State_Initial)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(!aDatabaseId.IsEmpty());
    MOZ_ASSERT(aCallback);
  }

  void
  WaitForTransactions();

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~WaitForTransactionsHelper()
  {
    MOZ_ASSERT(!mCallback);
    MOZ_ASSERT(mState == State_Complete);
  }

  void
  MaybeWaitForTransactions();

  void
  DispatchToMainThread();

  void
  MaybeWaitForFileHandles();

  void
  DispatchToOwningThread();

  void
  CallCallback();

  NS_DECL_NSIRUNNABLE
};

class UnlockDirectoryRunnable final
  : public nsRunnable
{
  nsRefPtr<DirectoryLock> mDirectoryLock;

public:
  explicit
  UnlockDirectoryRunnable(already_AddRefed<DirectoryLock> aDirectoryLock)
    : mDirectoryLock(Move(aDirectoryLock))
  { }

private:
  ~UnlockDirectoryRunnable()
  {
    MOZ_ASSERT(!mDirectoryLock);
  }

  NS_IMETHOD
  Run() override;
};

class Database final
  : public PBackgroundIDBDatabaseParent
{
  friend class VersionChangeTransaction;

  class StartTransactionOp;

private:
  nsRefPtr<Factory> mFactory;
  nsRefPtr<FullDatabaseMetadata> mMetadata;
  nsRefPtr<FileManager> mFileManager;
  nsRefPtr<DirectoryLock> mDirectoryLock;
  nsTHashtable<nsPtrHashKey<TransactionBase>> mTransactions;
  nsRefPtr<DatabaseConnection> mConnection;
  const PrincipalInfo mPrincipalInfo;
  const OptionalContentId mOptionalContentParentId;
  const nsCString mGroup;
  const nsCString mOrigin;
  const nsCString mId;
  const nsString mFilePath;
  uint32_t mFileHandleCount;
  const uint32_t mTelemetryId;
  const PersistenceType mPersistenceType;
  const bool mChromeWriteAccessAllowed;
  bool mClosed;
  bool mInvalidated;
  bool mActorWasAlive;
  bool mActorDestroyed;
  bool mMetadataCleanedUp;

public:
  
  Database(Factory* aFactory,
           const PrincipalInfo& aPrincipalInfo,
           const OptionalContentId& aOptionalContentParentId,
           const nsACString& aGroup,
           const nsACString& aOrigin,
           uint32_t aTelemetryId,
           FullDatabaseMetadata* aMetadata,
           FileManager* aFileManager,
           already_AddRefed<DirectoryLock> aDirectoryLock,
           bool aChromeWriteAccessAllowed);

  void
  AssertIsOnConnectionThread() const
  {
#ifdef DEBUG
    if (mConnection) {
      MOZ_ASSERT(mConnection);
      mConnection->AssertIsOnConnectionThread();
    } else {
      MOZ_ASSERT(!NS_IsMainThread());
      MOZ_ASSERT(!IsOnBackgroundThread());
      MOZ_ASSERT(mInvalidated);
    }
#endif
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::indexedDB::Database)

  void
  Invalidate();

  const PrincipalInfo&
  GetPrincipalInfo() const
  {
    return mPrincipalInfo;
  }

  bool
  IsOwnedByProcess(ContentParentId aContentParentId) const
  {
    MOZ_ASSERT(mOptionalContentParentId.type() != OptionalContentId::T__None);

    return
      mOptionalContentParentId.type() == OptionalContentId::TContentParentId &&
      mOptionalContentParentId.get_ContentParentId() == aContentParentId;
  }

  const nsCString&
  Group() const
  {
    return mGroup;
  }

  const nsCString&
  Origin() const
  {
    return mOrigin;
  }

  const nsCString&
  Id() const
  {
    return mId;
  }

  uint32_t
  TelemetryId() const
  {
    return mTelemetryId;
  }

  PersistenceType
  Type() const
  {
    return mPersistenceType;
  }

  const nsString&
  FilePath() const
  {
    return mFilePath;
  }

  FileManager*
  GetFileManager() const
  {
    return mFileManager;
  }

  FullDatabaseMetadata*
  Metadata() const
  {
    MOZ_ASSERT(mMetadata);
    return mMetadata;
  }

  PBackgroundParent*
  GetBackgroundParent() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(!IsActorDestroyed());

    return Manager()->Manager();
  }

  DatabaseLoggingInfo*
  GetLoggingInfo() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mFactory);

    return mFactory->GetLoggingInfo();
  }

  void
  ReleaseTransactionThreadObjects();

  void
  ReleaseBackgroundThreadObjects();

  bool
  RegisterTransaction(TransactionBase* aTransaction);

  void
  UnregisterTransaction(TransactionBase* aTransaction);

  void
  SetActorAlive();

  bool
  IsActorAlive() const
  {
    AssertIsOnBackgroundThread();

    return mActorWasAlive && !mActorDestroyed;
  }

  bool
  IsActorDestroyed() const
  {
    AssertIsOnBackgroundThread();

    return mActorWasAlive && mActorDestroyed;
  }

  bool
  IsClosed() const
  {
    AssertIsOnBackgroundThread();

    return mClosed;
  }

  bool
  IsInvalidated() const
  {
    AssertIsOnBackgroundThread();

    return mInvalidated;
  }

  nsresult
  EnsureConnection();

  DatabaseConnection*
  GetConnection() const
  {
#ifdef DEBUG
    if (mConnection) {
      mConnection->AssertIsOnConnectionThread();
    }
#endif

    return mConnection;
  }

private:
  
  ~Database()
  {
    MOZ_ASSERT(mClosed);
    MOZ_ASSERT_IF(mActorWasAlive, mActorDestroyed);
  }

  bool
  CloseInternal();

  void
  MaybeCloseConnection();

  void
  ConnectionClosedCallback();

  void
  CleanupMetadata();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBackgroundIDBDatabaseFileParent*
  AllocPBackgroundIDBDatabaseFileParent(PBlobParent* aBlobParent)
                                        override;

  virtual bool
  DeallocPBackgroundIDBDatabaseFileParent(
                                       PBackgroundIDBDatabaseFileParent* aActor)
                                       override;

  virtual PBackgroundIDBTransactionParent*
  AllocPBackgroundIDBTransactionParent(
                                    const nsTArray<nsString>& aObjectStoreNames,
                                    const Mode& aMode)
                                    override;

  virtual bool
  RecvPBackgroundIDBTransactionConstructor(
                                    PBackgroundIDBTransactionParent* aActor,
                                    InfallibleTArray<nsString>&& aObjectStoreNames,
                                    const Mode& aMode)
                                    override;

  virtual bool
  DeallocPBackgroundIDBTransactionParent(
                                        PBackgroundIDBTransactionParent* aActor)
                                        override;

  virtual PBackgroundIDBVersionChangeTransactionParent*
  AllocPBackgroundIDBVersionChangeTransactionParent(
                                              const uint64_t& aCurrentVersion,
                                              const uint64_t& aRequestedVersion,
                                              const int64_t& aNextObjectStoreId,
                                              const int64_t& aNextIndexId)
                                              override;

  virtual bool
  DeallocPBackgroundIDBVersionChangeTransactionParent(
                           PBackgroundIDBVersionChangeTransactionParent* aActor)
                           override;

  virtual bool
  RecvDeleteMe() override;

  virtual bool
  RecvBlocked() override;

  virtual bool
  RecvClose() override;

  virtual bool
  RecvNewFileHandle() override;

  virtual bool
  RecvFileHandleFinished() override;
};

class Database::StartTransactionOp final
  : public TransactionDatabaseOperationBase
{
  friend class Database;

private:
  explicit
  StartTransactionOp(TransactionBase* aTransaction)
    : TransactionDatabaseOperationBase(aTransaction,
                                        0)
  { }

  ~StartTransactionOp()
  { }

  virtual void
  RunOnConnectionThread() override;

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual nsresult
  SendSuccessResult() override;

  virtual bool
  SendFailureResult(nsresult aResultCode) override;

  virtual void
  Cleanup() override;
};

class DatabaseFile final
  : public PBackgroundIDBDatabaseFileParent
{
  friend class Database;

  nsRefPtr<BlobImpl> mBlobImpl;
  nsRefPtr<FileInfo> mFileInfo;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::indexedDB::DatabaseFile);

  FileInfo*
  GetFileInfo() const
  {
    AssertIsOnBackgroundThread();

    return mFileInfo;
  }

  already_AddRefed<nsIInputStream>
  GetInputStream() const
  {
    AssertIsOnBackgroundThread();

    nsCOMPtr<nsIInputStream> inputStream;
    if (mBlobImpl) {
      ErrorResult rv;
      mBlobImpl->GetInternalStream(getter_AddRefs(inputStream), rv);
      MOZ_ALWAYS_TRUE(!rv.Failed());
    }

    return inputStream.forget();
  }

  void
  ClearInputStream()
  {
    AssertIsOnBackgroundThread();

    mBlobImpl = nullptr;
  }

private:
  
  explicit DatabaseFile(FileInfo* aFileInfo)
    : mFileInfo(aFileInfo)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aFileInfo);
  }

  
  DatabaseFile(BlobImpl* aBlobImpl, FileInfo* aFileInfo)
    : mBlobImpl(aBlobImpl)
    , mFileInfo(aFileInfo)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aBlobImpl);
    MOZ_ASSERT(aFileInfo);
  }

  ~DatabaseFile()
  { }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override
  {
    AssertIsOnBackgroundThread();

    mBlobImpl = nullptr;
    mFileInfo = nullptr;
  }
};

class TransactionBase
{
  friend class Cursor;

  class CommitOp;

protected:
  typedef IDBTransaction::Mode Mode;

private:
  nsRefPtr<Database> mDatabase;
  nsTArray<nsRefPtr<FullObjectStoreMetadata>>
    mModifiedAutoIncrementObjectStoreMetadataArray;
  uint64_t mTransactionId;
  const nsCString mDatabaseId;
  const int64_t mLoggingSerialNumber;
  uint64_t mActiveRequestCount;
  Atomic<bool> mInvalidatedOnAnyThread;
  const Mode mMode;
  bool mHasBeenActive;
  bool mHasBeenActiveOnConnectionThread;
  bool mActorDestroyed;
  bool mInvalidated;

protected:
  nsresult mResultCode;
  bool mCommitOrAbortReceived;
  bool mCommittedOrAborted;
  bool mForceAborted;

public:
  void
  AssertIsOnConnectionThread() const
  {
    MOZ_ASSERT(mDatabase);
    mDatabase->AssertIsOnConnectionThread();
  }

  bool
  IsActorDestroyed() const
  {
    AssertIsOnBackgroundThread();

    return mActorDestroyed;
  }

  
  bool
  IsInvalidated() const
  {
    MOZ_ASSERT(IsOnBackgroundThread(), "Use IsInvalidatedOnAnyThread()");
    MOZ_ASSERT_IF(mInvalidated, NS_FAILED(mResultCode));

    return mInvalidated;
  }

  
  bool
  IsInvalidatedOnAnyThread() const
  {
    return mInvalidatedOnAnyThread;
  }

  void
  SetActive(uint64_t aTransactionId)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aTransactionId);

    mTransactionId = aTransactionId;
    mHasBeenActive = true;
  }

  void
  SetActiveOnConnectionThread()
  {
    AssertIsOnConnectionThread();
    mHasBeenActiveOnConnectionThread = true;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(
    mozilla::dom::indexedDB::TransactionBase)

  void
  Abort(nsresult aResultCode, bool aForce);

  uint64_t
  TransactionId() const
  {
    return mTransactionId;
  }

  const nsCString&
  DatabaseId() const
  {
    return mDatabaseId;
  }

  Mode
  GetMode() const
  {
    return mMode;
  }

  Database*
  GetDatabase() const
  {
    MOZ_ASSERT(mDatabase);

    return mDatabase;
  }

  DatabaseLoggingInfo*
  GetLoggingInfo() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mDatabase);

    return mDatabase->GetLoggingInfo();
  }

  int64_t
  LoggingSerialNumber() const
  {
    return mLoggingSerialNumber;
  }

  bool
  IsAborted() const
  {
    AssertIsOnBackgroundThread();

    return NS_FAILED(mResultCode);
  }

  already_AddRefed<FullObjectStoreMetadata>
  GetMetadataForObjectStoreId(int64_t aObjectStoreId) const;

  already_AddRefed<FullIndexMetadata>
  GetMetadataForIndexId(FullObjectStoreMetadata* const aObjectStoreMetadata,
                        int64_t aIndexId) const;

  PBackgroundParent*
  GetBackgroundParent() const
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(!IsActorDestroyed());

    return GetDatabase()->GetBackgroundParent();
  }

  void
  NoteModifiedAutoIncrementObjectStore(FullObjectStoreMetadata* aMetadata);

  void
  ForgetModifiedAutoIncrementObjectStore(FullObjectStoreMetadata* aMetadata);

  void
  NoteActiveRequest();

  void
  NoteFinishedRequest();

  void
  Invalidate();

protected:
  TransactionBase(Database* aDatabase, Mode aMode);

  virtual
  ~TransactionBase();

  void
  NoteActorDestroyed()
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(!mActorDestroyed);

    mActorDestroyed = true;
  }

#ifdef DEBUG
  
  void
  FakeActorDestroyed()
  {
    mActorDestroyed = true;
  }
#endif

  bool
  RecvCommit();

  bool
  RecvAbort(nsresult aResultCode);

  void
  MaybeCommitOrAbort()
  {
    AssertIsOnBackgroundThread();

    
    if (mCommittedOrAborted) {
      return;
    }

    
    
    if (mActiveRequestCount) {
      return;
    }

    
    
    
    if (!mCommitOrAbortReceived && !mForceAborted) {
      return;
    }

    CommitOrAbort();
  }

  PBackgroundIDBRequestParent*
  AllocRequest(const RequestParams& aParams, bool aTrustParams);

  bool
  StartRequest(PBackgroundIDBRequestParent* aActor);

  bool
  DeallocRequest(PBackgroundIDBRequestParent* aActor);

  PBackgroundIDBCursorParent*
  AllocCursor(const OpenCursorParams& aParams, bool aTrustParams);

  bool
  StartCursor(PBackgroundIDBCursorParent* aActor,
              const OpenCursorParams& aParams);

  bool
  DeallocCursor(PBackgroundIDBCursorParent* aActor);

  virtual void
  UpdateMetadata(nsresult aResult)
  { }

  virtual void
  SendCompleteNotification(nsresult aResult) = 0;

private:
  bool
  VerifyRequestParams(const RequestParams& aParams) const;

  bool
  VerifyRequestParams(const SerializedKeyRange& aKeyRange) const;

  bool
  VerifyRequestParams(const ObjectStoreAddPutParams& aParams) const;

  bool
  VerifyRequestParams(const OptionalKeyRange& aKeyRange) const;

  void
  CommitOrAbort();
};

class TransactionBase::CommitOp final
  : public DatabaseOperationBase
  , public ConnectionPool::FinishCallback
{
  friend class TransactionBase;

  nsRefPtr<TransactionBase> mTransaction;
  nsresult mResultCode;

private:
  CommitOp(TransactionBase* aTransaction, nsresult aResultCode);

  ~CommitOp()
  { }

  
  nsresult
  WriteAutoIncrementCounts();

  
  void
  CommitOrRollbackAutoIncrementCounts();

  void
  AssertForeignKeyConsistency(DatabaseConnection* aConnection)
#ifdef DEBUG
  ;
#else
  { }
#endif

  NS_DECL_NSIRUNNABLE

  virtual void
  TransactionFinishedBeforeUnblock() override;

  virtual void
  TransactionFinishedAfterUnblock() override;

public:
  NS_DECL_ISUPPORTS_INHERITED
};

class NormalTransaction final
  : public TransactionBase
  , public PBackgroundIDBTransactionParent
{
  friend class Database;

  nsTArray<nsRefPtr<FullObjectStoreMetadata>> mObjectStores;

private:
  
  NormalTransaction(Database* aDatabase,
                    TransactionBase::Mode aMode,
                    nsTArray<nsRefPtr<FullObjectStoreMetadata>>& aObjectStores);

  
  ~NormalTransaction()
  { }

  bool
  IsSameProcessActor();

  
  virtual void
  SendCompleteNotification(nsresult aResult) override;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvDeleteMe() override;

  virtual bool
  RecvCommit() override;

  virtual bool
  RecvAbort(const nsresult& aResultCode) override;

  virtual PBackgroundIDBRequestParent*
  AllocPBackgroundIDBRequestParent(const RequestParams& aParams) override;

  virtual bool
  RecvPBackgroundIDBRequestConstructor(PBackgroundIDBRequestParent* aActor,
                                       const RequestParams& aParams)
                                       override;

  virtual bool
  DeallocPBackgroundIDBRequestParent(PBackgroundIDBRequestParent* aActor)
                                     override;

  virtual PBackgroundIDBCursorParent*
  AllocPBackgroundIDBCursorParent(const OpenCursorParams& aParams) override;

  virtual bool
  RecvPBackgroundIDBCursorConstructor(PBackgroundIDBCursorParent* aActor,
                                      const OpenCursorParams& aParams)
                                      override;

  virtual bool
  DeallocPBackgroundIDBCursorParent(PBackgroundIDBCursorParent* aActor)
                                    override;
};

class VersionChangeTransaction final
  : public TransactionBase
  , public PBackgroundIDBVersionChangeTransactionParent
{
  friend class OpenDatabaseOp;

  nsRefPtr<OpenDatabaseOp> mOpenDatabaseOp;
  nsRefPtr<FullDatabaseMetadata> mOldMetadata;

  bool mActorWasAlive;

private:
  
  explicit VersionChangeTransaction(OpenDatabaseOp* aOpenDatabaseOp);

  
  ~VersionChangeTransaction();

  bool
  IsSameProcessActor();

  
  bool
  CopyDatabaseMetadata();

  void
  SetActorAlive();

  
  virtual void
  UpdateMetadata(nsresult aResult) override;

  
  virtual void
  SendCompleteNotification(nsresult aResult) override;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvDeleteMe() override;

  virtual bool
  RecvCommit() override;

  virtual bool
  RecvAbort(const nsresult& aResultCode) override;

  virtual bool
  RecvCreateObjectStore(const ObjectStoreMetadata& aMetadata) override;

  virtual bool
  RecvDeleteObjectStore(const int64_t& aObjectStoreId) override;

  virtual bool
  RecvCreateIndex(const int64_t& aObjectStoreId,
                  const IndexMetadata& aMetadata) override;

  virtual bool
  RecvDeleteIndex(const int64_t& aObjectStoreId,
                  const int64_t& aIndexId) override;

  virtual PBackgroundIDBRequestParent*
  AllocPBackgroundIDBRequestParent(const RequestParams& aParams) override;

  virtual bool
  RecvPBackgroundIDBRequestConstructor(PBackgroundIDBRequestParent* aActor,
                                       const RequestParams& aParams)
                                       override;

  virtual bool
  DeallocPBackgroundIDBRequestParent(PBackgroundIDBRequestParent* aActor)
                                     override;

  virtual PBackgroundIDBCursorParent*
  AllocPBackgroundIDBCursorParent(const OpenCursorParams& aParams) override;

  virtual bool
  RecvPBackgroundIDBCursorConstructor(PBackgroundIDBCursorParent* aActor,
                                      const OpenCursorParams& aParams)
                                      override;

  virtual bool
  DeallocPBackgroundIDBCursorParent(PBackgroundIDBCursorParent* aActor)
                                    override;
};

class FactoryOp
  : public DatabaseOperationBase
  , public OpenDirectoryListener
  , public PBackgroundIDBFactoryRequestParent
{
public:
  struct MaybeBlockedDatabaseInfo;

protected:
  enum State
  {
    
    
    
    
    State_Initial,

    
    
    State_PermissionChallenge,

    
    
    
    State_PermissionRetry,

    
    
    
    State_DirectoryOpenPending,

    
    
    State_DirectoryWorkOpen,

    
    
    State_DatabaseOpenPending,

    
    
    
    
    State_DatabaseWorkOpen,

    
    
    
    
    
    
    State_BeginVersionChange,

    
    
    
    State_WaitingForOtherDatabasesToClose,

    
    
    
    State_WaitingForTransactionsToComplete,

    
    
    
    
    State_DatabaseWorkVersionChange,

    
    
    State_SendingResults,

    
    State_Completed
  };

  
  nsRefPtr<Factory> mFactory;

  
  nsRefPtr<ContentParent> mContentParent;

  
  nsRefPtr<DirectoryLock> mDirectoryLock;

  nsRefPtr<FactoryOp> mDelayedOp;
  nsTArray<MaybeBlockedDatabaseInfo> mMaybeBlockedDatabases;

  const CommonFactoryRequestParams mCommonParams;
  nsCString mGroup;
  nsCString mOrigin;
  nsCString mDatabaseId;
  State mState;
  bool mIsApp;
  bool mEnforcingQuota;
  const bool mDeleting;
  bool mBlockedDatabaseOpen;
  bool mChromeWriteAccessAllowed;

public:
  void
  NoteDatabaseBlocked(Database* aDatabase);

  virtual void
  NoteDatabaseClosed(Database* aDatabase) = 0;

#ifdef DEBUG
  bool
  HasBlockedDatabases() const
  {
    return !mMaybeBlockedDatabases.IsEmpty();
  }
#endif

protected:
  FactoryOp(Factory* aFactory,
            already_AddRefed<ContentParent> aContentParent,
            const CommonFactoryRequestParams& aCommonParams,
            bool aDeleting);

  virtual
  ~FactoryOp()
  {
    
    
    MOZ_ASSERT_IF(OperationMayProceed(),
                  mState == State_Initial || mState == State_Completed);
  }

  nsresult
  Open();

  nsresult
  ChallengePermission();

  nsresult
  RetryCheckPermission();

  nsresult
  DirectoryOpen();

  nsresult
  SendToIOThread();

  void
  WaitForTransactions();

  void
  FinishSendResults();

  nsresult
  SendVersionChangeMessages(DatabaseActorInfo* aDatabaseActorInfo,
                            Database* aOpeningDatabase,
                            uint64_t aOldVersion,
                            const NullableVersion& aNewVersion);

  
  virtual nsresult
  DatabaseOpen() = 0;

  virtual nsresult
  DoDatabaseWork() = 0;

  virtual nsresult
  BeginVersionChange() = 0;

  virtual nsresult
  DispatchToWorkThread() = 0;

  virtual void
  SendResults() = 0;

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD
  Run() final;

  
  virtual void
  DirectoryLockAcquired(DirectoryLock* aLock) override;

  virtual void
  DirectoryLockFailed() override;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvPermissionRetry() override;

  virtual void
  SendBlockedNotification() = 0;

private:
  nsresult
  CheckPermission(ContentParent* aContentParent,
                  PermissionRequestBase::PermissionValue* aPermission);

  static bool
  CheckAtLeastOneAppHasPermission(ContentParent* aContentParent,
                                  const nsACString& aPermissionString);

  nsresult
  FinishOpen();

  
  bool
  MustWaitFor(const FactoryOp& aExistingOp);
};

struct FactoryOp::MaybeBlockedDatabaseInfo final
{
  nsRefPtr<Database> mDatabase;
  bool mBlocked;

  MOZ_IMPLICIT MaybeBlockedDatabaseInfo(Database* aDatabase)
    : mDatabase(aDatabase)
    , mBlocked(false)
  {
    MOZ_ASSERT(aDatabase);

    MOZ_COUNT_CTOR(FactoryOp::MaybeBlockedDatabaseInfo);
  }

  ~MaybeBlockedDatabaseInfo()
  {
    MOZ_COUNT_DTOR(FactoryOp::MaybeBlockedDatabaseInfo);
  }

  bool
  operator==(const MaybeBlockedDatabaseInfo& aOther) const
  {
    return mDatabase == aOther.mDatabase;
  }

  bool
  operator<(const MaybeBlockedDatabaseInfo& aOther) const
  {
    return mDatabase < aOther.mDatabase;
  }

  Database*
  operator->() MOZ_NO_ADDREF_RELEASE_ON_RETURN
  {
    return mDatabase;
  }
};

class OpenDatabaseOp final
  : public FactoryOp
{
  friend class Database;
  friend class VersionChangeTransaction;

  class VersionChangeOp;

  const OptionalContentId mOptionalContentParentId;

  nsRefPtr<FullDatabaseMetadata> mMetadata;

  uint64_t mRequestedVersion;
  nsString mDatabaseFilePath;
  nsRefPtr<FileManager> mFileManager;

  nsRefPtr<Database> mDatabase;
  nsRefPtr<VersionChangeTransaction> mVersionChangeTransaction;

  
  
  
  VersionChangeOp* mVersionChangeOp;

  uint32_t mTelemetryId;

public:
  OpenDatabaseOp(Factory* aFactory,
                 already_AddRefed<ContentParent> aContentParent,
                 const CommonFactoryRequestParams& aParams);

  bool
  IsOtherProcessActor() const
  {
    MOZ_ASSERT(mOptionalContentParentId.type() != OptionalContentId::T__None);

    return mOptionalContentParentId.type() ==
             OptionalContentId::TContentParentId;
  }

private:
  ~OpenDatabaseOp()
  {
    MOZ_ASSERT(!mVersionChangeOp);
  }

  nsresult
  LoadDatabaseInformation(mozIStorageConnection* aConnection);

  nsresult
  SendUpgradeNeeded();

  void
  EnsureDatabaseActor();

  nsresult
  EnsureDatabaseActorIsAlive();

  void
  MetadataToSpec(DatabaseSpec& aSpec);

  void
  AssertMetadataConsistency(const FullDatabaseMetadata* aMetadata)
#ifdef DEBUG
  ;
#else
  { }
#endif

  void
  ConnectionClosedCallback();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual nsresult
  DatabaseOpen() override;

  virtual nsresult
  DoDatabaseWork() override;

  virtual nsresult
  BeginVersionChange() override;

  virtual void
  NoteDatabaseClosed(Database* aDatabase) override;

  virtual void
  SendBlockedNotification() override;

  virtual nsresult
  DispatchToWorkThread() override;

  virtual void
  SendResults() override;
};

class OpenDatabaseOp::VersionChangeOp final
  : public TransactionDatabaseOperationBase
{
  friend class OpenDatabaseOp;

  nsRefPtr<OpenDatabaseOp> mOpenDatabaseOp;
  const uint64_t mRequestedVersion;
  uint64_t mPreviousVersion;

private:
  explicit
  VersionChangeOp(OpenDatabaseOp* aOpenDatabaseOp)
    : TransactionDatabaseOperationBase(
                                     aOpenDatabaseOp->mVersionChangeTransaction,
                                     aOpenDatabaseOp->LoggingSerialNumber())
    , mOpenDatabaseOp(aOpenDatabaseOp)
    , mRequestedVersion(aOpenDatabaseOp->mRequestedVersion)
    , mPreviousVersion(aOpenDatabaseOp->mMetadata->mCommonMetadata.version())
  {
    MOZ_ASSERT(aOpenDatabaseOp);
    MOZ_ASSERT(mRequestedVersion);
  }

  ~VersionChangeOp()
  {
    MOZ_ASSERT(!mOpenDatabaseOp);
  }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual nsresult
  SendSuccessResult() override;

  virtual bool
  SendFailureResult(nsresult aResultCode) override;

  virtual void
  Cleanup() override;
};

class DeleteDatabaseOp final
  : public FactoryOp
{
  class VersionChangeOp;

  nsString mDatabaseDirectoryPath;
  nsString mDatabaseFilenameBase;
  uint64_t mPreviousVersion;

public:
  DeleteDatabaseOp(Factory* aFactory,
                   already_AddRefed<ContentParent> aContentParent,
                   const CommonFactoryRequestParams& aParams)
    : FactoryOp(aFactory, Move(aContentParent), aParams,  true)
    , mPreviousVersion(0)
  { }

private:
  ~DeleteDatabaseOp()
  { }

  void
  LoadPreviousVersion(nsIFile* aDatabaseFile);

  virtual nsresult
  DatabaseOpen() override;

  virtual nsresult
  DoDatabaseWork() override;

  virtual nsresult
  BeginVersionChange() override;

  virtual void
  NoteDatabaseClosed(Database* aDatabase) override;

  virtual void
  SendBlockedNotification() override;

  virtual nsresult
  DispatchToWorkThread() override;

  virtual void
  SendResults() override;
};

class DeleteDatabaseOp::VersionChangeOp final
  : public DatabaseOperationBase
{
  friend class DeleteDatabaseOp;

  nsRefPtr<DeleteDatabaseOp> mDeleteDatabaseOp;

private:
  explicit
  VersionChangeOp(DeleteDatabaseOp* aDeleteDatabaseOp)
    : DatabaseOperationBase(aDeleteDatabaseOp->BackgroundChildLoggingId(),
                            aDeleteDatabaseOp->LoggingSerialNumber())
    , mDeleteDatabaseOp(aDeleteDatabaseOp)
  {
    MOZ_ASSERT(aDeleteDatabaseOp);
    MOZ_ASSERT(!aDeleteDatabaseOp->mDatabaseDirectoryPath.IsEmpty());
  }

  ~VersionChangeOp()
  { }

  
  
  nsresult
  RunOnMainThread();

  nsresult
  RunOnIOThread();

  void
  RunOnOwningThread();

  nsresult
  DeleteFile(nsIFile* aDirectory,
             const nsAString& aFilename,
             QuotaManager* aQuotaManager);

  NS_DECL_NSIRUNNABLE
};

class VersionChangeTransactionOp
  : public TransactionDatabaseOperationBase
{
public:
  virtual void
  Cleanup() override;

protected:
  explicit VersionChangeTransactionOp(VersionChangeTransaction* aTransaction)
    : TransactionDatabaseOperationBase(aTransaction)
  { }

  virtual
  ~VersionChangeTransactionOp()
  { }

private:
  virtual nsresult
  SendSuccessResult() override;

  virtual bool
  SendFailureResult(nsresult aResultCode) override;
};

class CreateObjectStoreOp final
  : public VersionChangeTransactionOp
{
  friend class VersionChangeTransaction;

  const ObjectStoreMetadata mMetadata;

private:
  
  CreateObjectStoreOp(VersionChangeTransaction* aTransaction,
                      const ObjectStoreMetadata& aMetadata)
    : VersionChangeTransactionOp(aTransaction)
    , mMetadata(aMetadata)
  {
    MOZ_ASSERT(aMetadata.id());
  }

  ~CreateObjectStoreOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;
};

class DeleteObjectStoreOp final
  : public VersionChangeTransactionOp
{
  friend class VersionChangeTransaction;

  const nsRefPtr<FullObjectStoreMetadata> mMetadata;
  const bool mIsLastObjectStore;

private:
  
  DeleteObjectStoreOp(VersionChangeTransaction* aTransaction,
                      FullObjectStoreMetadata* const aMetadata,
                      const bool aIsLastObjectStore)
    : VersionChangeTransactionOp(aTransaction)
    , mMetadata(aMetadata)
    , mIsLastObjectStore(aIsLastObjectStore)
  {
    MOZ_ASSERT(aMetadata->mCommonMetadata.id());
  }

  ~DeleteObjectStoreOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;
};

class CreateIndexOp final
  : public VersionChangeTransactionOp
{
  friend class VersionChangeTransaction;

  class ThreadLocalJSRuntime;
  class UpdateIndexDataValuesFunction;

  static const unsigned int kBadThreadLocalIndex =
    static_cast<unsigned int>(-1);

  static unsigned int sThreadLocalIndex;

  const IndexMetadata mMetadata;
  Maybe<UniqueIndexTable> mMaybeUniqueIndexTable;
  nsRefPtr<FileManager> mFileManager;
  const nsCString mDatabaseId;
  const uint64_t mObjectStoreId;

private:
  
  CreateIndexOp(VersionChangeTransaction* aTransaction,
                const int64_t aObjectStoreId,
                const IndexMetadata& aMetadata);

  ~CreateIndexOp()
  { }

  nsresult
  InsertDataFromObjectStore(DatabaseConnection* aConnection);

  nsresult
  InsertDataFromObjectStoreInternal(DatabaseConnection* aConnection);

  virtual bool
  Init(TransactionBase* aTransaction) override;

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;
};

class CreateIndexOp::ThreadLocalJSRuntime final
{
  friend class CreateIndexOp;
  friend class nsAutoPtr<ThreadLocalJSRuntime>;

  static const JSClass kGlobalClass;
  static const uint32_t kRuntimeHeapSize = 768 * 1024;

  JSRuntime* mRuntime;
  JSContext* mContext;
  JSObject* mGlobal;

public:
  static ThreadLocalJSRuntime*
  GetOrCreate();

  JSContext*
  Context() const
  {
    return mContext;
  }

  JSObject*
  Global() const
  {
    return mGlobal;
  }

private:
  ThreadLocalJSRuntime()
    : mRuntime(nullptr)
    , mContext(nullptr)
    , mGlobal(nullptr)
  {
    MOZ_COUNT_CTOR(CreateIndexOp::ThreadLocalJSRuntime);
  }

  ~ThreadLocalJSRuntime()
  {
    MOZ_COUNT_DTOR(CreateIndexOp::ThreadLocalJSRuntime);

    if (mContext) {
      JS_DestroyContext(mContext);
    }

    if (mRuntime) {
      JS_DestroyRuntime(mRuntime);
    }
  }

  bool
  Init();
};

class CreateIndexOp::UpdateIndexDataValuesFunction final
  : public mozIStorageFunction
{
  nsRefPtr<CreateIndexOp> mOp;
  nsRefPtr<DatabaseConnection> mConnection;
  JSContext* mCx;

public:
  UpdateIndexDataValuesFunction(CreateIndexOp* aOp,
                                DatabaseConnection* aConnection,
                                JSContext* aCx)
    : mOp(aOp)
    , mConnection(aConnection)
    , mCx(aCx)
  {
    MOZ_ASSERT(aOp);
    MOZ_ASSERT(aConnection);
    aConnection->AssertIsOnConnectionThread();
    MOZ_ASSERT(aCx);
  }

  NS_DECL_ISUPPORTS

private:
  ~UpdateIndexDataValuesFunction()
  { }

  NS_DECL_MOZISTORAGEFUNCTION
};

class DeleteIndexOp final
  : public VersionChangeTransactionOp
{
  friend class VersionChangeTransaction;

  const int64_t mObjectStoreId;
  const int64_t mIndexId;
  const bool mUnique;
  const bool mIsLastIndex;

private:
  
  DeleteIndexOp(VersionChangeTransaction* aTransaction,
                const int64_t aObjectStoreId,
                const int64_t aIndexId,
                const bool aUnique,
                const bool aIsLastIndex);

  ~DeleteIndexOp()
  { }

  nsresult
  RemoveReferencesToIndex(DatabaseConnection* aConnection,
                          const Key& aObjectDataKey,
                          FallibleTArray<IndexDataValue>& aIndexValues);

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;
};

class NormalTransactionOp
  : public TransactionDatabaseOperationBase
  , public PBackgroundIDBRequestParent
{
  DebugOnly<bool> mResponseSent;

public:
  virtual void
  Cleanup() override;

protected:
  explicit NormalTransactionOp(TransactionBase* aTransaction)
    : TransactionDatabaseOperationBase(aTransaction)
    , mResponseSent(false)
  { }

  virtual
  ~NormalTransactionOp()
  { }

  
  
  static nsresult
  ObjectStoreHasIndexes(NormalTransactionOp* aOp,
                        DatabaseConnection* aConnection,
                        const int64_t aObjectStoreId,
                        const bool aMayHaveIndexes,
                        bool* aHasIndexes);

  
  virtual void
  GetResponse(RequestResponse& aResponse) = 0;

private:
  virtual nsresult
  SendSuccessResult() override;

  virtual bool
  SendFailureResult(nsresult aResultCode) override;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;
};

class ObjectStoreAddOrPutRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  struct StoredFileInfo;

  const ObjectStoreAddPutParams mParams;
  Maybe<UniqueIndexTable> mUniqueIndexTable;

  
  
  nsRefPtr<FullObjectStoreMetadata> mMetadata;

  FallibleTArray<StoredFileInfo> mStoredFileInfos;

  nsRefPtr<FileManager> mFileManager;

  Key mResponse;
  const nsCString mGroup;
  const nsCString mOrigin;
  const PersistenceType mPersistenceType;
  const bool mOverwrite;
  const bool mObjectStoreMayHaveIndexes;

private:
  
  ObjectStoreAddOrPutRequestOp(TransactionBase* aTransaction,
                               const RequestParams& aParams);

  ~ObjectStoreAddOrPutRequestOp()
  { }

  nsresult
  RemoveOldIndexDataValues(DatabaseConnection* aConnection);

  nsresult
  CopyFileData(nsIInputStream* aInputStream, nsIOutputStream* aOutputStream);

  virtual bool
  Init(TransactionBase* aTransaction) override;

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override;

  virtual void
  Cleanup() override;
};

struct ObjectStoreAddOrPutRequestOp::StoredFileInfo final
{
  nsRefPtr<DatabaseFile> mFileActor;
  nsRefPtr<FileInfo> mFileInfo;
  nsCOMPtr<nsIInputStream> mInputStream;
  bool mCopiedSuccessfully;

  StoredFileInfo()
    : mCopiedSuccessfully(false)
  {
    AssertIsOnBackgroundThread();

    MOZ_COUNT_CTOR(ObjectStoreAddOrPutRequestOp::StoredFileInfo);
  }

  ~StoredFileInfo()
  {
    AssertIsOnBackgroundThread();

    MOZ_COUNT_DTOR(ObjectStoreAddOrPutRequestOp::StoredFileInfo);
  }
};

class ObjectStoreGetRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  const uint32_t mObjectStoreId;
  nsRefPtr<FileManager> mFileManager;
  const OptionalKeyRange mOptionalKeyRange;
  AutoFallibleTArray<StructuredCloneReadInfo, 1> mResponse;
  PBackgroundParent* mBackgroundParent;
  const uint32_t mLimit;
  const bool mGetAll;

private:
  
  ObjectStoreGetRequestOp(TransactionBase* aTransaction,
                          const RequestParams& aParams,
                          bool aGetAll);

  ~ObjectStoreGetRequestOp()
  { }

  nsresult
  ConvertResponse(uint32_t aIndex,
                  SerializedStructuredCloneReadInfo& aSerializedInfo);

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override;
};

class ObjectStoreGetAllKeysRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  const ObjectStoreGetAllKeysParams mParams;
  FallibleTArray<Key> mResponse;

private:
  
  ObjectStoreGetAllKeysRequestOp(TransactionBase* aTransaction,
                                 const ObjectStoreGetAllKeysParams& aParams)
    : NormalTransactionOp(aTransaction)
    , mParams(aParams)
  { }

  ~ObjectStoreGetAllKeysRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override;
};

class ObjectStoreDeleteRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  const ObjectStoreDeleteParams mParams;
  ObjectStoreDeleteResponse mResponse;
  const bool mObjectStoreMayHaveIndexes;

private:
  ObjectStoreDeleteRequestOp(TransactionBase* aTransaction,
                             const ObjectStoreDeleteParams& aParams);

  ~ObjectStoreDeleteRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override
  {
    aResponse = Move(mResponse);
  }
};

class ObjectStoreClearRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  const ObjectStoreClearParams mParams;
  ObjectStoreClearResponse mResponse;
  const bool mObjectStoreMayHaveIndexes;

private:
  ObjectStoreClearRequestOp(TransactionBase* aTransaction,
                            const ObjectStoreClearParams& aParams);

  ~ObjectStoreClearRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override
  {
    aResponse = Move(mResponse);
  }
};

class ObjectStoreCountRequestOp final
  : public NormalTransactionOp
{
  friend class TransactionBase;

  const ObjectStoreCountParams mParams;
  ObjectStoreCountResponse mResponse;

private:
  ObjectStoreCountRequestOp(TransactionBase* aTransaction,
                            const ObjectStoreCountParams& aParams)
    : NormalTransactionOp(aTransaction)
    , mParams(aParams)
  { }

  ~ObjectStoreCountRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override
  {
    aResponse = Move(mResponse);
  }
};

class IndexRequestOpBase
  : public NormalTransactionOp
{
protected:
  const nsRefPtr<FullIndexMetadata> mMetadata;

protected:
  IndexRequestOpBase(TransactionBase* aTransaction,
                     const RequestParams& aParams)
    : NormalTransactionOp(aTransaction)
    , mMetadata(IndexMetadataForParams(aTransaction, aParams))
  { }

  virtual
  ~IndexRequestOpBase()
  { }

private:
  static already_AddRefed<FullIndexMetadata>
  IndexMetadataForParams(TransactionBase* aTransaction,
                         const RequestParams& aParams);
};

class IndexGetRequestOp final
  : public IndexRequestOpBase
{
  friend class TransactionBase;

  nsRefPtr<FileManager> mFileManager;
  const OptionalKeyRange mOptionalKeyRange;
  AutoFallibleTArray<StructuredCloneReadInfo, 1> mResponse;
  PBackgroundParent* mBackgroundParent;
  const uint32_t mLimit;
  const bool mGetAll;

private:
  
  IndexGetRequestOp(TransactionBase* aTransaction,
                    const RequestParams& aParams,
                    bool aGetAll);

  ~IndexGetRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override;
};

class IndexGetKeyRequestOp final
  : public IndexRequestOpBase
{
  friend class TransactionBase;

  const OptionalKeyRange mOptionalKeyRange;
  AutoFallibleTArray<Key, 1> mResponse;
  const uint32_t mLimit;
  const bool mGetAll;

private:
  
  IndexGetKeyRequestOp(TransactionBase* aTransaction,
                       const RequestParams& aParams,
                       bool aGetAll);

  ~IndexGetKeyRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override;
};

class IndexCountRequestOp final
  : public IndexRequestOpBase
{
  friend class TransactionBase;

  const IndexCountParams mParams;
  IndexCountResponse mResponse;

private:
  
  IndexCountRequestOp(TransactionBase* aTransaction,
                      const RequestParams& aParams)
    : IndexRequestOpBase(aTransaction, aParams)
    , mParams(aParams.get_IndexCountParams())
  { }

  ~IndexCountRequestOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual void
  GetResponse(RequestResponse& aResponse) override
  {
    aResponse = Move(mResponse);
  }
};

class Cursor final :
    public PBackgroundIDBCursorParent
{
  friend class TransactionBase;

  class ContinueOp;
  class CursorOpBase;
  class OpenOp;

public:
  typedef OpenCursorParams::Type Type;

private:
  nsRefPtr<TransactionBase> mTransaction;
  nsRefPtr<FileManager> mFileManager;
  PBackgroundParent* mBackgroundParent;

  
  
  
  nsRefPtr<FullObjectStoreMetadata> mObjectStoreMetadata;
  nsRefPtr<FullIndexMetadata> mIndexMetadata;

  const int64_t mObjectStoreId;
  const int64_t mIndexId;

  nsCString mContinueQuery;
  nsCString mContinueToQuery;

  Key mKey;
  Key mObjectKey;
  Key mRangeKey;

  CursorOpBase* mCurrentlyRunningOp;

  const Type mType;
  const Direction mDirection;

  const bool mUniqueIndex;
  const bool mIsSameProcessActor;
  bool mActorDestroyed;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::indexedDB::Cursor)

private:
  
  Cursor(TransactionBase* aTransaction,
         Type aType,
         FullObjectStoreMetadata* aObjectStoreMetadata,
         FullIndexMetadata* aIndexMetadata,
         Direction aDirection);

  
  ~Cursor()
  {
    MOZ_ASSERT(mActorDestroyed);
  }

  bool
  VerifyRequestParams(const CursorRequestParams& aParams) const;

  
  bool
  Start(const OpenCursorParams& aParams);

  void
  SendResponseInternal(CursorResponse& aResponse,
                       const nsTArray<StructuredCloneFile>& aFiles);

  
  bool
  SendResponse(const CursorResponse& aResponse) = delete;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvDeleteMe() override;

  virtual bool
  RecvContinue(const CursorRequestParams& aParams) override;
};

class Cursor::CursorOpBase
  : public TransactionDatabaseOperationBase
{
protected:
  nsRefPtr<Cursor> mCursor;
  FallibleTArray<StructuredCloneFile> mFiles;

  CursorResponse mResponse;

  DebugOnly<bool> mResponseSent;

protected:
  explicit CursorOpBase(Cursor* aCursor)
    : TransactionDatabaseOperationBase(aCursor->mTransaction)
    , mCursor(aCursor)
    , mResponseSent(false)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aCursor);
  }

  virtual
  ~CursorOpBase()
  { }

  virtual bool
  SendFailureResult(nsresult aResultCode) override;

  virtual void
  Cleanup() override;

  nsresult
  PopulateResponseFromStatement(DatabaseConnection::CachedStatement& aStmt);
};

class Cursor::OpenOp final
  : public Cursor::CursorOpBase
{
  friend class Cursor;

  const OptionalKeyRange mOptionalKeyRange;

private:
  
  OpenOp(Cursor* aCursor,
         const OptionalKeyRange& aOptionalKeyRange)
    : CursorOpBase(aCursor)
    , mOptionalKeyRange(aOptionalKeyRange)
  { }

  
  ~OpenOp()
  { }

  void
  GetRangeKeyInfo(bool aLowerBound, Key* aKey, bool* aOpen);

  nsresult
  DoObjectStoreDatabaseWork(DatabaseConnection* aConnection);

  nsresult
  DoObjectStoreKeyDatabaseWork(DatabaseConnection* aConnection);

  nsresult
  DoIndexDatabaseWork(DatabaseConnection* aConnection);

  nsresult
  DoIndexKeyDatabaseWork(DatabaseConnection* aConnection);

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual nsresult
  SendSuccessResult() override;
};

class Cursor::ContinueOp final
  : public Cursor::CursorOpBase
{
  friend class Cursor;

  const CursorRequestParams mParams;

private:
  
  ContinueOp(Cursor* aCursor, const CursorRequestParams& aParams)
    : CursorOpBase(aCursor)
    , mParams(aParams)
  {
    MOZ_ASSERT(aParams.type() != CursorRequestParams::T__None);
  }

  
  ~ContinueOp()
  { }

  virtual nsresult
  DoDatabaseWork(DatabaseConnection* aConnection) override;

  virtual nsresult
  SendSuccessResult() override;
};

class PermissionRequestHelper final
  : public PermissionRequestBase
  , public PIndexedDBPermissionRequestParent
{
  bool mActorDestroyed;

public:
  PermissionRequestHelper(Element* aOwnerElement,
                          nsIPrincipal* aPrincipal)
    : PermissionRequestBase(aOwnerElement, aPrincipal)
    , mActorDestroyed(false)
  { }

protected:
  ~PermissionRequestHelper()
  { }

private:
  virtual void
  OnPromptComplete(PermissionValue aPermissionValue) override;

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;
};





struct DatabaseActorInfo final
{
  friend class nsAutoPtr<DatabaseActorInfo>;

  nsRefPtr<FullDatabaseMetadata> mMetadata;
  nsTArray<Database*> mLiveDatabases;
  nsRefPtr<FactoryOp> mWaitingFactoryOp;

  DatabaseActorInfo(FullDatabaseMetadata* aMetadata,
                    Database* aDatabase)
    : mMetadata(aMetadata)
  {
    MOZ_ASSERT(aDatabase);

    MOZ_COUNT_CTOR(DatabaseActorInfo);

    mLiveDatabases.AppendElement(aDatabase);
  }

private:
  ~DatabaseActorInfo()
  {
    MOZ_ASSERT(mLiveDatabases.IsEmpty());
    MOZ_ASSERT(!mWaitingFactoryOp ||
               !mWaitingFactoryOp->HasBlockedDatabases());

    MOZ_COUNT_DTOR(DatabaseActorInfo);
  }
};

class DatabaseLoggingInfo final
{
#ifdef DEBUG
  
  friend class Factory;
#endif

  LoggingInfo mLoggingInfo;

public:
  explicit
  DatabaseLoggingInfo(const LoggingInfo& aLoggingInfo)
    : mLoggingInfo(aLoggingInfo)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(aLoggingInfo.nextTransactionSerialNumber());
    MOZ_ASSERT(aLoggingInfo.nextVersionChangeTransactionSerialNumber());
    MOZ_ASSERT(aLoggingInfo.nextRequestSerialNumber());
  }

  const nsID&
  Id() const
  {
    AssertIsOnBackgroundThread();

    return mLoggingInfo.backgroundChildLoggingId();
  }

  int64_t
  NextTransactionSN(IDBTransaction::Mode aMode)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mLoggingInfo.nextTransactionSerialNumber() < INT64_MAX);
    MOZ_ASSERT(mLoggingInfo.nextVersionChangeTransactionSerialNumber() >
                 INT64_MIN);

    if (aMode == IDBTransaction::VERSION_CHANGE) {
      return mLoggingInfo.nextVersionChangeTransactionSerialNumber()--;
    }

    return mLoggingInfo.nextTransactionSerialNumber()++;
  }

  uint64_t
  NextRequestSN()
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mLoggingInfo.nextRequestSerialNumber() < UINT64_MAX);

    return mLoggingInfo.nextRequestSerialNumber()++;
  }

  NS_INLINE_DECL_REFCOUNTING(DatabaseLoggingInfo)

private:
  ~DatabaseLoggingInfo();
};

class NonMainThreadHackBlobImpl final
  : public BlobImplFile
{
public:
  NonMainThreadHackBlobImpl(nsIFile* aFile, FileInfo* aFileInfo)
    : BlobImplFile(aFile, aFileInfo)
  {
    
    
    
    
    
    
    
    
    
    
    
    
    
    mContentType.Truncate();
    mIsFile = false;
  }

private:
  ~NonMainThreadHackBlobImpl()
  { }
};

class QuotaClient final
  : public mozilla::dom::quota::Client
  , public nsIObserver
{
  
  
  static const uint32_t kIdleObserverTimeSec = 1;

  
  
  static const PRTime kMinVacuumAge =
    PRTime(PR_USEC_PER_SEC) * 60 * 60 * 24 * 7;

  
  
  static const int32_t kPercentUnorderedThreshold = 30;

  
  
  static const int32_t kPercentFileSizeGrowthThreshold = 10;

  
  
  static const int32_t kMaxFreelistThreshold = 5;

  
  
  static const int32_t kPercentUnusedThreshold = 20;

  class AbortOperationsRunnable;
  class AutoProgressHandler;
  class GetDirectoryLockListener;
  struct MaintenanceInfoBase;
  struct MultipleMaintenanceInfo;
  class ShutdownWorkThreadsRunnable;
  struct SingleMaintenanceInfo;

  typedef nsClassHashtable<nsCStringHashKey, MultipleMaintenanceInfo>
          MaintenanceInfoHashtable;

  enum MaintenanceAction {
    MaintenanceAction_Nothing = 0,
    MaintenanceAction_IncrementalVacuum,
    MaintenanceAction_FullVacuum
  };

  static QuotaClient* sInstance;

  nsCOMPtr<nsIEventTarget> mBackgroundThread;
  nsRefPtr<ShutdownWorkThreadsRunnable> mShutdownRunnable;
  nsRefPtr<nsThreadPool> mMaintenanceThreadPool;
  PRTime mMaintenanceStartTime;
  Atomic<uint32_t> mMaintenanceRunId;
  UniquePtr<MaintenanceInfoHashtable> mMaintenanceInfoHashtable;
  bool mShutdownRequested;
  bool mIdleObserverRegistered;

public:
  QuotaClient();

  static QuotaClient*
  GetInstance()
  {
    MOZ_ASSERT(NS_IsMainThread());

    return sInstance;
  }

  static bool
  IsShuttingDownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (sInstance) {
      return sInstance->IsShuttingDown();
    }

    return QuotaManager::IsShuttingDown();
  }

  static bool
  IsShuttingDownOnNonMainThread()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    return QuotaManager::IsShuttingDown();
  }

  bool
  IsShuttingDown() const
  {
    MOZ_ASSERT(NS_IsMainThread());

    return mShutdownRequested;
  }

  bool
  IdleMaintenanceMustEnd(uint32_t aRunId) const
  {
    if (mMaintenanceRunId != aRunId) {
      MOZ_ASSERT(mMaintenanceRunId > aRunId);
      return true;
    }

    return false;
  }

  void
  NoteBackgroundThread(nsIEventTarget* aBackgroundThread);

  NS_DECL_THREADSAFE_ISUPPORTS

  virtual mozilla::dom::quota::Client::Type
  GetType() override;

  virtual nsresult
  InitOrigin(PersistenceType aPersistenceType,
             const nsACString& aGroup,
             const nsACString& aOrigin,
             UsageInfo* aUsageInfo) override;

  virtual nsresult
  GetUsageForOrigin(PersistenceType aPersistenceType,
                    const nsACString& aGroup,
                    const nsACString& aOrigin,
                    UsageInfo* aUsageInfo) override;

  virtual void
  OnOriginClearCompleted(PersistenceType aPersistenceType,
                         const nsACString& aOrigin)
                         override;

  virtual void
  ReleaseIOThreadObjects() override;

  virtual void
  AbortOperations(const nsACString& aOrigin) override;

  virtual void
  AbortOperationsForProcess(ContentParentId aContentParentId) override;

  virtual void
  PerformIdleMaintenance() override;

  virtual void
  ShutdownWorkThreads() override;

private:
  ~QuotaClient();

  nsresult
  GetDirectory(PersistenceType aPersistenceType,
               const nsACString& aOrigin,
               nsIFile** aDirectory);

  nsresult
  GetUsageForDirectoryInternal(nsIFile* aDirectory,
                               UsageInfo* aUsageInfo,
                               bool aDatabaseFiles);

  void
  RemoveIdleObserver();

  void
  StartIdleMaintenance();

  void
  StopIdleMaintenance();

  
  
  void
  FindDatabasesForIdleMaintenance(uint32_t aRunId);

  
  
  void
  GetDirectoryLockForIdleMaintenance(
                                    uint32_t aRunId,
                                    MultipleMaintenanceInfo&& aMaintenanceInfo);

  
  
  void
  ScheduleIdleMaintenance(uint32_t aRunId,
                          const nsACString& aKey,
                          const MultipleMaintenanceInfo& aMaintenanceInfo);

  
  
  
  void
  PerformIdleMaintenanceOnDatabase(uint32_t aRunId,
                                   const nsACString& aKey,
                                   SingleMaintenanceInfo&& aMaintenanceInfo);

  
  void
  PerformIdleMaintenanceOnDatabaseInternal(
                                 uint32_t aRunId,
                                 const SingleMaintenanceInfo& aMaintenanceInfo);

  
  nsresult
  CheckIntegrity(mozIStorageConnection* aConnection, bool* aOk);

  
  nsresult
  DetermineMaintenanceAction(mozIStorageConnection* aConnection,
                             nsIFile* aDatabaseFile,
                             MaintenanceAction* aMaintenanceAction);

  
  void
  IncrementalVacuum(mozIStorageConnection* aConnection);

  
  void
  FullVacuum(mozIStorageConnection* aConnection,
             nsIFile* aDatabaseFile);

  
  
  void
  MaybeReleaseDirectoryLockForIdleMaintenance(
                                     const nsACString& aKey,
                                     const nsAString& aDatabasePath);

  NS_DECL_NSIOBSERVER
};

class MOZ_STACK_CLASS QuotaClient::AutoProgressHandler final
  : public mozIStorageProgressHandler
{
  QuotaClient* mQuotaClient;
  mozIStorageConnection* mConnection;
  uint32_t mRunId;

  NS_DECL_OWNINGTHREAD

  
  
  
  DebugOnly<nsrefcnt> mDEBUGRefCnt;

public:
  AutoProgressHandler(QuotaClient* aQuotaClient, uint32_t aRunId)
    : mQuotaClient(aQuotaClient)
    , mConnection(nullptr)
    , mRunId(aRunId)
    , mDEBUGRefCnt(0)
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsOnBackgroundThread());
    NS_ASSERT_OWNINGTHREAD(QuotaClient::AutoProgressHandler);
    MOZ_ASSERT(aQuotaClient);
  }

  ~AutoProgressHandler()
  {
    NS_ASSERT_OWNINGTHREAD(QuotaClient::AutoProgressHandler);

    if (mConnection) {
      Unregister();
    }

    MOZ_ASSERT(!mDEBUGRefCnt);
  }

  nsresult
  Register(mozIStorageConnection* aConnection);

  
  
  NS_DECL_ISUPPORTS_INHERITED

private:
  void
  Unregister();

  NS_DECL_MOZISTORAGEPROGRESSHANDLER

  
  void*
  operator new(size_t) = delete;
  void*
  operator new[](size_t) = delete;
  void
  operator delete(void*) = delete;
  void
  operator delete[](void*) = delete;
};

struct QuotaClient::MaintenanceInfoBase
{
  const nsCString mGroup;
  const nsCString mOrigin;
  const PersistenceType mPersistenceType;

protected:
  MaintenanceInfoBase(const nsACString& aGroup,
                      const nsACString& aOrigin,
                      PersistenceType aPersistenceType)
    : mGroup(aGroup)
    , mOrigin(aOrigin)
    , mPersistenceType(aPersistenceType)
  {
    MOZ_ASSERT(!aGroup.IsEmpty());
    MOZ_ASSERT(!aOrigin.IsEmpty());
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_INVALID);

    MOZ_COUNT_CTOR(QuotaClient::MaintenanceInfoBase);
  }

  MaintenanceInfoBase(MaintenanceInfoBase&& aOther)
    : mGroup(Move(aOther.mGroup))
    , mOrigin(Move(aOther.mOrigin))
    , mPersistenceType(Move(aOther.mPersistenceType))
  {
    MOZ_COUNT_CTOR(QuotaClient::MaintenanceInfoBase);
  }

  ~MaintenanceInfoBase()
  {
    MOZ_COUNT_DTOR(QuotaClient::MaintenanceInfoBase);
  }

  MaintenanceInfoBase(const MaintenanceInfoBase& aOther) = delete;
};

struct QuotaClient::SingleMaintenanceInfo final
  : public MaintenanceInfoBase
{
  const nsString mDatabasePath;

  SingleMaintenanceInfo(const nsACString& aGroup,
                        const nsACString& aOrigin,
                        PersistenceType aPersistenceType,
                        const nsAString& aDatabasePath)
   : MaintenanceInfoBase(aGroup, aOrigin, aPersistenceType)
   , mDatabasePath(aDatabasePath)
  {
    MOZ_ASSERT(!aDatabasePath.IsEmpty());
  }

  SingleMaintenanceInfo(SingleMaintenanceInfo&& aOther)
    : MaintenanceInfoBase(Move(aOther))
    , mDatabasePath(Move(aOther.mDatabasePath))
  {
    MOZ_ASSERT(!mDatabasePath.IsEmpty());
  }

  SingleMaintenanceInfo(const SingleMaintenanceInfo& aOther) = delete;
};

struct QuotaClient::MultipleMaintenanceInfo final
  : public MaintenanceInfoBase
{
  nsTArray<nsString> mDatabasePaths;
  nsRefPtr<DirectoryLock> mDirectoryLock;
  const bool mIsApp;

  MultipleMaintenanceInfo(const nsACString& aGroup,
                          const nsACString& aOrigin,
                          PersistenceType aPersistenceType,
                          bool aIsApp,
                          nsTArray<nsString>&& aDatabasePaths)
   : MaintenanceInfoBase(aGroup, aOrigin, aPersistenceType)
   , mDatabasePaths(Move(aDatabasePaths))
   , mIsApp(aIsApp)
  {
#ifdef DEBUG
    MOZ_ASSERT(!mDatabasePaths.IsEmpty());
    for (const nsString& databasePath : mDatabasePaths) {
      MOZ_ASSERT(!databasePath.IsEmpty());
    }
#endif
  }

  MultipleMaintenanceInfo(MultipleMaintenanceInfo&& aOther)
    : MaintenanceInfoBase(Move(aOther))
    , mDatabasePaths(Move(aOther.mDatabasePaths))
    , mDirectoryLock(Move(aOther.mDirectoryLock))
    , mIsApp(Move(aOther.mIsApp))
  {
#ifdef DEBUG
    MOZ_ASSERT(!mDatabasePaths.IsEmpty());
    for (const nsString& databasePath : mDatabasePaths) {
      MOZ_ASSERT(!databasePath.IsEmpty());
    }
#endif
  }

  MultipleMaintenanceInfo(const MultipleMaintenanceInfo& aOther) = delete;
};

class QuotaClient::ShutdownWorkThreadsRunnable final
  : public nsRunnable
{
  nsRefPtr<QuotaClient> mQuotaClient;

public:
  explicit ShutdownWorkThreadsRunnable(QuotaClient* aQuotaClient)
    : mQuotaClient(aQuotaClient)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aQuotaClient);
    MOZ_ASSERT(QuotaClient::GetInstance() == aQuotaClient);
    MOZ_ASSERT(aQuotaClient->mShutdownRequested);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~ShutdownWorkThreadsRunnable()
  {
    MOZ_ASSERT(!mQuotaClient);
  }

  NS_DECL_NSIRUNNABLE
};

class QuotaClient::AbortOperationsRunnable final
  : public nsRunnable
{
  const ContentParentId mContentParentId;
  const nsCString mOrigin;

  nsTArray<nsRefPtr<Database>> mDatabases;

public:
  explicit AbortOperationsRunnable(const nsACString& aOrigin)
    : mOrigin(aOrigin)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!mOrigin.IsEmpty());
  }

  explicit AbortOperationsRunnable(ContentParentId aContentParentId)
    : mContentParentId(aContentParentId)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mOrigin.IsEmpty());
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~AbortOperationsRunnable()
  { }

  static PLDHashOperator
  MatchOrigin(const nsACString& aKey,
              DatabaseActorInfo* aValue,
              void* aClosure);

  static PLDHashOperator
  MatchContentParentId(const nsACString& aKey,
                       DatabaseActorInfo* aValue,
                       void* aClosure);

  NS_DECL_NSIRUNNABLE
};

class QuotaClient::GetDirectoryLockListener final
  : public OpenDirectoryListener
{
  nsRefPtr<QuotaClient> mQuotaClient;
  const uint32_t mRunId;
  const nsCString mKey;

public:
  GetDirectoryLockListener(QuotaClient* aQuotaClient,
                           uint32_t aRunId,
                           const nsACString& aKey)
    : mQuotaClient(aQuotaClient)
    , mRunId(aRunId)
    , mKey(aKey)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aQuotaClient);
    MOZ_ASSERT(QuotaClient::GetInstance() == aQuotaClient);
  }

  NS_INLINE_DECL_REFCOUNTING(QuotaClient::GetDirectoryLockListener, override)

private:
  ~GetDirectoryLockListener()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  
  virtual void
  DirectoryLockAcquired(DirectoryLock* aLock) override;

  virtual void
  DirectoryLockFailed() override;
};

#ifdef DEBUG

class DEBUGThreadSlower final
  : public nsIThreadObserver
{
public:
  DEBUGThreadSlower()
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(kDEBUGThreadSleepMS);
  }

  NS_DECL_ISUPPORTS

private:
  ~DEBUGThreadSlower()
  {
    AssertIsOnBackgroundThread();
  }

  NS_DECL_NSITHREADOBSERVER
};

#endif





bool
TokenizerIgnoreNothing(char16_t )
{
  return false;
}

nsresult
ConvertFileIdsToArray(const nsAString& aFileIds,
                      nsTArray<int64_t>& aResult)
{
  nsCharSeparatedTokenizerTemplate<TokenizerIgnoreNothing>
    tokenizer(aFileIds, ' ');

  nsAutoString token;
  nsresult rv;

  while (tokenizer.hasMoreTokens()) {
    token = tokenizer.nextToken();
    MOZ_ASSERT(!token.IsEmpty());

    int32_t id = token.ToInteger(&rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    aResult.AppendElement(id);
  }

  return NS_OK;
}

bool
GetDatabaseBaseFilename(const nsAString& aFilename,
                        nsDependentSubstring& aDatabaseBaseFilename)
{
  MOZ_ASSERT(!aFilename.IsEmpty());
  MOZ_ASSERT(aDatabaseBaseFilename.IsEmpty());

  NS_NAMED_LITERAL_STRING(sqlite, ".sqlite");

  if (!StringEndsWith(aFilename, sqlite) ||
      aFilename.Length() == sqlite.Length()) {
    return false;
  }

  MOZ_ASSERT(aFilename.Length() > sqlite.Length());

  aDatabaseBaseFilename.Rebind(aFilename,
                               0,
                               aFilename.Length() - sqlite.Length());
  return true;
}

nsresult
ConvertBlobsToActors(PBackgroundParent* aBackgroundActor,
                     FileManager* aFileManager,
                     const nsTArray<StructuredCloneFile>& aFiles,
                     FallibleTArray<PBlobParent*>& aActors,
                     FallibleTArray<intptr_t>& aFileInfos)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aBackgroundActor);
  MOZ_ASSERT(aFileManager);
  MOZ_ASSERT(aActors.IsEmpty());
  MOZ_ASSERT(aFileInfos.IsEmpty());

  if (aFiles.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsIFile> directory = aFileManager->GetDirectory();
  if (NS_WARN_IF(!directory)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  DebugOnly<bool> exists;
  MOZ_ASSERT(NS_SUCCEEDED(directory->Exists(&exists)));
  MOZ_ASSERT(exists);

  DebugOnly<bool> isDirectory;
  MOZ_ASSERT(NS_SUCCEEDED(directory->IsDirectory(&isDirectory)));
  MOZ_ASSERT(isDirectory);

  const uint32_t count = aFiles.Length();

  if (NS_WARN_IF(!aActors.SetCapacity(count, fallible))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  const bool collectFileInfos =
    !BackgroundParent::IsOtherProcessActor(aBackgroundActor);

  if (collectFileInfos &&
      NS_WARN_IF(!aFileInfos.SetCapacity(count, fallible))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t index = 0; index < count; index++) {
    const StructuredCloneFile& file = aFiles[index];

    const int64_t fileId = file.mFileInfo->Id();
    MOZ_ASSERT(fileId > 0);

    nsCOMPtr<nsIFile> nativeFile =
      aFileManager->GetFileForId(directory, fileId);
    if (NS_WARN_IF(!nativeFile)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    MOZ_ASSERT(NS_SUCCEEDED(nativeFile->Exists(&exists)));
    MOZ_ASSERT(exists);

    DebugOnly<bool> isFile;
    MOZ_ASSERT(NS_SUCCEEDED(nativeFile->IsFile(&isFile)));
    MOZ_ASSERT(isFile);

    nsRefPtr<BlobImpl> impl =
      new NonMainThreadHackBlobImpl(nativeFile, file.mFileInfo);

    PBlobParent* actor =
      BackgroundParent::GetOrCreateActorForBlobImpl(aBackgroundActor, impl);
    if (!actor) {
      
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    MOZ_ALWAYS_TRUE(aActors.AppendElement(actor, fallible));

    if (collectFileInfos) {
      nsRefPtr<FileInfo> fileInfo = file.mFileInfo;

      
      auto transferedFileInfo =
        reinterpret_cast<intptr_t>(fileInfo.forget().take());
      MOZ_ALWAYS_TRUE(aFileInfos.AppendElement(transferedFileInfo, fallible));
    }
  }

  return NS_OK;
}





typedef nsTArray<nsRefPtr<FactoryOp>> FactoryOpArray;

StaticAutoPtr<FactoryOpArray> gFactoryOps;


typedef nsClassHashtable<nsCStringHashKey, DatabaseActorInfo>
        DatabaseActorHashtable;

StaticAutoPtr<DatabaseActorHashtable> gLiveDatabaseHashtable;

StaticRefPtr<ConnectionPool> gConnectionPool;

typedef nsDataHashtable<nsIDHashKey, DatabaseLoggingInfo*>
        DatabaseLoggingInfoHashtable;

StaticAutoPtr<DatabaseLoggingInfoHashtable> gLoggingInfoHashtable;

typedef nsDataHashtable<nsUint32HashKey, uint32_t> TelemetryIdHashtable;

StaticAutoPtr<TelemetryIdHashtable> gTelemetryIdHashtable;


StaticAutoPtr<Mutex> gTelemetryIdMutex;

#ifdef DEBUG

StaticRefPtr<DEBUGThreadSlower> gDEBUGThreadSlower;

#endif 


uint32_t
TelemetryIdForFile(nsIFile* aFile)
{
  

  MOZ_ASSERT(aFile);
  MOZ_ASSERT(gTelemetryIdMutex);

  
  
  
  
  
  

  nsString filename;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aFile->GetLeafName(filename)));

  
  NS_NAMED_LITERAL_STRING(sqliteExtension, ".sqlite");

  MOZ_ASSERT(StringEndsWith(filename, sqliteExtension));

  filename.Truncate(filename.Length() - sqliteExtension.Length());

  
  nsCOMPtr<nsIFile> idbDirectory;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aFile->GetParent(getter_AddRefs(idbDirectory))));

  DebugOnly<nsString> idbLeafName;
  MOZ_ASSERT(NS_SUCCEEDED(idbDirectory->GetLeafName(idbLeafName)));
  MOZ_ASSERT(static_cast<nsString&>(idbLeafName).EqualsLiteral("idb"));

  
  nsCOMPtr<nsIFile> originDirectory;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    idbDirectory->GetParent(getter_AddRefs(originDirectory))));

  nsString origin;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(originDirectory->GetLeafName(origin)));

  
  
  
  if (origin.EqualsLiteral("chrome") ||
      origin.EqualsLiteral("moz-safe-about+home")) {
    return 0;
  }

  
  nsCOMPtr<nsIFile> persistenceDirectory;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    originDirectory->GetParent(getter_AddRefs(persistenceDirectory))));

  nsString persistence;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(persistenceDirectory->GetLeafName(persistence)));

  NS_NAMED_LITERAL_STRING(separator, "*");

  uint32_t hashValue = HashString(persistence + separator +
                                  origin + separator +
                                  filename);

  MutexAutoLock lock(*gTelemetryIdMutex);

  if (!gTelemetryIdHashtable) {
    gTelemetryIdHashtable = new TelemetryIdHashtable();
  }

  uint32_t id;
  if (!gTelemetryIdHashtable->Get(hashValue, &id)) {
    static uint32_t sNextId = 1;

    
    id = sNextId++;

    gTelemetryIdHashtable->Put(hashValue, id);
  }

  return id;
}

} 





PBackgroundIDBFactoryParent*
AllocPBackgroundIDBFactoryParent(const LoggingInfo& aLoggingInfo)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread())) {
    return nullptr;
  }

  if (NS_WARN_IF(!aLoggingInfo.nextTransactionSerialNumber()) ||
      NS_WARN_IF(!aLoggingInfo.nextVersionChangeTransactionSerialNumber()) ||
      NS_WARN_IF(!aLoggingInfo.nextRequestSerialNumber())) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  nsRefPtr<Factory> actor = Factory::Create(aLoggingInfo);
  MOZ_ASSERT(actor);

  return actor.forget().take();
}

bool
RecvPBackgroundIDBFactoryConstructor(PBackgroundIDBFactoryParent* aActor,
                                     const LoggingInfo& )
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnNonMainThread());

  return true;
}

bool
DeallocPBackgroundIDBFactoryParent(PBackgroundIDBFactoryParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  nsRefPtr<Factory> actor = dont_AddRef(static_cast<Factory*>(aActor));
  return true;
}

PIndexedDBPermissionRequestParent*
AllocPIndexedDBPermissionRequestParent(Element* aOwnerElement,
                                       nsIPrincipal* aPrincipal)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<PermissionRequestHelper> actor =
    new PermissionRequestHelper(aOwnerElement, aPrincipal);
  return actor.forget().take();
}

bool
RecvPIndexedDBPermissionRequestConstructor(
                                      PIndexedDBPermissionRequestParent* aActor)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aActor);

  auto* actor = static_cast<PermissionRequestHelper*>(aActor);

  PermissionRequestBase::PermissionValue permission;
  nsresult rv = actor->PromptIfNeeded(&permission);
  if (NS_FAILED(rv)) {
    return false;
  }

  if (permission != PermissionRequestBase::kPermissionPrompt) {
    unused <<
      PIndexedDBPermissionRequestParent::Send__delete__(actor, permission);
  }

  return true;
}

bool
DeallocPIndexedDBPermissionRequestParent(
                                      PIndexedDBPermissionRequestParent* aActor)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aActor);

  nsRefPtr<PermissionRequestHelper> actor =
    dont_AddRef(static_cast<PermissionRequestHelper*>(aActor));
  return true;
}

already_AddRefed<mozilla::dom::quota::Client>
CreateQuotaClient()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<QuotaClient> client = new QuotaClient();
  return client.forget();
}





DatabaseConnection::DatabaseConnection(
                                      mozIStorageConnection* aStorageConnection,
                                      FileManager* aFileManager)
  : mStorageConnection(aStorageConnection)
  , mFileManager(aFileManager)
  , mInReadTransaction(false)
  , mInWriteTransaction(false)
#ifdef DEBUG
  , mDEBUGSavepointCount(0)
  , mDEBUGThread(PR_GetCurrentThread())
#endif
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aStorageConnection);
  MOZ_ASSERT(aFileManager);
}

DatabaseConnection::~DatabaseConnection()
{
  MOZ_ASSERT(!mStorageConnection);
  MOZ_ASSERT(!mFileManager);
  MOZ_ASSERT(!mCachedStatements.Count());
  MOZ_ASSERT(!mUpdateRefcountFunction);
  MOZ_ASSERT(!mInWriteTransaction);
  MOZ_ASSERT(!mDEBUGSavepointCount);
}

nsresult
DatabaseConnection::Init()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(NS_LITERAL_CSTRING("BEGIN;"), &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mInReadTransaction = true;

  return NS_OK;
}

nsresult
DatabaseConnection::GetCachedStatement(const nsACString& aQuery,
                                       CachedStatement* aCachedStatement)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(!aQuery.IsEmpty());
  MOZ_ASSERT(aCachedStatement);
  MOZ_ASSERT(mStorageConnection);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::GetCachedStatement",
                 js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<mozIStorageStatement> stmt;

  if (!mCachedStatements.Get(aQuery, getter_AddRefs(stmt))) {
    nsresult rv =
      mStorageConnection->CreateStatement(aQuery, getter_AddRefs(stmt));
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      nsCString msg;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        mStorageConnection->GetLastErrorString(msg)));

      nsAutoCString error =
        NS_LITERAL_CSTRING("The statement '") + aQuery +
        NS_LITERAL_CSTRING("' failed to compile with the error message '") +
        msg + NS_LITERAL_CSTRING("'.");

      NS_WARNING(error.get());
#endif
      return rv;
    }

    mCachedStatements.Put(aQuery, stmt);
  }

  aCachedStatement->Assign(this, stmt.forget());
  return NS_OK;
}

nsresult
DatabaseConnection::BeginWriteTransaction()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::BeginWriteTransaction",
                 js::ProfileEntry::Category::STORAGE);

  
  CachedStatement rollbackStmt;
  nsresult rv =
    GetCachedStatement(NS_LITERAL_CSTRING("ROLLBACK;"), &rollbackStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = rollbackStmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mInReadTransaction = false;

  if (!mUpdateRefcountFunction) {
    MOZ_ASSERT(mFileManager);

    nsRefPtr<UpdateRefcountFunction> function =
      new UpdateRefcountFunction(this, mFileManager);

    rv =
      mStorageConnection->CreateFunction(NS_LITERAL_CSTRING("update_refcount"),
                                          2,
                                         function);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    mUpdateRefcountFunction.swap(function);
  }

  CachedStatement beginStmt;
  rv = GetCachedStatement(NS_LITERAL_CSTRING("BEGIN IMMEDIATE;"), &beginStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = beginStmt->Execute();
  if (rv == NS_ERROR_STORAGE_BUSY) {
    NS_WARNING("Received NS_ERROR_STORAGE_BUSY when attempting to start write "
               "transaction, retrying for up to 10 seconds");

    
    
    TimeStamp start = TimeStamp::NowLoRes();

    while (true) {
      PR_Sleep(PR_MillisecondsToInterval(100));

      rv = beginStmt->Execute();
      if (rv != NS_ERROR_STORAGE_BUSY ||
          TimeStamp::NowLoRes() - start > TimeDuration::FromSeconds(10)) {
        break;
      }
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mInWriteTransaction = true;

  return NS_OK;
}

nsresult
DatabaseConnection::CommitWriteTransaction()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::CommitWriteTransaction",
                 js::ProfileEntry::Category::STORAGE);

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(NS_LITERAL_CSTRING("COMMIT;"), &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mInWriteTransaction = false;
  return NS_OK;
}

void
DatabaseConnection::RollbackWriteTransaction()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(mStorageConnection);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::RollbackWriteTransaction",
                 js::ProfileEntry::Category::STORAGE);

  if (!mInWriteTransaction) {
    return;
  }

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = GetCachedStatement(NS_LITERAL_CSTRING("ROLLBACK;"), &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  
  unused << stmt->Execute();

  mInWriteTransaction = false;
}

void
DatabaseConnection::FinishWriteTransaction()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::FinishWriteTransaction",
                 js::ProfileEntry::Category::STORAGE);

  if (mUpdateRefcountFunction) {
    mUpdateRefcountFunction->Reset();
  }

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(NS_LITERAL_CSTRING("BEGIN;"), &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  mInReadTransaction = true;
}

nsresult
DatabaseConnection::StartSavepoint()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(mUpdateRefcountFunction);
  MOZ_ASSERT(mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::StartSavepoint",
                 js::ProfileEntry::Category::STORAGE);

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(NS_LITERAL_CSTRING(SAVEPOINT_CLAUSE), &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mUpdateRefcountFunction->StartSavepoint();

#ifdef DEBUG
  MOZ_ASSERT(mDEBUGSavepointCount < UINT32_MAX);
  mDEBUGSavepointCount++;
#endif

  return NS_OK;
}

nsresult
DatabaseConnection::ReleaseSavepoint()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(mUpdateRefcountFunction);
  MOZ_ASSERT(mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::ReleaseSavepoint",
                 js::ProfileEntry::Category::STORAGE);

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(
    NS_LITERAL_CSTRING("RELEASE " SAVEPOINT_CLAUSE),
    &stmt);
  if (NS_SUCCEEDED(rv)) {
    rv = stmt->Execute();
    if (NS_SUCCEEDED(rv)) {
      mUpdateRefcountFunction->ReleaseSavepoint();

#ifdef DEBUG
      MOZ_ASSERT(mDEBUGSavepointCount);
      mDEBUGSavepointCount--;
#endif
    }
  }

  return rv;
}

nsresult
DatabaseConnection::RollbackSavepoint()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(mUpdateRefcountFunction);
  MOZ_ASSERT(mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::RollbackSavepoint",
                 js::ProfileEntry::Category::STORAGE);

#ifdef DEBUG
  MOZ_ASSERT(mDEBUGSavepointCount);
  mDEBUGSavepointCount--;
#endif

  mUpdateRefcountFunction->RollbackSavepoint();

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(
    NS_LITERAL_CSTRING("ROLLBACK TO " SAVEPOINT_CLAUSE),
    &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  unused << stmt->Execute();

  return NS_OK;
}

nsresult
DatabaseConnection::CheckpointInternal(CheckpointMode aMode)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::CheckpointInternal",
                 js::ProfileEntry::Category::STORAGE);

  nsAutoCString stmtString;
  stmtString.AssignLiteral("PRAGMA wal_checkpoint(");

  switch (aMode) {
    case CheckpointMode_Full:
      
      
      stmtString.AppendLiteral("FULL");
      break;

    case CheckpointMode_Restart:
      
      
      
      stmtString.AppendLiteral("RESTART");
      break;

    case CheckpointMode_Truncate:
      
      stmtString.AppendLiteral("TRUNCATE");
      break;

    default:
      MOZ_CRASH("Unknown CheckpointMode!");
  }

  stmtString.AppendLiteral(");");

  CachedStatement stmt;
  nsresult rv = GetCachedStatement(stmtString, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
DatabaseConnection::DoIdleProcessing(bool aNeedsCheckpoint)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::DoIdleProcessing",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseConnection::CachedStatement freelistStmt;
  uint32_t freelistCount;
  nsresult rv = GetFreelistCount(freelistStmt, &freelistCount);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    freelistCount = 0;
  }

  CachedStatement rollbackStmt;
  CachedStatement beginStmt;
  if (aNeedsCheckpoint || freelistCount) {
    rv = GetCachedStatement(NS_LITERAL_CSTRING("ROLLBACK;"), &rollbackStmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    rv = GetCachedStatement(NS_LITERAL_CSTRING("BEGIN;"), &beginStmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    
    
    unused << rollbackStmt->Execute();

    mInReadTransaction = false;
  }

  bool freedSomePages = false;

  if (freelistCount) {
    rv = ReclaimFreePagesWhileIdle(freelistStmt,
                                   rollbackStmt,
                                   freelistCount,
                                   aNeedsCheckpoint,
                                   &freedSomePages);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      MOZ_ASSERT(!freedSomePages);
    }

    
    MOZ_ASSERT(!mInReadTransaction);
    MOZ_ASSERT(!mInWriteTransaction);
  }

  
  if (aNeedsCheckpoint || freedSomePages) {
    rv = CheckpointInternal(CheckpointMode_Truncate);
    unused << NS_WARN_IF(NS_FAILED(rv));
  }

  
  if (beginStmt) {
    rv = beginStmt->Execute();
    if (NS_SUCCEEDED(rv)) {
      mInReadTransaction = true;
    } else {
      NS_WARNING("Falied to restart read transaction!");
    }
  }
}

nsresult
DatabaseConnection::ReclaimFreePagesWhileIdle(
                                            CachedStatement& aFreelistStatement,
                                            CachedStatement& aRollbackStatement,
                                            uint32_t aFreelistCount,
                                            bool aNeedsCheckpoint,
                                            bool* aFreedSomePages)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aFreelistStatement);
  MOZ_ASSERT(aRollbackStatement);
  MOZ_ASSERT(aFreelistCount);
  MOZ_ASSERT(aFreedSomePages);
  MOZ_ASSERT(!mInReadTransaction);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::ReclaimFreePagesWhileIdle",
                 js::ProfileEntry::Category::STORAGE);

  
  nsIThread* currentThread = NS_GetCurrentThread();
  MOZ_ASSERT(currentThread);

  if (NS_HasPendingEvents(currentThread)) {
    *aFreedSomePages = false;
    return NS_OK;
  }

  
  
  nsAutoCString stmtString;
  stmtString.AssignLiteral("PRAGMA incremental_vacuum(");
  stmtString.AppendInt(std::max(uint64_t(1), uint64_t(aFreelistCount / 10)));
  stmtString.AppendLiteral(");");

  
  CachedStatement incrementalVacuumStmt;
  nsresult rv = GetCachedStatement(stmtString, &incrementalVacuumStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  CachedStatement beginImmediateStmt;
  rv = GetCachedStatement(NS_LITERAL_CSTRING("BEGIN IMMEDIATE;"),
                          &beginImmediateStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  CachedStatement commitStmt;
  rv = GetCachedStatement(NS_LITERAL_CSTRING("COMMIT;"), &commitStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (aNeedsCheckpoint) {
    
    
    
    rv = CheckpointInternal(CheckpointMode_Restart);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  rv = beginImmediateStmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mInWriteTransaction = true;

  bool freedSomePages = false;

  while (aFreelistCount) {
    if (NS_HasPendingEvents(currentThread)) {
      
      
      
      rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      break;
    }

    rv = incrementalVacuumStmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      break;
    }

    freedSomePages = true;

    rv = GetFreelistCount(aFreelistStatement, &aFreelistCount);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      break;
    }
  }

  if (NS_SUCCEEDED(rv) && freedSomePages) {
    
    rv = commitStmt->Execute();
    if (NS_SUCCEEDED(rv)) {
      mInWriteTransaction = false;
    } else {
      NS_WARNING("Failed to commit!");
    }
  }

  if (NS_FAILED(rv)) {
    MOZ_ASSERT(mInWriteTransaction);

    
    unused << aRollbackStatement->Execute();

    mInWriteTransaction = false;

    return rv;
  }

  *aFreedSomePages = freedSomePages;
  return NS_OK;
}

nsresult
DatabaseConnection::GetFreelistCount(CachedStatement& aCachedStatement,
                                     uint32_t* aFreelistCount)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aFreelistCount);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::GetFreelistCount",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

  if (!aCachedStatement) {
    rv = GetCachedStatement(NS_LITERAL_CSTRING("PRAGMA freelist_count;"),
                            &aCachedStatement);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = aCachedStatement->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  
  
  mozStorageStatementScoper scoper(aCachedStatement);

  int32_t freelistCount;
  rv = aCachedStatement->GetInt32(0, &freelistCount);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(freelistCount >= 0);

  *aFreelistCount = uint32_t(freelistCount);
  return NS_OK;
}

void
DatabaseConnection::Close()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStorageConnection);
  MOZ_ASSERT(!mDEBUGSavepointCount);
  MOZ_ASSERT(!mInWriteTransaction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::Close",
                 js::ProfileEntry::Category::STORAGE);

  if (mUpdateRefcountFunction) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mStorageConnection->RemoveFunction(
        NS_LITERAL_CSTRING("update_refcount"))));
    mUpdateRefcountFunction = nullptr;
  }

  mCachedStatements.Clear();

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mStorageConnection->Close()));
  mStorageConnection = nullptr;

  mFileManager = nullptr;
}

DatabaseConnection::
CachedStatement::CachedStatement()
#ifdef DEBUG
  : mDEBUGConnection(nullptr)
#endif
{
  AssertIsOnConnectionThread();

  MOZ_COUNT_CTOR(DatabaseConnection::CachedStatement);
}

DatabaseConnection::
CachedStatement::~CachedStatement()
{
  AssertIsOnConnectionThread();

  MOZ_COUNT_DTOR(DatabaseConnection::CachedStatement);
}

DatabaseConnection::
CachedStatement::operator mozIStorageStatement*() const
{
  AssertIsOnConnectionThread();

  return mStatement;
}

mozIStorageStatement*
DatabaseConnection::
CachedStatement::operator->() const
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(mStatement);

  return mStatement;
}

void
DatabaseConnection::
CachedStatement::Reset()
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT_IF(mStatement, mScoper);

  if (mStatement) {
    mScoper.reset();
    mScoper.emplace(mStatement);
  }
}

void
DatabaseConnection::
CachedStatement::Assign(DatabaseConnection* aConnection,
                        already_AddRefed<mozIStorageStatement> aStatement)
{
#ifdef DEBUG
    MOZ_ASSERT(aConnection);
    aConnection->AssertIsOnConnectionThread();
    MOZ_ASSERT_IF(mDEBUGConnection, mDEBUGConnection == aConnection);

    mDEBUGConnection = aConnection;
#endif
  AssertIsOnConnectionThread();

  mScoper.reset();

  mStatement = aStatement;

  if (mStatement) {
    mScoper.emplace(mStatement);
  }
}

DatabaseConnection::
AutoSavepoint::AutoSavepoint()
  : mConnection(nullptr)
#ifdef DEBUG
  , mDEBUGTransaction(nullptr)
#endif
{
  MOZ_COUNT_CTOR(DatabaseConnection::AutoSavepoint);
}

DatabaseConnection::
AutoSavepoint::~AutoSavepoint()
{
  MOZ_COUNT_DTOR(DatabaseConnection::AutoSavepoint);

  if (mConnection) {
    mConnection->AssertIsOnConnectionThread();
    MOZ_ASSERT(mDEBUGTransaction);
    MOZ_ASSERT(mDEBUGTransaction->GetMode() == IDBTransaction::READ_WRITE ||
               mDEBUGTransaction->GetMode() ==
                 IDBTransaction::READ_WRITE_FLUSH ||
               mDEBUGTransaction->GetMode() == IDBTransaction::VERSION_CHANGE);

    if (NS_FAILED(mConnection->RollbackSavepoint())) {
      NS_WARNING("Failed to rollback savepoint!");
    }
  }
}

nsresult
DatabaseConnection::
AutoSavepoint::Start(const TransactionBase* aTransaction)
{
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(aTransaction->GetMode() == IDBTransaction::READ_WRITE ||
             aTransaction->GetMode() == IDBTransaction::READ_WRITE_FLUSH ||
             aTransaction->GetMode() == IDBTransaction::VERSION_CHANGE);

  DatabaseConnection* connection = aTransaction->GetDatabase()->GetConnection();
  MOZ_ASSERT(connection);
  connection->AssertIsOnConnectionThread();

  MOZ_ASSERT(!mConnection);
  MOZ_ASSERT(!mDEBUGTransaction);

  nsresult rv = connection->StartSavepoint();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mConnection = connection;
#ifdef DEBUG
  mDEBUGTransaction = aTransaction;
#endif

  return NS_OK;
}

nsresult
DatabaseConnection::
AutoSavepoint::Commit()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mDEBUGTransaction);

  nsresult rv = mConnection->ReleaseSavepoint();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mConnection = nullptr;
#ifdef DEBUG
  mDEBUGTransaction = nullptr;
#endif

  return NS_OK;
}

DatabaseConnection::
UpdateRefcountFunction::UpdateRefcountFunction(DatabaseConnection* aConnection,
                                               FileManager* aFileManager)
  : mConnection(aConnection)
  , mFileManager(aFileManager)
  , mInSavepoint(false)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aFileManager);
}

nsresult
DatabaseConnection::
UpdateRefcountFunction::WillCommit()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::WillCommit",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseUpdateFunction function(this);
  for (auto iter = mFileInfoEntries.ConstIter(); !iter.Done(); iter.Next()) {
    auto key = iter.Key();
    FileInfoEntry* value = iter.Data();
    MOZ_ASSERT(value);

    if (value->mDelta && !function.Update(key, value->mDelta)) {
      break;
    }
  }

  nsresult rv = function.ErrorCode();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = CreateJournals();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
DatabaseConnection::
UpdateRefcountFunction::DidCommit()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::DidCommit",
                 js::ProfileEntry::Category::STORAGE);

  for (auto iter = mFileInfoEntries.ConstIter(); !iter.Done(); iter.Next()) {
    auto value = iter.Data();

    MOZ_ASSERT(value);

    if (value->mDelta) {
      value->mFileInfo->UpdateDBRefs(value->mDelta);
    }
  }

  if (NS_FAILED(RemoveJournals(mJournalsToRemoveAfterCommit))) {
    NS_WARNING("RemoveJournals failed!");
  }
}

void
DatabaseConnection::
UpdateRefcountFunction::DidAbort()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::DidAbort",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_FAILED(RemoveJournals(mJournalsToRemoveAfterAbort))) {
    NS_WARNING("RemoveJournals failed!");
  }
}

void
DatabaseConnection::
UpdateRefcountFunction::StartSavepoint()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!mInSavepoint);
  MOZ_ASSERT(!mSavepointEntriesIndex.Count());

  mInSavepoint = true;
}

void
DatabaseConnection::
UpdateRefcountFunction::ReleaseSavepoint()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mInSavepoint);

  mSavepointEntriesIndex.Clear();
  mInSavepoint = false;
}

void
DatabaseConnection::
UpdateRefcountFunction::RollbackSavepoint()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mInSavepoint);

  for (auto iter = mSavepointEntriesIndex.ConstIter();
       !iter.Done(); iter.Next()) {
    auto value = iter.Data();
    value->mDelta -= value->mSavepointDelta;
  }

  mInSavepoint = false;
  mSavepointEntriesIndex.Clear();
}

void
DatabaseConnection::
UpdateRefcountFunction::Reset()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!mSavepointEntriesIndex.Count());
  MOZ_ASSERT(!mInSavepoint);

  mJournalsToCreateBeforeCommit.Clear();
  mJournalsToRemoveAfterCommit.Clear();
  mJournalsToRemoveAfterAbort.Clear();
  mFileInfoEntries.Clear();
}

nsresult
DatabaseConnection::
UpdateRefcountFunction::ProcessValue(mozIStorageValueArray* aValues,
                                     int32_t aIndex,
                                     UpdateType aUpdateType)
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aValues);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::ProcessValue",
                 js::ProfileEntry::Category::STORAGE);

  int32_t type;
  nsresult rv = aValues->GetTypeOfIndex(aIndex, &type);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (type == mozIStorageValueArray::VALUE_TYPE_NULL) {
    return NS_OK;
  }

  nsString ids;
  rv = aValues->GetString(aIndex, ids);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsTArray<int64_t> fileIds;
  rv = ConvertFileIdsToArray(ids, fileIds);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (uint32_t i = 0; i < fileIds.Length(); i++) {
    int64_t id = fileIds.ElementAt(i);

    FileInfoEntry* entry;
    if (!mFileInfoEntries.Get(id, &entry)) {
      nsRefPtr<FileInfo> fileInfo = mFileManager->GetFileInfo(id);
      MOZ_ASSERT(fileInfo);

      entry = new FileInfoEntry(fileInfo);
      mFileInfoEntries.Put(id, entry);
    }

    if (mInSavepoint) {
      mSavepointEntriesIndex.Put(id, entry);
    }

    switch (aUpdateType) {
      case eIncrement:
        entry->mDelta++;
        if (mInSavepoint) {
          entry->mSavepointDelta++;
        }
        break;
      case eDecrement:
        entry->mDelta--;
        if (mInSavepoint) {
          entry->mSavepointDelta--;
        }
        break;
      default:
        MOZ_CRASH("Unknown update type!");
    }
  }

  return NS_OK;
}

nsresult
DatabaseConnection::
UpdateRefcountFunction::CreateJournals()
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::CreateJournals",
                 js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<nsIFile> journalDirectory = mFileManager->GetJournalDirectory();
  if (NS_WARN_IF(!journalDirectory)) {
    return NS_ERROR_FAILURE;
  }

  for (uint32_t i = 0; i < mJournalsToCreateBeforeCommit.Length(); i++) {
    int64_t id = mJournalsToCreateBeforeCommit[i];

    nsCOMPtr<nsIFile> file =
      mFileManager->GetFileForId(journalDirectory, id);
    if (NS_WARN_IF(!file)) {
      return NS_ERROR_FAILURE;
    }

    nsresult rv = file->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    mJournalsToRemoveAfterAbort.AppendElement(id);
  }

  return NS_OK;
}

nsresult
DatabaseConnection::
UpdateRefcountFunction::RemoveJournals(const nsTArray<int64_t>& aJournals)
{
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::RemoveJournals",
                 js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<nsIFile> journalDirectory = mFileManager->GetJournalDirectory();
  if (NS_WARN_IF(!journalDirectory)) {
    return NS_ERROR_FAILURE;
  }

  for (uint32_t index = 0; index < aJournals.Length(); index++) {
    nsCOMPtr<nsIFile> file =
      mFileManager->GetFileForId(journalDirectory, aJournals[index]);
    if (NS_WARN_IF(!file)) {
      return NS_ERROR_FAILURE;
    }

    if (NS_FAILED(file->Remove(false))) {
      NS_WARNING("Failed to removed journal!");
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS(DatabaseConnection::UpdateRefcountFunction,
                  mozIStorageFunction)

NS_IMETHODIMP
DatabaseConnection::
UpdateRefcountFunction::OnFunctionCall(mozIStorageValueArray* aValues,
                                       nsIVariant** _retval)
{
  MOZ_ASSERT(aValues);
  MOZ_ASSERT(_retval);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::OnFunctionCall",
                 js::ProfileEntry::Category::STORAGE);

  uint32_t numEntries;
  nsresult rv = aValues->GetNumEntries(&numEntries);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(numEntries == 2);

#ifdef DEBUG
  {
    int32_t type1 = mozIStorageValueArray::VALUE_TYPE_NULL;
    MOZ_ASSERT(NS_SUCCEEDED(aValues->GetTypeOfIndex(0, &type1)));

    int32_t type2 = mozIStorageValueArray::VALUE_TYPE_NULL;
    MOZ_ASSERT(NS_SUCCEEDED(aValues->GetTypeOfIndex(1, &type2)));

    MOZ_ASSERT(!(type1 == mozIStorageValueArray::VALUE_TYPE_NULL &&
                 type2 == mozIStorageValueArray::VALUE_TYPE_NULL));
  }
#endif

  rv = ProcessValue(aValues, 0, eDecrement);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = ProcessValue(aValues, 1, eIncrement);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

bool
DatabaseConnection::UpdateRefcountFunction::
DatabaseUpdateFunction::Update(int64_t aId,
                               int32_t aDelta)
{
  nsresult rv = UpdateInternal(aId, aDelta);
  if (NS_FAILED(rv)) {
    mErrorCode = rv;
    return false;
  }

  return true;
}

nsresult
DatabaseConnection::UpdateRefcountFunction::
DatabaseUpdateFunction::UpdateInternal(int64_t aId,
                                       int32_t aDelta)
{
  MOZ_ASSERT(mFunction);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseConnection::UpdateRefcountFunction::"
                 "DatabaseUpdateFunction::UpdateInternal",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseConnection* connection = mFunction->mConnection;
  MOZ_ASSERT(connection);
  connection->AssertIsOnConnectionThread();

  MOZ_ASSERT(connection->GetStorageConnection());

  nsresult rv;
  if (!mUpdateStatement) {
    rv = connection->GetCachedStatement(NS_LITERAL_CSTRING(
      "UPDATE file "
      "SET refcount = refcount + :delta "
      "WHERE id = :id"),
      &mUpdateStatement);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  mozStorageStatementScoper updateScoper(mUpdateStatement);

  rv = mUpdateStatement->BindInt32ByName(NS_LITERAL_CSTRING("delta"), aDelta);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mUpdateStatement->BindInt64ByName(NS_LITERAL_CSTRING("id"), aId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mUpdateStatement->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int32_t rows;
  rv = connection->GetStorageConnection()->GetAffectedRows(&rows);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (rows > 0) {
    if (!mSelectStatement) {
      rv = connection->GetCachedStatement(NS_LITERAL_CSTRING(
        "SELECT id "
        "FROM file "
        "WHERE id = :id"),
        &mSelectStatement);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    mozStorageStatementScoper selectScoper(mSelectStatement);

    rv = mSelectStatement->BindInt64ByName(NS_LITERAL_CSTRING("id"), aId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool hasResult;
    rv = mSelectStatement->ExecuteStep(&hasResult);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!hasResult) {
      
      
      mFunction->mJournalsToCreateBeforeCommit.AppendElement(aId);
    }

    return NS_OK;
  }

  if (!mInsertStatement) {
    rv = connection->GetCachedStatement(NS_LITERAL_CSTRING(
      "INSERT INTO file (id, refcount) "
      "VALUES(:id, :delta)"),
      &mInsertStatement);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  mozStorageStatementScoper insertScoper(mInsertStatement);

  rv = mInsertStatement->BindInt64ByName(NS_LITERAL_CSTRING("id"), aId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mInsertStatement->BindInt32ByName(NS_LITERAL_CSTRING("delta"), aDelta);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mInsertStatement->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mFunction->mJournalsToRemoveAfterCommit.AppendElement(aId);
  return NS_OK;
}





ConnectionPool::ConnectionPool()
  : mDatabasesMutex("ConnectionPool::mDatabasesMutex")
  , mIdleTimer(do_CreateInstance(NS_TIMER_CONTRACTID))
  , mNextTransactionId(0)
  , mTotalThreadCount(0)
  , mShutdownRequested(false)
  , mShutdownComplete(false)
#ifdef DEBUG
  , mDEBUGOwningThread(PR_GetCurrentThread())
#endif
{
  AssertIsOnOwningThread();
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mIdleTimer);
}

ConnectionPool::~ConnectionPool()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mIdleThreads.IsEmpty());
  MOZ_ASSERT(mIdleDatabases.IsEmpty());
  MOZ_ASSERT(!mIdleTimer);
  MOZ_ASSERT(mTargetIdleTime.IsNull());
  MOZ_ASSERT(!mDatabases.Count());
  MOZ_ASSERT(!mTransactions.Count());
  MOZ_ASSERT(mQueuedTransactions.IsEmpty());
  MOZ_ASSERT(mCompleteCallbacks.IsEmpty());
  MOZ_ASSERT(!mTotalThreadCount);
  MOZ_ASSERT(mShutdownRequested);
  MOZ_ASSERT(mShutdownComplete);
}

#ifdef DEBUG

void
ConnectionPool::AssertIsOnOwningThread() const
{
  MOZ_ASSERT(mDEBUGOwningThread);
  MOZ_ASSERT(PR_GetCurrentThread() == mDEBUGOwningThread);
}

#endif 


void
ConnectionPool::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
  MOZ_ASSERT(aTimer);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::IdleTimerCallback",
                 js::ProfileEntry::Category::STORAGE);

  auto* self = static_cast<ConnectionPool*>(aClosure);
  MOZ_ASSERT(self);
  MOZ_ASSERT(self->mIdleTimer);
  MOZ_ASSERT(SameCOMIdentity(self->mIdleTimer, aTimer));
  MOZ_ASSERT(!self->mTargetIdleTime.IsNull());
  MOZ_ASSERT_IF(self->mIdleDatabases.IsEmpty(), !self->mIdleThreads.IsEmpty());
  MOZ_ASSERT_IF(self->mIdleThreads.IsEmpty(), !self->mIdleDatabases.IsEmpty());

  self->mTargetIdleTime = TimeStamp();

  
  TimeStamp now = TimeStamp::NowLoRes() + TimeDuration::FromMilliseconds(500);

  uint32_t index = 0;

  for (uint32_t count = self->mIdleDatabases.Length(); index < count; index++) {
    IdleDatabaseInfo& info = self->mIdleDatabases[index];

    if (now >= info.mIdleTime) {
      if (info.mDatabaseInfo->mIdle) {
        self->PerformIdleDatabaseMaintenance(info.mDatabaseInfo);
      } else {
        self->CloseDatabase(info.mDatabaseInfo);
      }
    } else {
      break;
    }
  }

  if (index) {
    self->mIdleDatabases.RemoveElementsAt(0, index);

    index = 0;
  }

  for (uint32_t count = self->mIdleThreads.Length(); index < count; index++) {
    IdleThreadInfo& info = self->mIdleThreads[index];
    MOZ_ASSERT(info.mThreadInfo.mThread);
    MOZ_ASSERT(info.mThreadInfo.mRunnable);

    if (now >= info.mIdleTime) {
      self->ShutdownThread(info.mThreadInfo);
    } else {
      break;
    }
  }

  if (index) {
    self->mIdleThreads.RemoveElementsAt(0, index);
  }

  self->AdjustIdleTimer();
}

nsresult
ConnectionPool::GetOrCreateConnection(const Database* aDatabase,
                                      DatabaseConnection** aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aDatabase);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::GetOrCreateConnection",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseInfo* dbInfo;
  {
    MutexAutoLock lock(mDatabasesMutex);

    dbInfo = mDatabases.Get(aDatabase->Id());
  }

  MOZ_ASSERT(dbInfo);

  nsRefPtr<DatabaseConnection> connection = dbInfo->mConnection;
  if (!connection) {
    MOZ_ASSERT(!dbInfo->mDEBUGConnectionThread);

    nsCOMPtr<mozIStorageConnection> storageConnection;
    nsresult rv =
      GetStorageConnection(aDatabase->FilePath(),
                           aDatabase->Type(),
                           aDatabase->Group(),
                           aDatabase->Origin(),
                           aDatabase->TelemetryId(),
                           getter_AddRefs(storageConnection));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    connection =
      new DatabaseConnection(storageConnection, aDatabase->GetFileManager());

    rv = connection->Init();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    dbInfo->mConnection = connection;

    IDB_DEBUG_LOG(("ConnectionPool created connection 0x%p for '%s'",
                   dbInfo->mConnection.get(),
                   NS_ConvertUTF16toUTF8(aDatabase->FilePath()).get()));

#ifdef DEBUG
    dbInfo->mDEBUGConnectionThread = PR_GetCurrentThread();
#endif
  }

  dbInfo->AssertIsOnConnectionThread();

  connection.forget(aConnection);
  return NS_OK;
}

uint64_t
ConnectionPool::Start(const nsID& aBackgroundChildLoggingId,
                      const nsACString& aDatabaseId,
                      int64_t aLoggingSerialNumber,
                      const nsTArray<nsString>& aObjectStoreNames,
                      bool aIsWriteTransaction,
                      TransactionDatabaseOperationBase* aTransactionOp)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!aDatabaseId.IsEmpty());
  MOZ_ASSERT(mNextTransactionId < UINT64_MAX);
  MOZ_ASSERT(!mShutdownRequested);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::Start",
                 js::ProfileEntry::Category::STORAGE);

  const uint64_t transactionId = ++mNextTransactionId;

  DatabaseInfo* dbInfo = mDatabases.Get(aDatabaseId);

  const bool databaseInfoIsNew = !dbInfo;

  if (databaseInfoIsNew) {
    dbInfo = new DatabaseInfo(this, aDatabaseId);

    MutexAutoLock lock(mDatabasesMutex);

    mDatabases.Put(aDatabaseId, dbInfo);
  }

  TransactionInfo* transactionInfo =
    new TransactionInfo(dbInfo,
                        aBackgroundChildLoggingId,
                        aDatabaseId,
                        transactionId,
                        aLoggingSerialNumber,
                        aObjectStoreNames,
                        aIsWriteTransaction,
                        aTransactionOp);

  MOZ_ASSERT(!mTransactions.Get(transactionId));
  mTransactions.Put(transactionId, transactionInfo);

  if (aIsWriteTransaction) {
    MOZ_ASSERT(dbInfo->mWriteTransactionCount < UINT32_MAX);
    dbInfo->mWriteTransactionCount++;
  } else {
    MOZ_ASSERT(dbInfo->mReadTransactionCount < UINT32_MAX);
    dbInfo->mReadTransactionCount++;
  }

  auto& blockingTransactions = dbInfo->mBlockingTransactions;

  for (uint32_t nameIndex = 0, nameCount = aObjectStoreNames.Length();
       nameIndex < nameCount;
       nameIndex++) {
    const nsString& objectStoreName = aObjectStoreNames[nameIndex];

    TransactionInfoPair* blockInfo = blockingTransactions.Get(objectStoreName);
    if (!blockInfo) {
      blockInfo = new TransactionInfoPair();
      blockingTransactions.Put(objectStoreName, blockInfo);
    }

    
    if (TransactionInfo* blockingRead = blockInfo->mLastBlockingReads) {
      transactionInfo->mBlockedOn.PutEntry(blockingRead);
      blockingRead->mBlocking.PutEntry(transactionInfo);
    }

    if (aIsWriteTransaction) {
      if (const uint32_t writeCount = blockInfo->mLastBlockingWrites.Length()) {
        for (uint32_t writeIndex = 0; writeIndex < writeCount; writeIndex++) {
          TransactionInfo* blockingWrite =
            blockInfo->mLastBlockingWrites[writeIndex];
          MOZ_ASSERT(blockingWrite);

          transactionInfo->mBlockedOn.PutEntry(blockingWrite);
          blockingWrite->mBlocking.PutEntry(transactionInfo);
        }
      }

      blockInfo->mLastBlockingReads = transactionInfo;
      blockInfo->mLastBlockingWrites.Clear();
    } else {
      blockInfo->mLastBlockingWrites.AppendElement(transactionInfo);
    }
  }

  if (!transactionInfo->mBlockedOn.Count()) {
    unused << ScheduleTransaction(transactionInfo,
                                   false);
  }

  if (!databaseInfoIsNew && mIdleDatabases.RemoveElement(dbInfo)) {
    AdjustIdleTimer();
  }

  return transactionId;
}

void
ConnectionPool::Dispatch(uint64_t aTransactionId, nsIRunnable* aRunnable)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aRunnable);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::Dispatch",
                 js::ProfileEntry::Category::STORAGE);

  TransactionInfo* transactionInfo = mTransactions.Get(aTransactionId);
  MOZ_ASSERT(transactionInfo);
  MOZ_ASSERT(!transactionInfo->mFinished);

  if (transactionInfo->mRunning) {
    DatabaseInfo* dbInfo = transactionInfo->mDatabaseInfo;
    MOZ_ASSERT(dbInfo);
    MOZ_ASSERT(dbInfo->mThreadInfo.mThread);
    MOZ_ASSERT(dbInfo->mThreadInfo.mRunnable);
    MOZ_ASSERT(!dbInfo->mClosing);
    MOZ_ASSERT_IF(transactionInfo->mIsWriteTransaction,
                  dbInfo->mRunningWriteTransaction == transactionInfo);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      dbInfo->mThreadInfo.mThread->Dispatch(aRunnable, NS_DISPATCH_NORMAL)));
  } else {
    transactionInfo->mQueuedRunnables.AppendElement(aRunnable);
  }
}

void
ConnectionPool::Finish(uint64_t aTransactionId, FinishCallback* aCallback)
{
  AssertIsOnOwningThread();

#ifdef DEBUG
  TransactionInfo* transactionInfo = mTransactions.Get(aTransactionId);
  MOZ_ASSERT(transactionInfo);
  MOZ_ASSERT(!transactionInfo->mFinished);
#endif

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::Finish",
                 js::ProfileEntry::Category::STORAGE);

  nsRefPtr<FinishCallbackWrapper> wrapper =
    new FinishCallbackWrapper(this, aTransactionId, aCallback);

  Dispatch(aTransactionId, wrapper);

#ifdef DEBUG
  MOZ_ASSERT(!transactionInfo->mFinished);
  transactionInfo->mFinished = true;
#endif
}

void
ConnectionPool::WaitForDatabasesToComplete(nsTArray<nsCString>&& aDatabaseIds,
                                           nsIRunnable* aCallback)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!aDatabaseIds.IsEmpty());
  MOZ_ASSERT(aCallback);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::WaitForDatabasesToComplete",
                 js::ProfileEntry::Category::STORAGE);

  bool mayRunCallbackImmediately = true;

  for (uint32_t index = 0, count = aDatabaseIds.Length();
       index < count;
       index++) {
    const nsCString& databaseId = aDatabaseIds[index];
    MOZ_ASSERT(!databaseId.IsEmpty());

    if (CloseDatabaseWhenIdleInternal(databaseId)) {
      mayRunCallbackImmediately = false;
    }
  }

  if (mayRunCallbackImmediately) {
    unused << aCallback->Run();
    return;
  }

  nsAutoPtr<DatabasesCompleteCallback> callback(
    new DatabasesCompleteCallback(Move(aDatabaseIds), aCallback));
  mCompleteCallbacks.AppendElement(callback.forget());
}

void
ConnectionPool::Shutdown()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!mShutdownRequested);
  MOZ_ASSERT(!mShutdownComplete);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::Shutdown",
                 js::ProfileEntry::Category::STORAGE);

  mShutdownRequested = true;

  CancelIdleTimer();
  MOZ_ASSERT(mTargetIdleTime.IsNull());

  mIdleTimer = nullptr;

  CloseIdleDatabases();

  ShutdownIdleThreads();

  if (!mDatabases.Count()) {
    MOZ_ASSERT(!mTransactions.Count());

    Cleanup();

    MOZ_ASSERT(mShutdownComplete);
    return;
  }

  nsIThread* currentThread = NS_GetCurrentThread();
  MOZ_ASSERT(currentThread);

  while (!mShutdownComplete) {
    MOZ_ALWAYS_TRUE(NS_ProcessNextEvent(currentThread));
  }
}

void
ConnectionPool::Cleanup()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mShutdownRequested);
  MOZ_ASSERT(!mShutdownComplete);
  MOZ_ASSERT(!mDatabases.Count());
  MOZ_ASSERT(!mTransactions.Count());
  MOZ_ASSERT(mIdleThreads.IsEmpty());

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::Cleanup",
                 js::ProfileEntry::Category::STORAGE);

  if (!mCompleteCallbacks.IsEmpty()) {
    
    for (uint32_t count = mCompleteCallbacks.Length(), index = 0;
         index < count;
         index++) {
      nsAutoPtr<DatabasesCompleteCallback> completeCallback(
        mCompleteCallbacks[index].forget());
      MOZ_ASSERT(completeCallback);
      MOZ_ASSERT(completeCallback->mCallback);

      unused << completeCallback->mCallback->Run();
    }

    mCompleteCallbacks.Clear();

    
    nsIThread* currentThread = NS_GetCurrentThread();
    MOZ_ASSERT(currentThread);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_ProcessPendingEvents(currentThread)));
  }

  mShutdownComplete = true;
}

void
ConnectionPool::AdjustIdleTimer()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mIdleTimer);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::AdjustIdleTimer",
                 js::ProfileEntry::Category::STORAGE);

  
  
  TimeStamp newTargetIdleTime;
  MOZ_ASSERT(newTargetIdleTime.IsNull());

  if (!mIdleDatabases.IsEmpty()) {
    newTargetIdleTime = mIdleDatabases[0].mIdleTime;
  }

  if (!mIdleThreads.IsEmpty()) {
    const TimeStamp& idleTime = mIdleThreads[0].mIdleTime;

    if (newTargetIdleTime.IsNull() || idleTime < newTargetIdleTime) {
      newTargetIdleTime = idleTime;
    }
  }

  MOZ_ASSERT_IF(newTargetIdleTime.IsNull(), mIdleDatabases.IsEmpty());
  MOZ_ASSERT_IF(newTargetIdleTime.IsNull(), mIdleThreads.IsEmpty());

  
  if (!mTargetIdleTime.IsNull() &&
      (newTargetIdleTime.IsNull() || mTargetIdleTime != newTargetIdleTime)) {
    CancelIdleTimer();

    MOZ_ASSERT(mTargetIdleTime.IsNull());
  }

  
  if (!newTargetIdleTime.IsNull() &&
      (mTargetIdleTime.IsNull() || mTargetIdleTime != newTargetIdleTime)) {
    double delta = (newTargetIdleTime - TimeStamp::NowLoRes()).ToMilliseconds();

    uint32_t delay;
    if (delta > 0) {
      delay = uint32_t(std::min(delta, double(UINT32_MAX)));
    } else {
      delay = 0;
    }

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mIdleTimer->InitWithFuncCallback(IdleTimerCallback,
                                       this,
                                       delay,
                                       nsITimer::TYPE_ONE_SHOT)));

    mTargetIdleTime = newTargetIdleTime;
  }
}

void
ConnectionPool::CancelIdleTimer()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mIdleTimer);

  if (!mTargetIdleTime.IsNull()) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mIdleTimer->Cancel()));

    mTargetIdleTime = TimeStamp();
    MOZ_ASSERT(mTargetIdleTime.IsNull());
  }
}

void
ConnectionPool::ShutdownThread(ThreadInfo& aThreadInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aThreadInfo.mThread);
  MOZ_ASSERT(aThreadInfo.mRunnable);
  MOZ_ASSERT(mTotalThreadCount);

  nsRefPtr<ThreadRunnable> runnable;
  aThreadInfo.mRunnable.swap(runnable);

  nsCOMPtr<nsIThread> thread;
  aThreadInfo.mThread.swap(thread);

  IDB_DEBUG_LOG(("ConnectionPool shutting down thread %lu",
                 runnable->SerialNumber()));

  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(thread->Dispatch(runnable, NS_DISPATCH_NORMAL)));

  nsCOMPtr<nsIRunnable> shutdownRunnable =
    NS_NewRunnableMethod(thread, &nsIThread::Shutdown);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(shutdownRunnable)));

  mTotalThreadCount--;
}

void
ConnectionPool::CloseIdleDatabases()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mShutdownRequested);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::CloseIdleDatabases",
                 js::ProfileEntry::Category::STORAGE);

  if (!mIdleDatabases.IsEmpty()) {
    for (IdleDatabaseInfo& idleInfo : mIdleDatabases) {
      CloseDatabase(idleInfo.mDatabaseInfo);
    }
    mIdleDatabases.Clear();
  }

  if (!mDatabasesPerformingIdleMaintenance.IsEmpty()) {
    for (DatabaseInfo* dbInfo : mDatabasesPerformingIdleMaintenance) {
      MOZ_ASSERT(dbInfo);
      CloseDatabase(dbInfo);
    }
    mDatabasesPerformingIdleMaintenance.Clear();
  }
}

void
ConnectionPool::ShutdownIdleThreads()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mShutdownRequested);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::ShutdownIdleThreads",
                 js::ProfileEntry::Category::STORAGE);

  if (!mIdleThreads.IsEmpty()) {
    for (uint32_t threadCount = mIdleThreads.Length(), threadIndex = 0;
         threadIndex < threadCount;
         threadIndex++) {
      ShutdownThread(mIdleThreads[threadIndex].mThreadInfo);
    }
    mIdleThreads.Clear();
  }
}

bool
ConnectionPool::ScheduleTransaction(TransactionInfo* aTransactionInfo,
                                    bool aFromQueuedTransactions)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aTransactionInfo);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::ScheduleTransaction",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseInfo* dbInfo = aTransactionInfo->mDatabaseInfo;
  MOZ_ASSERT(dbInfo);

  dbInfo->mIdle = false;

  if (dbInfo->mClosing) {
    MOZ_ASSERT(!mIdleDatabases.Contains(dbInfo));
    MOZ_ASSERT(
      !dbInfo->mTransactionsScheduledDuringClose.Contains(aTransactionInfo));

    dbInfo->mTransactionsScheduledDuringClose.AppendElement(aTransactionInfo);
    return true;
  }

  if (!dbInfo->mThreadInfo.mThread) {
    MOZ_ASSERT(!dbInfo->mThreadInfo.mRunnable);

    if (mIdleThreads.IsEmpty()) {
      bool created = false;

      if (mTotalThreadCount < kMaxConnectionThreadCount) {
        
        nsRefPtr<ThreadRunnable> runnable = new ThreadRunnable();

        nsCOMPtr<nsIThread> newThread;
        if (NS_SUCCEEDED(NS_NewThread(getter_AddRefs(newThread), runnable))) {
          MOZ_ASSERT(newThread);

          IDB_DEBUG_LOG(("ConnectionPool created thread %lu",
                         runnable->SerialNumber()));

          dbInfo->mThreadInfo.mThread.swap(newThread);
          dbInfo->mThreadInfo.mRunnable.swap(runnable);

          mTotalThreadCount++;
          created = true;
        } else {
          NS_WARNING("Failed to make new thread!");
        }
      } else if (!mDatabasesPerformingIdleMaintenance.IsEmpty()) {
        
        
        
        nsCOMPtr<nsIRunnable> runnable = new nsRunnable();

        for (uint32_t index = mDatabasesPerformingIdleMaintenance.Length();
             index > 0;
             index--) {
          DatabaseInfo* dbInfo = mDatabasesPerformingIdleMaintenance[index - 1];
          MOZ_ASSERT(dbInfo);
          MOZ_ASSERT(dbInfo->mThreadInfo.mThread);

          MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
            dbInfo->mThreadInfo.mThread->Dispatch(runnable,
                                                  NS_DISPATCH_NORMAL)));
        }
      }

      if (!created) {
        if (!aFromQueuedTransactions) {
          MOZ_ASSERT(!mQueuedTransactions.Contains(aTransactionInfo));
          mQueuedTransactions.AppendElement(aTransactionInfo);
        }
        return false;
      }
    } else {
      const uint32_t lastIndex = mIdleThreads.Length() - 1;

      ThreadInfo& threadInfo = mIdleThreads[lastIndex].mThreadInfo;

      dbInfo->mThreadInfo.mRunnable.swap(threadInfo.mRunnable);
      dbInfo->mThreadInfo.mThread.swap(threadInfo.mThread);

      mIdleThreads.RemoveElementAt(lastIndex);

      AdjustIdleTimer();
    }
  }

  MOZ_ASSERT(dbInfo->mThreadInfo.mThread);
  MOZ_ASSERT(dbInfo->mThreadInfo.mRunnable);

  if (aTransactionInfo->mIsWriteTransaction) {
    if (dbInfo->mRunningWriteTransaction) {
      
      
      MOZ_ASSERT(
        !dbInfo->mScheduledWriteTransactions.Contains(aTransactionInfo));

      dbInfo->mScheduledWriteTransactions.AppendElement(aTransactionInfo);
      return true;
    }

    dbInfo->mRunningWriteTransaction = aTransactionInfo;
    dbInfo->mNeedsCheckpoint = true;
  }

  MOZ_ASSERT(!aTransactionInfo->mRunning);
  aTransactionInfo->mRunning = true;

  nsTArray<nsCOMPtr<nsIRunnable>>& queuedRunnables =
    aTransactionInfo->mQueuedRunnables;

  if (!queuedRunnables.IsEmpty()) {
    for (uint32_t index = 0, count = queuedRunnables.Length();
         index < count;
         index++) {
      nsCOMPtr<nsIRunnable> runnable;
      queuedRunnables[index].swap(runnable);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        dbInfo->mThreadInfo.mThread->Dispatch(runnable, NS_DISPATCH_NORMAL)));
    }

    queuedRunnables.Clear();
  }

  return true;
}

void
ConnectionPool::NoteFinishedTransaction(uint64_t aTransactionId)
{
  AssertIsOnOwningThread();

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::NoteFinishedTransaction",
                 js::ProfileEntry::Category::STORAGE);

  TransactionInfo* transactionInfo = mTransactions.Get(aTransactionId);
  MOZ_ASSERT(transactionInfo);
  MOZ_ASSERT(transactionInfo->mRunning);
  MOZ_ASSERT(transactionInfo->mFinished);

  transactionInfo->mRunning = false;

  DatabaseInfo* dbInfo = transactionInfo->mDatabaseInfo;
  MOZ_ASSERT(dbInfo);
  MOZ_ASSERT(mDatabases.Get(transactionInfo->mDatabaseId) == dbInfo);
  MOZ_ASSERT(dbInfo->mThreadInfo.mThread);
  MOZ_ASSERT(dbInfo->mThreadInfo.mRunnable);

  
  if (dbInfo->mRunningWriteTransaction == transactionInfo) {
    MOZ_ASSERT(transactionInfo->mIsWriteTransaction);
    MOZ_ASSERT(dbInfo->mNeedsCheckpoint);

    dbInfo->mRunningWriteTransaction = nullptr;

    if (!dbInfo->mScheduledWriteTransactions.IsEmpty()) {
      TransactionInfo* nextWriteTransaction =
        dbInfo->mScheduledWriteTransactions[0];
      MOZ_ASSERT(nextWriteTransaction);

      dbInfo->mScheduledWriteTransactions.RemoveElementAt(0);

      MOZ_ALWAYS_TRUE(ScheduleTransaction(nextWriteTransaction,
                                           false));
    }
  }

  const nsTArray<nsString>& objectStoreNames =
    transactionInfo->mObjectStoreNames;

  for (uint32_t index = 0, count = objectStoreNames.Length();
       index < count;
       index++) {
    TransactionInfoPair* blockInfo =
      dbInfo->mBlockingTransactions.Get(objectStoreNames[index]);
    MOZ_ASSERT(blockInfo);

    if (transactionInfo->mIsWriteTransaction &&
        blockInfo->mLastBlockingReads == transactionInfo) {
      blockInfo->mLastBlockingReads = nullptr;
    }

    blockInfo->mLastBlockingWrites.RemoveElement(transactionInfo);
  }

  for (auto iter = transactionInfo->mBlocking.Iter();
       !iter.Done();
       iter.Next()) {
    TransactionInfo* blockedInfo = iter.Get()->GetKey();
    MOZ_ASSERT(blockedInfo);
    MOZ_ASSERT(blockedInfo->mBlockedOn.Contains(transactionInfo));

    blockedInfo->mBlockedOn.RemoveEntry(transactionInfo);
    if (!blockedInfo->mBlockedOn.Count()) {
      blockedInfo->Schedule();
    }
  }

  if (transactionInfo->mIsWriteTransaction) {
    MOZ_ASSERT(dbInfo->mWriteTransactionCount);
    dbInfo->mWriteTransactionCount--;
  } else {
    MOZ_ASSERT(dbInfo->mReadTransactionCount);
    dbInfo->mReadTransactionCount--;
  }

  mTransactions.Remove(aTransactionId);

#ifdef DEBUG
  
  transactionInfo = nullptr;
#endif

  if (!dbInfo->TotalTransactionCount()) {
    MOZ_ASSERT(!dbInfo->mIdle);
    dbInfo->mIdle = true;

    NoteIdleDatabase(dbInfo);
  }
}

void
ConnectionPool::ScheduleQueuedTransactions(ThreadInfo& aThreadInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aThreadInfo.mThread);
  MOZ_ASSERT(aThreadInfo.mRunnable);
  MOZ_ASSERT(!mQueuedTransactions.IsEmpty());
  MOZ_ASSERT(!mIdleThreads.Contains(aThreadInfo));

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::ScheduleQueuedTransactions",
                 js::ProfileEntry::Category::STORAGE);

  mIdleThreads.InsertElementSorted(aThreadInfo);

  aThreadInfo.mRunnable = nullptr;
  aThreadInfo.mThread = nullptr;

  uint32_t index = 0;
  for (uint32_t count = mQueuedTransactions.Length(); index < count; index++) {
    if (!ScheduleTransaction(mQueuedTransactions[index],
                              true)) {
      break;
    }
  }

  if (index) {
    mQueuedTransactions.RemoveElementsAt(0, index);
  }

  AdjustIdleTimer();
}

void
ConnectionPool::NoteIdleDatabase(DatabaseInfo* aDatabaseInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabaseInfo);
  MOZ_ASSERT(!aDatabaseInfo->TotalTransactionCount());
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mThread);
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mRunnable);
  MOZ_ASSERT(!mIdleDatabases.Contains(aDatabaseInfo));

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::NoteIdleDatabase",
                 js::ProfileEntry::Category::STORAGE);

  const bool otherDatabasesWaiting = !mQueuedTransactions.IsEmpty();

  if (mShutdownRequested ||
      otherDatabasesWaiting ||
      aDatabaseInfo->mCloseOnIdle) {
    
    
    CloseDatabase(aDatabaseInfo);

    if (otherDatabasesWaiting) {
      
      ScheduleQueuedTransactions(aDatabaseInfo->mThreadInfo);
    } else if (mShutdownRequested) {
      
      
      
      ShutdownThread(aDatabaseInfo->mThreadInfo);
    }

    return;
  }

  mIdleDatabases.InsertElementSorted(aDatabaseInfo);

  AdjustIdleTimer();
}

void
ConnectionPool::NoteClosedDatabase(DatabaseInfo* aDatabaseInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabaseInfo);
  MOZ_ASSERT(aDatabaseInfo->mClosing);
  MOZ_ASSERT(!mIdleDatabases.Contains(aDatabaseInfo));

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::NoteClosedDatabase",
                 js::ProfileEntry::Category::STORAGE);

  aDatabaseInfo->mClosing = false;

  
  
  
  
  
  
  
  
  
  
  
  
  if (aDatabaseInfo->mThreadInfo.mThread) {
    MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mRunnable);

    if (!mQueuedTransactions.IsEmpty()) {
      
      ScheduleQueuedTransactions(aDatabaseInfo->mThreadInfo);
    } else if (!aDatabaseInfo->TotalTransactionCount()) {
      if (mShutdownRequested) {
        ShutdownThread(aDatabaseInfo->mThreadInfo);
      } else {
        MOZ_ASSERT(!mIdleThreads.Contains(aDatabaseInfo->mThreadInfo));

        mIdleThreads.InsertElementSorted(aDatabaseInfo->mThreadInfo);

        aDatabaseInfo->mThreadInfo.mRunnable = nullptr;
        aDatabaseInfo->mThreadInfo.mThread = nullptr;

        if (mIdleThreads.Length() > kMaxIdleConnectionThreadCount) {
          ShutdownThread(mIdleThreads[0].mThreadInfo);
          mIdleThreads.RemoveElementAt(0);
        }

        AdjustIdleTimer();
      }
    }
  }

  
  
  if (aDatabaseInfo->TotalTransactionCount()) {
    nsTArray<TransactionInfo*>& scheduledTransactions =
      aDatabaseInfo->mTransactionsScheduledDuringClose;

    MOZ_ASSERT(!scheduledTransactions.IsEmpty());

    for (uint32_t index = 0, count = scheduledTransactions.Length();
         index < count;
         index++) {
      unused << ScheduleTransaction(scheduledTransactions[index],
                                     false);
    }

    scheduledTransactions.Clear();

    return;
  }

  
  
  {
    MutexAutoLock lock(mDatabasesMutex);

    mDatabases.Remove(aDatabaseInfo->mDatabaseId);
  }

#ifdef DEBUG
  
  aDatabaseInfo = nullptr;
#endif

  
  
  for (uint32_t index = 0;
       index < mCompleteCallbacks.Length();
       ) {
    if (MaybeFireCallback(mCompleteCallbacks[index])) {
      mCompleteCallbacks.RemoveElementAt(index);
    } else {
      index++;
    }
  }

  
  
  if (mShutdownRequested && !mDatabases.Count()) {
    MOZ_ASSERT(!mTransactions.Count());
    Cleanup();
  }
}

bool
ConnectionPool::MaybeFireCallback(DatabasesCompleteCallback* aCallback)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(!aCallback->mDatabaseIds.IsEmpty());
  MOZ_ASSERT(aCallback->mCallback);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::MaybeFireCallback",
                 js::ProfileEntry::Category::STORAGE);

  for (uint32_t count = aCallback->mDatabaseIds.Length(), index = 0;
       index < count;
       index++) {
    const nsCString& databaseId = aCallback->mDatabaseIds[index];
    MOZ_ASSERT(!databaseId.IsEmpty());

    if (mDatabases.Get(databaseId)) {
      return false;
    }
  }

  unused << aCallback->mCallback->Run();
  return true;
}

void
ConnectionPool::PerformIdleDatabaseMaintenance(DatabaseInfo* aDatabaseInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabaseInfo);
  MOZ_ASSERT(!aDatabaseInfo->TotalTransactionCount());
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mThread);
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mRunnable);
  MOZ_ASSERT(aDatabaseInfo->mIdle);
  MOZ_ASSERT(!aDatabaseInfo->mCloseOnIdle);
  MOZ_ASSERT(!aDatabaseInfo->mClosing);
  MOZ_ASSERT(mIdleDatabases.Contains(aDatabaseInfo));
  MOZ_ASSERT(!mDatabasesPerformingIdleMaintenance.Contains(aDatabaseInfo));

  nsCOMPtr<nsIRunnable> runnable =
    new IdleConnectionRunnable(aDatabaseInfo, aDatabaseInfo->mNeedsCheckpoint);

  aDatabaseInfo->mNeedsCheckpoint = false;
  aDatabaseInfo->mIdle = false;

  mDatabasesPerformingIdleMaintenance.AppendElement(aDatabaseInfo);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aDatabaseInfo->mThreadInfo.mThread->Dispatch(runnable,
                                                 NS_DISPATCH_NORMAL)));
}

void
ConnectionPool::CloseDatabase(DatabaseInfo* aDatabaseInfo)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabaseInfo);
  MOZ_ASSERT(!aDatabaseInfo->TotalTransactionCount());
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mThread);
  MOZ_ASSERT(aDatabaseInfo->mThreadInfo.mRunnable);
  MOZ_ASSERT(!aDatabaseInfo->mClosing);

  aDatabaseInfo->mIdle = false;
  aDatabaseInfo->mNeedsCheckpoint = false;
  aDatabaseInfo->mClosing = true;

  nsCOMPtr<nsIRunnable> runnable = new CloseConnectionRunnable(aDatabaseInfo);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aDatabaseInfo->mThreadInfo.mThread->Dispatch(runnable,
                                                 NS_DISPATCH_NORMAL)));
}

bool
ConnectionPool::CloseDatabaseWhenIdleInternal(const nsACString& aDatabaseId)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!aDatabaseId.IsEmpty());

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::CloseDatabaseWhenIdleInternal",
                 js::ProfileEntry::Category::STORAGE);

  if (DatabaseInfo* dbInfo = mDatabases.Get(aDatabaseId)) {
    if (mIdleDatabases.RemoveElement(dbInfo) ||
        mDatabasesPerformingIdleMaintenance.RemoveElement(dbInfo)) {
      CloseDatabase(dbInfo);
      AdjustIdleTimer();
    } else {
      dbInfo->mCloseOnIdle = true;
    }

    return true;
  }

  return false;
}

ConnectionPool::
ConnectionRunnable::ConnectionRunnable(DatabaseInfo* aDatabaseInfo)
  : mDatabaseInfo(aDatabaseInfo)
  , mOwningThread(do_GetCurrentThread())
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aDatabaseInfo);
  MOZ_ASSERT(aDatabaseInfo->mConnectionPool);
  aDatabaseInfo->mConnectionPool->AssertIsOnOwningThread();
  MOZ_ASSERT(mOwningThread);
}

NS_IMPL_ISUPPORTS_INHERITED0(ConnectionPool::IdleConnectionRunnable,
                             ConnectionPool::ConnectionRunnable)

NS_IMETHODIMP
ConnectionPool::
IdleConnectionRunnable::Run()
{
  MOZ_ASSERT(mDatabaseInfo);
  MOZ_ASSERT(!mDatabaseInfo->mIdle);

  nsCOMPtr<nsIEventTarget> owningThread;
  mOwningThread.swap(owningThread);

  if (owningThread) {
    mDatabaseInfo->AssertIsOnConnectionThread();
    MOZ_ASSERT(mDatabaseInfo->mConnection);
    mDatabaseInfo->mConnection->DoIdleProcessing(mNeedsCheckpoint);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      owningThread->Dispatch(this, NS_DISPATCH_NORMAL)));
    return NS_OK;
  }

  nsRefPtr<ConnectionPool> connectionPool = mDatabaseInfo->mConnectionPool;
  MOZ_ASSERT(connectionPool);

  if (mDatabaseInfo->mClosing) {
    MOZ_ASSERT(!connectionPool->
                 mDatabasesPerformingIdleMaintenance.Contains(mDatabaseInfo));
  } else {
    MOZ_ALWAYS_TRUE(
      connectionPool->
        mDatabasesPerformingIdleMaintenance.RemoveElement(mDatabaseInfo));

    if (!mDatabaseInfo->TotalTransactionCount()) {
      connectionPool->NoteIdleDatabase(mDatabaseInfo);
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(ConnectionPool::CloseConnectionRunnable,
                             ConnectionPool::ConnectionRunnable)

NS_IMETHODIMP
ConnectionPool::
CloseConnectionRunnable::Run()
{
  MOZ_ASSERT(mDatabaseInfo);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::CloseConnectionRunnable::Run",
                 js::ProfileEntry::Category::STORAGE);

  if (mOwningThread) {
    MOZ_ASSERT(mDatabaseInfo->mClosing);

    nsCOMPtr<nsIEventTarget> owningThread;
    mOwningThread.swap(owningThread);

    if (mDatabaseInfo->mConnection) {
      mDatabaseInfo->AssertIsOnConnectionThread();

      mDatabaseInfo->mConnection->Close();

      IDB_DEBUG_LOG(("ConnectionPool closed connection 0x%p",
                     mDatabaseInfo->mConnection.get()));

      mDatabaseInfo->mConnection = nullptr;

#ifdef DEBUG
      mDatabaseInfo->mDEBUGConnectionThread = nullptr;
#endif
    }

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      owningThread->Dispatch(this, NS_DISPATCH_NORMAL)));
    return NS_OK;
  }

  nsRefPtr<ConnectionPool> connectionPool = mDatabaseInfo->mConnectionPool;
  MOZ_ASSERT(connectionPool);

  connectionPool->NoteClosedDatabase(mDatabaseInfo);
  return NS_OK;
}

ConnectionPool::
DatabaseInfo::DatabaseInfo(ConnectionPool* aConnectionPool,
                           const nsACString& aDatabaseId)
  : mConnectionPool(aConnectionPool)
  , mDatabaseId(aDatabaseId)
  , mRunningWriteTransaction(nullptr)
  , mReadTransactionCount(0)
  , mWriteTransactionCount(0)
  , mNeedsCheckpoint(false)
  , mIdle(false)
  , mCloseOnIdle(false)
  , mClosing(false)
#ifdef DEBUG
  , mDEBUGConnectionThread(nullptr)
#endif
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aConnectionPool);
  aConnectionPool->AssertIsOnOwningThread();
  MOZ_ASSERT(!aDatabaseId.IsEmpty());

  MOZ_COUNT_CTOR(ConnectionPool::DatabaseInfo);
}

ConnectionPool::
DatabaseInfo::~DatabaseInfo()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mConnection);
  MOZ_ASSERT(mScheduledWriteTransactions.IsEmpty());
  MOZ_ASSERT(!mRunningWriteTransaction);
  MOZ_ASSERT(!mThreadInfo.mThread);
  MOZ_ASSERT(!mThreadInfo.mRunnable);
  MOZ_ASSERT(!TotalTransactionCount());

  MOZ_COUNT_DTOR(ConnectionPool::DatabaseInfo);
}

ConnectionPool::
DatabasesCompleteCallback::DatabasesCompleteCallback(
                                             nsTArray<nsCString>&& aDatabaseIds,
                                             nsIRunnable* aCallback)
  : mDatabaseIds(Move(aDatabaseIds))
  , mCallback(aCallback)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mDatabaseIds.IsEmpty());
  MOZ_ASSERT(aCallback);

  MOZ_COUNT_CTOR(ConnectionPool::DatabasesCompleteCallback);
}

ConnectionPool::
DatabasesCompleteCallback::~DatabasesCompleteCallback()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_DTOR(ConnectionPool::DatabasesCompleteCallback);
}

ConnectionPool::
FinishCallbackWrapper::FinishCallbackWrapper(ConnectionPool* aConnectionPool,
                                             uint64_t aTransactionId,
                                             FinishCallback* aCallback)
  : mConnectionPool(aConnectionPool)
  , mCallback(aCallback)
  , mOwningThread(do_GetCurrentThread())
  , mTransactionId(aTransactionId)
  , mHasRunOnce(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aConnectionPool);
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mOwningThread);
}

ConnectionPool::
FinishCallbackWrapper::~FinishCallbackWrapper()
{
  MOZ_ASSERT(!mConnectionPool);
  MOZ_ASSERT(!mCallback);
}

NS_IMPL_ISUPPORTS_INHERITED0(ConnectionPool::FinishCallbackWrapper, nsRunnable)

nsresult
ConnectionPool::
FinishCallbackWrapper::Run()
{
  MOZ_ASSERT(mConnectionPool);
  MOZ_ASSERT(mCallback);
  MOZ_ASSERT(mOwningThread);

  PROFILER_LABEL("IndexedDB",
                 "ConnectionPool::FinishCallbackWrapper::Run",
                 js::ProfileEntry::Category::STORAGE);

  if (!mHasRunOnce) {
    MOZ_ASSERT(!IsOnBackgroundThread());

    mHasRunOnce = true;

    unused << mCallback->Run();

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mOwningThread->Dispatch(this, NS_DISPATCH_NORMAL)));

    return NS_OK;
  }

  mConnectionPool->AssertIsOnOwningThread();
  MOZ_ASSERT(mHasRunOnce);

  nsRefPtr<ConnectionPool> connectionPool = Move(mConnectionPool);
  nsRefPtr<FinishCallback> callback = Move(mCallback);

  callback->TransactionFinishedBeforeUnblock();

  connectionPool->NoteFinishedTransaction(mTransactionId);

  callback->TransactionFinishedAfterUnblock();

  return NS_OK;
}

uint32_t ConnectionPool::ThreadRunnable::sNextSerialNumber = 0;

ConnectionPool::
ThreadRunnable::ThreadRunnable()
  : mSerialNumber(++sNextSerialNumber)
  , mFirstRun(true)
  , mContinueRunning(true)
{
  AssertIsOnBackgroundThread();
}

ConnectionPool::
ThreadRunnable::~ThreadRunnable()
{
  MOZ_ASSERT(!mFirstRun);
  MOZ_ASSERT(!mContinueRunning);
}

NS_IMPL_ISUPPORTS_INHERITED0(ConnectionPool::ThreadRunnable, nsRunnable)

nsresult
ConnectionPool::
ThreadRunnable::Run()
{
#ifdef MOZ_ENABLE_PROFILER_SPS
  char stackTopGuess;
#endif 

  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mContinueRunning);

  if (!mFirstRun) {
    mContinueRunning = false;
    return NS_OK;
  }

  mFirstRun = false;

  {
    
    
    const nsPrintfCString threadName("IndexedDB #%lu", mSerialNumber);

    PR_SetCurrentThreadName(threadName.get());

#ifdef MOZ_ENABLE_PROFILER_SPS
    profiler_register_thread(threadName.get(), &stackTopGuess);
#endif 
  }

  {
    
    PROFILER_LABEL("IndexedDB",
                   "ConnectionPool::ThreadRunnable::Run",
                   js::ProfileEntry::Category::STORAGE);

    nsIThread* currentThread = NS_GetCurrentThread();
    MOZ_ASSERT(currentThread);

#ifdef DEBUG
    if (kDEBUGTransactionThreadPriority !=
          nsISupportsPriority::PRIORITY_NORMAL) {
      NS_WARNING("ConnectionPool thread debugging enabled, priority has been "
                 "modified!");

      nsCOMPtr<nsISupportsPriority> thread = do_QueryInterface(currentThread);
      MOZ_ASSERT(thread);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        thread->SetPriority(kDEBUGTransactionThreadPriority)));
    }

    if (kDEBUGTransactionThreadSleepMS) {
      NS_WARNING("TransactionThreadPool thread debugging enabled, sleeping "
                 "after every event!");
    }
#endif 

    while (mContinueRunning) {
      MOZ_ALWAYS_TRUE(NS_ProcessNextEvent(currentThread));

#ifdef DEBUG
      if (kDEBUGTransactionThreadSleepMS) {
        MOZ_ALWAYS_TRUE(
          PR_Sleep(PR_MillisecondsToInterval(kDEBUGTransactionThreadSleepMS)) ==
            PR_SUCCESS);
      }
#endif 
    }
  }

#ifdef MOZ_ENABLE_PROFILER_SPS
  profiler_unregister_thread();
#endif 

  return NS_OK;
}

ConnectionPool::
ThreadInfo::ThreadInfo()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_CTOR(ConnectionPool::ThreadInfo);
}

ConnectionPool::
ThreadInfo::ThreadInfo(const ThreadInfo& aOther)
  : mThread(aOther.mThread)
  , mRunnable(aOther.mRunnable)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aOther.mThread);
  MOZ_ASSERT(aOther.mRunnable);

  MOZ_COUNT_CTOR(ConnectionPool::ThreadInfo);
}

ConnectionPool::
ThreadInfo::~ThreadInfo()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_DTOR(ConnectionPool::ThreadInfo);
}

ConnectionPool::
IdleResource::IdleResource(const TimeStamp& aIdleTime)
  : mIdleTime(aIdleTime)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!aIdleTime.IsNull());

  MOZ_COUNT_CTOR(ConnectionPool::IdleResource);
}

ConnectionPool::
IdleResource::IdleResource(const IdleResource& aOther)
  : mIdleTime(aOther.mIdleTime)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!aOther.mIdleTime.IsNull());

  MOZ_COUNT_CTOR(ConnectionPool::IdleResource);
}

ConnectionPool::
IdleResource::~IdleResource()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_DTOR(ConnectionPool::IdleResource);
}

ConnectionPool::
IdleDatabaseInfo::IdleDatabaseInfo(DatabaseInfo* aDatabaseInfo)
  : IdleResource(TimeStamp::NowLoRes() +
                 (aDatabaseInfo->mIdle ?
                  TimeDuration::FromMilliseconds(kConnectionIdleMaintenanceMS) :
                  TimeDuration::FromMilliseconds(kConnectionIdleCloseMS)))
  , mDatabaseInfo(aDatabaseInfo)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aDatabaseInfo);

  MOZ_COUNT_CTOR(ConnectionPool::IdleDatabaseInfo);
}

ConnectionPool::
IdleDatabaseInfo::IdleDatabaseInfo(const IdleDatabaseInfo& aOther)
  : IdleResource(aOther)
  , mDatabaseInfo(aOther.mDatabaseInfo)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mDatabaseInfo);

  MOZ_COUNT_CTOR(ConnectionPool::IdleDatabaseInfo);
}

ConnectionPool::
IdleDatabaseInfo::~IdleDatabaseInfo()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mDatabaseInfo);

  MOZ_COUNT_DTOR(ConnectionPool::IdleDatabaseInfo);
}

ConnectionPool::
IdleThreadInfo::IdleThreadInfo(const ThreadInfo& aThreadInfo)
  : IdleResource(TimeStamp::NowLoRes() +
                 TimeDuration::FromMilliseconds(kConnectionThreadIdleMS))
  , mThreadInfo(aThreadInfo)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aThreadInfo.mRunnable);
  MOZ_ASSERT(aThreadInfo.mThread);

  MOZ_COUNT_CTOR(ConnectionPool::IdleThreadInfo);
}

ConnectionPool::
IdleThreadInfo::IdleThreadInfo(const IdleThreadInfo& aOther)
  : IdleResource(aOther)
  , mThreadInfo(aOther.mThreadInfo)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mThreadInfo.mRunnable);
  MOZ_ASSERT(mThreadInfo.mThread);

  MOZ_COUNT_CTOR(ConnectionPool::IdleThreadInfo);
}

ConnectionPool::
IdleThreadInfo::~IdleThreadInfo()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_DTOR(ConnectionPool::IdleThreadInfo);
}

ConnectionPool::
TransactionInfo::TransactionInfo(
                               DatabaseInfo* aDatabaseInfo,
                               const nsID& aBackgroundChildLoggingId,
                               const nsACString& aDatabaseId,
                               uint64_t aTransactionId,
                               int64_t aLoggingSerialNumber,
                               const nsTArray<nsString>& aObjectStoreNames,
                               bool aIsWriteTransaction,
                               TransactionDatabaseOperationBase* aTransactionOp)
  : mDatabaseInfo(aDatabaseInfo)
  , mBackgroundChildLoggingId(aBackgroundChildLoggingId)
  , mDatabaseId(aDatabaseId)
  , mTransactionId(aTransactionId)
  , mLoggingSerialNumber(aLoggingSerialNumber)
  , mObjectStoreNames(aObjectStoreNames)
  , mIsWriteTransaction(aIsWriteTransaction)
  , mRunning(false)
  , mFinished(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aDatabaseInfo);
  aDatabaseInfo->mConnectionPool->AssertIsOnOwningThread();

  MOZ_COUNT_CTOR(ConnectionPool::TransactionInfo);

  if (aTransactionOp) {
    mQueuedRunnables.AppendElement(aTransactionOp);
  }
}

ConnectionPool::
TransactionInfo::~TransactionInfo()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mBlockedOn.Count());
  MOZ_ASSERT(mQueuedRunnables.IsEmpty());
  MOZ_ASSERT(!mRunning);
  MOZ_ASSERT(mFinished);

  MOZ_COUNT_DTOR(ConnectionPool::TransactionInfo);
}

void
ConnectionPool::
TransactionInfo::Schedule()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mDatabaseInfo);

  ConnectionPool* connectionPool = mDatabaseInfo->mConnectionPool;
  MOZ_ASSERT(connectionPool);
  connectionPool->AssertIsOnOwningThread();

  unused <<
    connectionPool->ScheduleTransaction(this,
                                         false);
}

ConnectionPool::
TransactionInfoPair::TransactionInfoPair()
  : mLastBlockingReads(nullptr)
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_CTOR(ConnectionPool::TransactionInfoPair);
}

ConnectionPool::
TransactionInfoPair::~TransactionInfoPair()
{
  AssertIsOnBackgroundThread();

  MOZ_COUNT_DTOR(ConnectionPool::TransactionInfoPair);
}





bool
FullObjectStoreMetadata::HasLiveIndexes() const
{
  AssertIsOnBackgroundThread();

  for (auto iter = mIndexes.ConstIter(); !iter.Done(); iter.Next()) {
    if (!iter.Data()->mDeleted) {
      return true;
    }
  }

  return false;
}

already_AddRefed<FullDatabaseMetadata>
FullDatabaseMetadata::Duplicate() const
{
  AssertIsOnBackgroundThread();

  
  
  nsRefPtr<FullDatabaseMetadata> newMetadata =
    new FullDatabaseMetadata(mCommonMetadata);

  newMetadata->mDatabaseId = mDatabaseId;
  newMetadata->mFilePath = mFilePath;
  newMetadata->mNextObjectStoreId = mNextObjectStoreId;
  newMetadata->mNextIndexId = mNextIndexId;

  for (auto iter = mObjectStores.ConstIter(); !iter.Done(); iter.Next()) {
    auto key = iter.Key();
    auto value = iter.Data();

    nsRefPtr<FullObjectStoreMetadata> newOSMetadata =
      new FullObjectStoreMetadata();

    newOSMetadata->mCommonMetadata = value->mCommonMetadata;
    newOSMetadata->mNextAutoIncrementId = value->mNextAutoIncrementId;
    newOSMetadata->mComittedAutoIncrementId = value->mComittedAutoIncrementId;

    for (auto iter = value->mIndexes.ConstIter(); !iter.Done(); iter.Next()) {
      auto key = iter.Key();
      auto value = iter.Data();

      nsRefPtr<FullIndexMetadata> newIndexMetadata = new FullIndexMetadata();

      newIndexMetadata->mCommonMetadata = value->mCommonMetadata;

      if (NS_WARN_IF(!newOSMetadata->mIndexes.Put(key, newIndexMetadata,
                                                  fallible))) {
        return nullptr;
      }
    }

    MOZ_ASSERT(value->mIndexes.Count() == newOSMetadata->mIndexes.Count());

    if (NS_WARN_IF(!newMetadata->mObjectStores.Put(key, newOSMetadata,
                                                   fallible))) {
      return nullptr;
    }
  }

  MOZ_ASSERT(mObjectStores.Count() == newMetadata->mObjectStores.Count());

  return newMetadata.forget();
}

DatabaseLoggingInfo::~DatabaseLoggingInfo()
{
  AssertIsOnBackgroundThread();

  if (gLoggingInfoHashtable) {
    const nsID& backgroundChildLoggingId =
      mLoggingInfo.backgroundChildLoggingId();

    MOZ_ASSERT(gLoggingInfoHashtable->Get(backgroundChildLoggingId) == this);

    gLoggingInfoHashtable->Remove(backgroundChildLoggingId);
  }
}





uint64_t Factory::sFactoryInstanceCount = 0;

Factory::Factory(already_AddRefed<DatabaseLoggingInfo> aLoggingInfo)
  : mLoggingInfo(Move(aLoggingInfo))
  , mActorDestroyed(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnNonMainThread());
}

Factory::~Factory()
{
  MOZ_ASSERT(mActorDestroyed);
}


already_AddRefed<Factory>
Factory::Create(const LoggingInfo& aLoggingInfo)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnNonMainThread());

  
  if (!sFactoryInstanceCount) {
    MOZ_ASSERT(!gFactoryOps);
    gFactoryOps = new FactoryOpArray();

    MOZ_ASSERT(!gLiveDatabaseHashtable);
    gLiveDatabaseHashtable = new DatabaseActorHashtable();

    MOZ_ASSERT(!gLoggingInfoHashtable);
    gLoggingInfoHashtable = new DatabaseLoggingInfoHashtable();

#ifdef DEBUG
    if (kDEBUGThreadPriority != nsISupportsPriority::PRIORITY_NORMAL) {
      NS_WARNING("PBackground thread debugging enabled, priority has been "
                 "modified!");
      nsCOMPtr<nsISupportsPriority> thread =
        do_QueryInterface(NS_GetCurrentThread());
      MOZ_ASSERT(thread);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(thread->SetPriority(kDEBUGThreadPriority)));
    }

    if (kDEBUGThreadSleepMS) {
      NS_WARNING("PBackground thread debugging enabled, sleeping after every "
                 "event!");
      nsCOMPtr<nsIThreadInternal> thread =
        do_QueryInterface(NS_GetCurrentThread());
      MOZ_ASSERT(thread);

      gDEBUGThreadSlower = new DEBUGThreadSlower();

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(thread->AddObserver(gDEBUGThreadSlower)));
    }
#endif 
  }

  nsRefPtr<DatabaseLoggingInfo> loggingInfo =
    gLoggingInfoHashtable->Get(aLoggingInfo.backgroundChildLoggingId());
  if (loggingInfo) {
    MOZ_ASSERT(aLoggingInfo.backgroundChildLoggingId() == loggingInfo->Id());
#if !DISABLE_ASSERTS_FOR_FUZZING
    NS_WARN_IF_FALSE(aLoggingInfo.nextTransactionSerialNumber() ==
                       loggingInfo->mLoggingInfo.nextTransactionSerialNumber(),
                     "NextTransactionSerialNumber doesn't match!");
    NS_WARN_IF_FALSE(aLoggingInfo.nextVersionChangeTransactionSerialNumber() ==
                       loggingInfo->mLoggingInfo.
                         nextVersionChangeTransactionSerialNumber(),
                     "NextVersionChangeTransactionSerialNumber doesn't match!");
    NS_WARN_IF_FALSE(aLoggingInfo.nextRequestSerialNumber() ==
                       loggingInfo->mLoggingInfo.nextRequestSerialNumber(),
                     "NextRequestSerialNumber doesn't match!");
#endif 
  } else {
    loggingInfo = new DatabaseLoggingInfo(aLoggingInfo);
    gLoggingInfoHashtable->Put(aLoggingInfo.backgroundChildLoggingId(),
                               loggingInfo);
  }

  nsRefPtr<Factory> actor = new Factory(loggingInfo.forget());

  sFactoryInstanceCount++;

  return actor.forget();
}

void
Factory::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  mActorDestroyed = true;

  
  if (!(--sFactoryInstanceCount)) {
    MOZ_ASSERT(gLoggingInfoHashtable);
    gLoggingInfoHashtable = nullptr;

    MOZ_ASSERT(gLiveDatabaseHashtable);
    MOZ_ASSERT(!gLiveDatabaseHashtable->Count());
    gLiveDatabaseHashtable = nullptr;

    MOZ_ASSERT(gFactoryOps);
    MOZ_ASSERT(gFactoryOps->IsEmpty());
    gFactoryOps = nullptr;

#ifdef DEBUG
    if (kDEBUGThreadPriority != nsISupportsPriority::PRIORITY_NORMAL) {
      nsCOMPtr<nsISupportsPriority> thread =
        do_QueryInterface(NS_GetCurrentThread());
      MOZ_ASSERT(thread);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        thread->SetPriority(nsISupportsPriority::PRIORITY_NORMAL)));
    }

    if (kDEBUGThreadSleepMS) {
      MOZ_ASSERT(gDEBUGThreadSlower);

      nsCOMPtr<nsIThreadInternal> thread =
        do_QueryInterface(NS_GetCurrentThread());
      MOZ_ASSERT(thread);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(thread->RemoveObserver(gDEBUGThreadSlower)));

      gDEBUGThreadSlower = nullptr;
    }
#endif 
  }
}

bool
Factory::RecvDeleteMe()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  return PBackgroundIDBFactoryParent::Send__delete__(this);
}

bool
Factory::RecvIncrementLoggingRequestSerialNumber()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mLoggingInfo);

  mLoggingInfo->NextRequestSN();
  return true;
}

PBackgroundIDBFactoryRequestParent*
Factory::AllocPBackgroundIDBFactoryRequestParent(
                                            const FactoryRequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != FactoryRequestParams::T__None);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread())) {
    return nullptr;
  }

  const CommonFactoryRequestParams* commonParams;

  switch (aParams.type()) {
    case FactoryRequestParams::TOpenDatabaseRequestParams: {
      const OpenDatabaseRequestParams& params =
         aParams.get_OpenDatabaseRequestParams();
      commonParams = &params.commonParams();
      break;
    }

    case FactoryRequestParams::TDeleteDatabaseRequestParams: {
      const DeleteDatabaseRequestParams& params =
         aParams.get_DeleteDatabaseRequestParams();
      commonParams = &params.commonParams();
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  MOZ_ASSERT(commonParams);

  const DatabaseMetadata& metadata = commonParams->metadata();
  if (NS_WARN_IF(metadata.persistenceType() != PERSISTENCE_TYPE_PERSISTENT &&
                 metadata.persistenceType() != PERSISTENCE_TYPE_TEMPORARY &&
                 metadata.persistenceType() != PERSISTENCE_TYPE_DEFAULT)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  const PrincipalInfo& principalInfo = commonParams->principalInfo();
  if (NS_WARN_IF(principalInfo.type() == PrincipalInfo::TNullPrincipalInfo)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  if (NS_WARN_IF(principalInfo.type() == PrincipalInfo::TSystemPrincipalInfo &&
                 metadata.persistenceType() != PERSISTENCE_TYPE_PERSISTENT)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  nsRefPtr<ContentParent> contentParent =
    BackgroundParent::GetContentParent(Manager());

  nsRefPtr<FactoryOp> actor;
  if (aParams.type() == FactoryRequestParams::TOpenDatabaseRequestParams) {
    actor = new OpenDatabaseOp(this,
                               contentParent.forget(),
                               *commonParams);
  } else {
    actor = new DeleteDatabaseOp(this, contentParent.forget(), *commonParams);
  }

  
  return actor.forget().take();
}

bool
Factory::RecvPBackgroundIDBFactoryRequestConstructor(
                                     PBackgroundIDBFactoryRequestParent* aActor,
                                     const FactoryRequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != FactoryRequestParams::T__None);
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnNonMainThread());

  auto* op = static_cast<FactoryOp*>(aActor);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(op)));
  return true;
}

bool
Factory::DeallocPBackgroundIDBFactoryRequestParent(
                                     PBackgroundIDBFactoryRequestParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  
  nsRefPtr<FactoryOp> op = dont_AddRef(static_cast<FactoryOp*>(aActor));
  return true;
}

PBackgroundIDBDatabaseParent*
Factory::AllocPBackgroundIDBDatabaseParent(
                                   const DatabaseSpec& aSpec,
                                   PBackgroundIDBFactoryRequestParent* aRequest)
{
  MOZ_CRASH("PBackgroundIDBDatabaseParent actors should be constructed "
            "manually!");
}

bool
Factory::DeallocPBackgroundIDBDatabaseParent(
                                           PBackgroundIDBDatabaseParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  nsRefPtr<Database> database = dont_AddRef(static_cast<Database*>(aActor));
  return true;
}





void
WaitForTransactionsHelper::WaitForTransactions()
{
  MOZ_ASSERT(mState == State_Initial);

  unused << this->Run();
}

void
WaitForTransactionsHelper::MaybeWaitForTransactions()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mState == State_Initial);

  nsRefPtr<ConnectionPool> connectionPool = gConnectionPool.get();
  if (connectionPool) {
    nsTArray<nsCString> ids(1);
    ids.AppendElement(mDatabaseId);

    mState = State_WaitingForTransactions;

    connectionPool->WaitForDatabasesToComplete(Move(ids), this);
    return;
  }

  DispatchToMainThread();
}

void
WaitForTransactionsHelper::DispatchToMainThread()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mState == State_Initial || mState == State_WaitingForTransactions);

  mState = State_DispatchToMainThread;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));
}

void
WaitForTransactionsHelper::MaybeWaitForFileHandles()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DispatchToMainThread);

  FileService* service = FileService::Get();
  if (service) {
    nsTArray<nsCString> ids(1);
    ids.AppendElement(mDatabaseId);

    mState = State_WaitingForFileHandles;

    service->WaitForStoragesToComplete(ids, this);

    MOZ_ASSERT(ids.IsEmpty());
    return;
  }

  DispatchToOwningThread();
}

void
WaitForTransactionsHelper::DispatchToOwningThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DispatchToMainThread ||
             mState == State_WaitingForFileHandles);

  mState = State_DispatchToOwningThread;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                       NS_DISPATCH_NORMAL)));
}

void
WaitForTransactionsHelper::CallCallback()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mState == State_DispatchToOwningThread);

  nsCOMPtr<nsIRunnable> callback;
  mCallback.swap(callback);

  callback->Run();

  mState = State_Complete;
}

NS_IMPL_ISUPPORTS_INHERITED0(WaitForTransactionsHelper,
                             nsRunnable)

NS_IMETHODIMP
WaitForTransactionsHelper::Run()
{
  MOZ_ASSERT(mState != State_Complete);
  MOZ_ASSERT(mCallback);

  switch (mState) {
    case State_Initial:
      MaybeWaitForTransactions();
      break;

    case State_WaitingForTransactions:
      DispatchToMainThread();
      break;

    case State_DispatchToMainThread:
      MaybeWaitForFileHandles();
      break;

    case State_WaitingForFileHandles:
      DispatchToOwningThread();
      break;

    case State_DispatchToOwningThread:
      CallCallback();
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  return NS_OK;
}

NS_IMETHODIMP
UnlockDirectoryRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mDirectoryLock);

  mDirectoryLock = nullptr;

  return NS_OK;
}





Database::Database(Factory* aFactory,
                   const PrincipalInfo& aPrincipalInfo,
                   const OptionalContentId& aOptionalContentParentId,
                   const nsACString& aGroup,
                   const nsACString& aOrigin,
                   uint32_t aTelemetryId,
                   FullDatabaseMetadata* aMetadata,
                   FileManager* aFileManager,
                   already_AddRefed<DirectoryLock> aDirectoryLock,
                   bool aChromeWriteAccessAllowed)
  : mFactory(aFactory)
  , mMetadata(aMetadata)
  , mFileManager(aFileManager)
  , mDirectoryLock(Move(aDirectoryLock))
  , mPrincipalInfo(aPrincipalInfo)
  , mOptionalContentParentId(aOptionalContentParentId)
  , mGroup(aGroup)
  , mOrigin(aOrigin)
  , mId(aMetadata->mDatabaseId)
  , mFilePath(aMetadata->mFilePath)
  , mFileHandleCount(0)
  , mTelemetryId(aTelemetryId)
  , mPersistenceType(aMetadata->mCommonMetadata.persistenceType())
  , mChromeWriteAccessAllowed(aChromeWriteAccessAllowed)
  , mClosed(false)
  , mInvalidated(false)
  , mActorWasAlive(false)
  , mActorDestroyed(false)
  , mMetadataCleanedUp(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFactory);
  MOZ_ASSERT(aMetadata);
  MOZ_ASSERT(aFileManager);
  MOZ_ASSERT_IF(aChromeWriteAccessAllowed,
                aPrincipalInfo.type() == PrincipalInfo::TSystemPrincipalInfo);
}

void
Database::Invalidate()
{
  AssertIsOnBackgroundThread();

  class MOZ_STACK_CLASS Helper final
  {
  public:
    static bool
    InvalidateTransactions(nsTHashtable<nsPtrHashKey<TransactionBase>>& aTable)
    {
      AssertIsOnBackgroundThread();

      const uint32_t count = aTable.Count();
      if (!count) {
        return true;
      }

      FallibleTArray<nsRefPtr<TransactionBase>> transactions;
      if (NS_WARN_IF(!transactions.SetCapacity(count, fallible))) {
        return false;
      }

      for (auto iter = aTable.Iter(); !iter.Done(); iter.Next()) {
        if (NS_WARN_IF(!transactions.AppendElement(iter.Get()->GetKey(),
                                                   fallible))) {
          return false;
        }
      }

      if (count) {
        IDB_REPORT_INTERNAL_ERR();

        for (uint32_t index = 0; index < count; index++) {
          nsRefPtr<TransactionBase> transaction = transactions[index].forget();
          MOZ_ASSERT(transaction);

          transaction->Invalidate();
        }
      }

      return true;
    }
  };

  if (mInvalidated) {
    return;
  }

  mInvalidated = true;

  if (mActorWasAlive && !mActorDestroyed) {
    unused << SendInvalidate();
  }

  if (!Helper::InvalidateTransactions(mTransactions)) {
    NS_WARNING("Failed to abort all transactions!");
  }

  MOZ_ALWAYS_TRUE(CloseInternal());

  CleanupMetadata();
}

nsresult
Database::EnsureConnection()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());

  PROFILER_LABEL("IndexedDB",
                 "Database::EnsureConnection",
                 js::ProfileEntry::Category::STORAGE);

  if (!mConnection || !mConnection->GetStorageConnection()) {
    nsresult rv =
      gConnectionPool->GetOrCreateConnection(this, getter_AddRefs(mConnection));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  AssertIsOnConnectionThread();

  return NS_OK;
}

bool
Database::RegisterTransaction(TransactionBase* aTransaction)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(!mTransactions.GetEntry(aTransaction));
  MOZ_ASSERT(mDirectoryLock);

  if (NS_WARN_IF(!mTransactions.PutEntry(aTransaction, fallible))) {
    return false;
  }

  return true;
}

void
Database::UnregisterTransaction(TransactionBase* aTransaction)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(mTransactions.GetEntry(aTransaction));

  mTransactions.RemoveEntry(aTransaction);

  MaybeCloseConnection();
}

void
Database::SetActorAlive()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorWasAlive);
  MOZ_ASSERT(!mActorDestroyed);

  mActorWasAlive = true;

  
  
  AddRef();
}

bool
Database::CloseInternal()
{
  AssertIsOnBackgroundThread();

  if (mClosed) {
    if (NS_WARN_IF(!IsInvalidated())) {
      
      return false;
    }

    
    return true;
  }

  mClosed = true;

  if (gConnectionPool) {
    gConnectionPool->CloseDatabaseWhenIdle(Id());
  }

  DatabaseActorInfo* info;
  MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(Id(), &info));

  MOZ_ASSERT(info->mLiveDatabases.Contains(this));

  if (info->mWaitingFactoryOp) {
    info->mWaitingFactoryOp->NoteDatabaseClosed(this);
  }

  MaybeCloseConnection();

  return true;
}

void
Database::MaybeCloseConnection()
{
  AssertIsOnBackgroundThread();

  if (!mTransactions.Count() &&
      !mFileHandleCount &&
      IsClosed() &&
      mDirectoryLock) {
    nsCOMPtr<nsIRunnable> callback =
      NS_NewRunnableMethod(this, &Database::ConnectionClosedCallback);

    nsRefPtr<WaitForTransactionsHelper> helper =
      new WaitForTransactionsHelper(Id(), callback);
    helper->WaitForTransactions();
  }
}

void
Database::ConnectionClosedCallback()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mClosed);
  MOZ_ASSERT(!mTransactions.Count());
  MOZ_ASSERT(!mFileHandleCount);

  if (mDirectoryLock) {
    nsRefPtr<UnlockDirectoryRunnable> runnable =
      new UnlockDirectoryRunnable(mDirectoryLock.forget());

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
  }

  CleanupMetadata();
}

void
Database::CleanupMetadata()
{
  AssertIsOnBackgroundThread();

  if (!mMetadataCleanedUp) {
    mMetadataCleanedUp = true;

    DatabaseActorInfo* info;
    MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(Id(), &info));
    MOZ_ALWAYS_TRUE(info->mLiveDatabases.RemoveElement(this));

    if (info->mLiveDatabases.IsEmpty()) {
      MOZ_ASSERT(!info->mWaitingFactoryOp ||
                 !info->mWaitingFactoryOp->HasBlockedDatabases());
      gLiveDatabaseHashtable->Remove(Id());
    }
  }
}

void
Database::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  mActorDestroyed = true;

  if (!IsInvalidated()) {
    Invalidate();
  }
}

PBackgroundIDBDatabaseFileParent*
Database::AllocPBackgroundIDBDatabaseFileParent(PBlobParent* aBlobParent)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aBlobParent);

  nsRefPtr<BlobImpl> blobImpl =
    static_cast<BlobParent*>(aBlobParent)->GetBlobImpl();
  MOZ_ASSERT(blobImpl);

  nsRefPtr<DatabaseFile> actor;

  if (nsRefPtr<FileInfo> fileInfo = blobImpl->GetFileInfo(mFileManager)) {
    
    actor = new DatabaseFile(fileInfo);
  } else {
    
    fileInfo = mFileManager->GetNewFileInfo();
    MOZ_ASSERT(fileInfo);

    actor = new DatabaseFile(blobImpl, fileInfo);
  }

  MOZ_ASSERT(actor);

  return actor.forget().take();
}

bool
Database::DeallocPBackgroundIDBDatabaseFileParent(
                                       PBackgroundIDBDatabaseFileParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  nsRefPtr<DatabaseFile> actor =
    dont_AddRef(static_cast<DatabaseFile*>(aActor));
  return true;
}

PBackgroundIDBTransactionParent*
Database::AllocPBackgroundIDBTransactionParent(
                                    const nsTArray<nsString>& aObjectStoreNames,
                                    const Mode& aMode)
{
  AssertIsOnBackgroundThread();

  
  if (NS_WARN_IF(mClosed)) {
    if (!mInvalidated) {
      ASSERT_UNLESS_FUZZING();
    }
    return nullptr;
  }

  if (NS_WARN_IF(aObjectStoreNames.IsEmpty())) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  if (NS_WARN_IF(aMode != IDBTransaction::READ_ONLY &&
                 aMode != IDBTransaction::READ_WRITE &&
                 aMode != IDBTransaction::READ_WRITE_FLUSH)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  
  
  if (NS_WARN_IF((aMode == IDBTransaction::READ_WRITE ||
                  aMode == IDBTransaction::READ_WRITE_FLUSH) &&
                 mPrincipalInfo.type() == PrincipalInfo::TSystemPrincipalInfo &&
                 !mChromeWriteAccessAllowed)) {
    return nullptr;
  }

  const ObjectStoreTable& objectStores = mMetadata->mObjectStores;
  const uint32_t nameCount = aObjectStoreNames.Length();

  if (NS_WARN_IF(nameCount > objectStores.Count())) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  FallibleTArray<nsRefPtr<FullObjectStoreMetadata>> fallibleObjectStores;
  if (NS_WARN_IF(!fallibleObjectStores.SetCapacity(nameCount, fallible))) {
    return nullptr;
  }

  for (uint32_t nameIndex = 0; nameIndex < nameCount; nameIndex++) {
    const nsString& name = aObjectStoreNames[nameIndex];

    if (nameIndex) {
      
      if (NS_WARN_IF(name <= aObjectStoreNames[nameIndex - 1])) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
    }

    for (auto iter = objectStores.ConstIter(); !iter.Done(); iter.Next()) {
      auto value = iter.Data();
      MOZ_ASSERT(iter.Key());

      if (name == value->mCommonMetadata.name() && !value->mDeleted) {
        if (NS_WARN_IF(!fallibleObjectStores.AppendElement(value, fallible))) {
          return nullptr;
        }
        break;
      }
    }
  }

  nsTArray<nsRefPtr<FullObjectStoreMetadata>> infallibleObjectStores;
  infallibleObjectStores.SwapElements(fallibleObjectStores);

  nsRefPtr<NormalTransaction> transaction =
    new NormalTransaction(this, aMode, infallibleObjectStores);

  MOZ_ASSERT(infallibleObjectStores.IsEmpty());

  return transaction.forget().take();
}

bool
Database::RecvPBackgroundIDBTransactionConstructor(
                                    PBackgroundIDBTransactionParent* aActor,
                                    InfallibleTArray<nsString>&& aObjectStoreNames,
                                    const Mode& aMode)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(!aObjectStoreNames.IsEmpty());
  MOZ_ASSERT(aMode == IDBTransaction::READ_ONLY ||
             aMode == IDBTransaction::READ_WRITE ||
             aMode == IDBTransaction::READ_WRITE_FLUSH);
  MOZ_ASSERT(!mClosed);

  if (IsInvalidated()) {
    
    
    return true;
  }

  if (!gConnectionPool) {
    gConnectionPool = new ConnectionPool();
  }

  auto* transaction = static_cast<NormalTransaction*>(aActor);

  nsRefPtr<StartTransactionOp> startOp = new StartTransactionOp(transaction);

  uint64_t transactionId =
    gConnectionPool->Start(GetLoggingInfo()->Id(),
                           mMetadata->mDatabaseId,
                           transaction->LoggingSerialNumber(),
                           aObjectStoreNames,
                           aMode != IDBTransaction::READ_ONLY,
                           startOp);

  transaction->SetActive(transactionId);

  if (NS_WARN_IF(!RegisterTransaction(transaction))) {
    IDB_REPORT_INTERNAL_ERR();
    transaction->Abort(NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR,  false);
    return true;
  }

  return true;
}

bool
Database::DeallocPBackgroundIDBTransactionParent(
                                        PBackgroundIDBTransactionParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  nsRefPtr<NormalTransaction> transaction =
    dont_AddRef(static_cast<NormalTransaction*>(aActor));
  return true;
}

PBackgroundIDBVersionChangeTransactionParent*
Database::AllocPBackgroundIDBVersionChangeTransactionParent(
                                              const uint64_t& aCurrentVersion,
                                              const uint64_t& aRequestedVersion,
                                              const int64_t& aNextObjectStoreId,
                                              const int64_t& aNextIndexId)
{
  MOZ_CRASH("PBackgroundIDBVersionChangeTransactionParent actors should be "
            "constructed manually!");
}

bool
Database::DeallocPBackgroundIDBVersionChangeTransactionParent(
                           PBackgroundIDBVersionChangeTransactionParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  nsRefPtr<VersionChangeTransaction> transaction =
    dont_AddRef(static_cast<VersionChangeTransaction*>(aActor));
  return true;
}

bool
Database::RecvDeleteMe()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  return PBackgroundIDBDatabaseParent::Send__delete__(this);
}

bool
Database::RecvBlocked()
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(mClosed)) {
    return false;
  }

  DatabaseActorInfo* info;
  MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(Id(), &info));

  MOZ_ASSERT(info->mLiveDatabases.Contains(this));
  MOZ_ASSERT(info->mWaitingFactoryOp);

  info->mWaitingFactoryOp->NoteDatabaseBlocked(this);

  return true;
}

bool
Database::RecvClose()
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!CloseInternal())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  return true;
}

bool
Database::RecvNewFileHandle()
{
  AssertIsOnBackgroundThread();

  if (!mDirectoryLock) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (mFileHandleCount == UINT32_MAX) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  ++mFileHandleCount;

  return true;
}

bool
Database::RecvFileHandleFinished()
{
  AssertIsOnBackgroundThread();

  if (mFileHandleCount == 0) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  --mFileHandleCount;

  MaybeCloseConnection();

  return true;
}

void
Database::
StartTransactionOp::RunOnConnectionThread()
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(Transaction());
  MOZ_ASSERT(NS_SUCCEEDED(mResultCode));

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld]: "
                 "Beginning database work",
               "IndexedDB %s: P T[%lld]: DB Start",
               IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
               mLoggingSerialNumber);

  TransactionDatabaseOperationBase::RunOnConnectionThread();
}

nsresult
Database::
StartTransactionOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  Transaction()->SetActiveOnConnectionThread();

  if (Transaction()->GetMode() != IDBTransaction::READ_ONLY) {
    nsresult rv = aConnection->BeginWriteTransaction();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

nsresult
Database::
StartTransactionOp::SendSuccessResult()
{
  
  return NS_OK;
}

bool
Database::
StartTransactionOp::SendFailureResult(nsresult )
{
  IDB_REPORT_INTERNAL_ERR();

  
  return false;
}

void
Database::
StartTransactionOp::Cleanup()
{
#ifdef DEBUG
  
  
  NoteActorDestroyed();
#endif

  TransactionDatabaseOperationBase::Cleanup();
}





TransactionBase::TransactionBase(Database* aDatabase, Mode aMode)
  : mDatabase(aDatabase)
  , mTransactionId(0)
  , mDatabaseId(aDatabase->Id())
  , mLoggingSerialNumber(aDatabase->GetLoggingInfo()->NextTransactionSN(aMode))
  , mActiveRequestCount(0)
  , mInvalidatedOnAnyThread(false)
  , mMode(aMode)
  , mHasBeenActive(false)
  , mHasBeenActiveOnConnectionThread(false)
  , mActorDestroyed(false)
  , mInvalidated(false)
  , mResultCode(NS_OK)
  , mCommitOrAbortReceived(false)
  , mCommittedOrAborted(false)
  , mForceAborted(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aDatabase);
  MOZ_ASSERT(mLoggingSerialNumber);
}

TransactionBase::~TransactionBase()
{
  MOZ_ASSERT(!mActiveRequestCount);
  MOZ_ASSERT(mActorDestroyed);
  MOZ_ASSERT_IF(mHasBeenActive, mCommittedOrAborted);
}

void
TransactionBase::Abort(nsresult aResultCode, bool aForce)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(NS_FAILED(aResultCode));

  if (NS_SUCCEEDED(mResultCode)) {
    mResultCode = aResultCode;
  }

  if (aForce) {
    mForceAborted = true;
  }

  MaybeCommitOrAbort();
}

bool
TransactionBase::RecvCommit()
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  mCommitOrAbortReceived = true;

  MaybeCommitOrAbort();
  return true;
}

bool
TransactionBase::RecvAbort(nsresult aResultCode)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(NS_SUCCEEDED(aResultCode))) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(NS_ERROR_GET_MODULE(aResultCode) !=
                 NS_ERROR_MODULE_DOM_INDEXEDDB)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  mCommitOrAbortReceived = true;

  Abort(aResultCode,  false);
  return true;
}

void
TransactionBase::CommitOrAbort()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mCommittedOrAborted);

  mCommittedOrAborted = true;

  if (!mHasBeenActive) {
    return;
  }

  nsRefPtr<CommitOp> commitOp =
    new CommitOp(this, ClampResultCode(mResultCode));

  gConnectionPool->Finish(TransactionId(), commitOp);
}

already_AddRefed<FullObjectStoreMetadata>
TransactionBase::GetMetadataForObjectStoreId(int64_t aObjectStoreId) const
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aObjectStoreId);

  if (!aObjectStoreId) {
    return nullptr;
  }

  nsRefPtr<FullObjectStoreMetadata> metadata;
  if (!mDatabase->Metadata()->mObjectStores.Get(aObjectStoreId,
                                                getter_AddRefs(metadata)) ||
      metadata->mDeleted) {
    return nullptr;
  }

  MOZ_ASSERT(metadata->mCommonMetadata.id() == aObjectStoreId);

  return metadata.forget();
}

already_AddRefed<FullIndexMetadata>
TransactionBase::GetMetadataForIndexId(
                            FullObjectStoreMetadata* const aObjectStoreMetadata,
                            int64_t aIndexId) const
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aIndexId);

  if (!aIndexId) {
    return nullptr;
  }

  nsRefPtr<FullIndexMetadata> metadata;
  if (!aObjectStoreMetadata->mIndexes.Get(aIndexId, getter_AddRefs(metadata)) ||
      metadata->mDeleted) {
    return nullptr;
  }

  MOZ_ASSERT(metadata->mCommonMetadata.id() == aIndexId);

  return metadata.forget();
}

void
TransactionBase::NoteModifiedAutoIncrementObjectStore(
                                             FullObjectStoreMetadata* aMetadata)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aMetadata);

  if (!mModifiedAutoIncrementObjectStoreMetadataArray.Contains(aMetadata)) {
    mModifiedAutoIncrementObjectStoreMetadataArray.AppendElement(aMetadata);
  }
}

void
TransactionBase::ForgetModifiedAutoIncrementObjectStore(
                                             FullObjectStoreMetadata* aMetadata)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aMetadata);

  mModifiedAutoIncrementObjectStoreMetadataArray.RemoveElement(aMetadata);
}

bool
TransactionBase::VerifyRequestParams(const RequestParams& aParams) const
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  switch (aParams.type()) {
    case RequestParams::TObjectStoreAddParams: {
      const ObjectStoreAddPutParams& params =
        aParams.get_ObjectStoreAddParams().commonParams();
      if (NS_WARN_IF(!VerifyRequestParams(params))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStorePutParams: {
      const ObjectStoreAddPutParams& params =
        aParams.get_ObjectStorePutParams().commonParams();
      if (NS_WARN_IF(!VerifyRequestParams(params))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreGetParams: {
      const ObjectStoreGetParams& params = aParams.get_ObjectStoreGetParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.keyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreGetAllParams: {
      const ObjectStoreGetAllParams& params =
        aParams.get_ObjectStoreGetAllParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreGetAllKeysParams: {
      const ObjectStoreGetAllKeysParams& params =
        aParams.get_ObjectStoreGetAllKeysParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreDeleteParams: {
      if (NS_WARN_IF(mMode != IDBTransaction::READ_WRITE &&
                     mMode != IDBTransaction::READ_WRITE_FLUSH &&
                     mMode != IDBTransaction::VERSION_CHANGE)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      const ObjectStoreDeleteParams& params =
        aParams.get_ObjectStoreDeleteParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.keyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreClearParams: {
      if (NS_WARN_IF(mMode != IDBTransaction::READ_WRITE &&
                     mMode != IDBTransaction::READ_WRITE_FLUSH &&
                     mMode != IDBTransaction::VERSION_CHANGE)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      const ObjectStoreClearParams& params =
        aParams.get_ObjectStoreClearParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TObjectStoreCountParams: {
      const ObjectStoreCountParams& params =
        aParams.get_ObjectStoreCountParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }


    case RequestParams::TIndexGetParams: {
      const IndexGetParams& params = aParams.get_IndexGetParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      const nsRefPtr<FullIndexMetadata> indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.keyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TIndexGetKeyParams: {
      const IndexGetKeyParams& params = aParams.get_IndexGetKeyParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      const nsRefPtr<FullIndexMetadata> indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.keyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TIndexGetAllParams: {
      const IndexGetAllParams& params = aParams.get_IndexGetAllParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      const nsRefPtr<FullIndexMetadata> indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TIndexGetAllKeysParams: {
      const IndexGetAllKeysParams& params = aParams.get_IndexGetAllKeysParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      const nsRefPtr<FullIndexMetadata> indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    case RequestParams::TIndexCountParams: {
      const IndexCountParams& params = aParams.get_IndexCountParams();
      const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
        GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      const nsRefPtr<FullIndexMetadata> indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      if (NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  return true;
}

bool
TransactionBase::VerifyRequestParams(const SerializedKeyRange& aParams) const
{
  AssertIsOnBackgroundThread();

  

  if (aParams.isOnly()) {
    if (NS_WARN_IF(aParams.lower().IsUnset())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }
    if (NS_WARN_IF(!aParams.upper().IsUnset())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }
    if (NS_WARN_IF(aParams.lowerOpen())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }
    if (NS_WARN_IF(aParams.upperOpen())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }
  } else if (NS_WARN_IF(aParams.lower().IsUnset() &&
                        aParams.upper().IsUnset())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  return true;
}

bool
TransactionBase::VerifyRequestParams(const ObjectStoreAddPutParams& aParams)
                                     const
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(mMode != IDBTransaction::READ_WRITE &&
                 mMode != IDBTransaction::READ_WRITE_FLUSH &&
                 mMode != IDBTransaction::VERSION_CHANGE)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullObjectStoreMetadata> objMetadata =
    GetMetadataForObjectStoreId(aParams.objectStoreId());
  if (NS_WARN_IF(!objMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(aParams.cloneInfo().data().IsEmpty())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (objMetadata->mCommonMetadata.autoIncrement() &&
      objMetadata->mCommonMetadata.keyPath().IsValid() &&
      aParams.key().IsUnset()) {
    const SerializedStructuredCloneWriteInfo cloneInfo = aParams.cloneInfo();

    if (NS_WARN_IF(!cloneInfo.offsetToKeyProp())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }

    if (NS_WARN_IF(cloneInfo.data().Length() < sizeof(uint64_t))) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }

    if (NS_WARN_IF(cloneInfo.offsetToKeyProp() >
                   (cloneInfo.data().Length() - sizeof(uint64_t)))) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }
  } else if (NS_WARN_IF(aParams.cloneInfo().offsetToKeyProp())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const nsTArray<IndexUpdateInfo>& updates = aParams.indexUpdateInfos();

  for (uint32_t index = 0; index < updates.Length(); index++) {
    nsRefPtr<FullIndexMetadata> indexMetadata =
      GetMetadataForIndexId(objMetadata, updates[index].indexId());
    if (NS_WARN_IF(!indexMetadata)) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }

    if (NS_WARN_IF(updates[index].value().IsUnset())) {
      ASSERT_UNLESS_FUZZING();
      return false;
    }

    MOZ_ASSERT(!updates[index].value().GetBuffer().IsEmpty());
  }

  const nsTArray<DatabaseFileOrMutableFileId>& files = aParams.files();

  for (uint32_t index = 0; index < files.Length(); index++) {
    const DatabaseFileOrMutableFileId& fileOrFileId = files[index];

    MOZ_ASSERT(fileOrFileId.type() != DatabaseFileOrMutableFileId::T__None);

    switch (fileOrFileId.type()) {
      case DatabaseFileOrMutableFileId::TPBackgroundIDBDatabaseFileChild:
        ASSERT_UNLESS_FUZZING();
        return false;

      case DatabaseFileOrMutableFileId::TPBackgroundIDBDatabaseFileParent:
        if (NS_WARN_IF(!fileOrFileId.get_PBackgroundIDBDatabaseFileParent())) {
          ASSERT_UNLESS_FUZZING();
          return false;
        }
        break;

      case DatabaseFileOrMutableFileId::Tint64_t:
        if (NS_WARN_IF(fileOrFileId.get_int64_t() <= 0)) {
          ASSERT_UNLESS_FUZZING();
          return false;
        }
        break;

      default:
        MOZ_CRASH("Should never get here!");
    }
  }

  return true;
}

bool
TransactionBase::VerifyRequestParams(const OptionalKeyRange& aParams) const
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != OptionalKeyRange::T__None);

  switch (aParams.type()) {
    case OptionalKeyRange::TSerializedKeyRange:
      if (NS_WARN_IF(!VerifyRequestParams(aParams.get_SerializedKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;

    case OptionalKeyRange::Tvoid_t:
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  return true;
}

void
TransactionBase::NoteActiveRequest()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mActiveRequestCount < UINT64_MAX);

  mActiveRequestCount++;
}

void
TransactionBase::NoteFinishedRequest()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mActiveRequestCount);

  mActiveRequestCount--;

  MaybeCommitOrAbort();
}

void
TransactionBase::Invalidate()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mInvalidated == mInvalidatedOnAnyThread);

  if (!mInvalidated) {
    mInvalidated = true;
    mInvalidatedOnAnyThread = true;

    Abort(NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR,  true);
  }
}

PBackgroundIDBRequestParent*
TransactionBase::AllocRequest(const RequestParams& aParams, bool aTrustParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

#ifdef DEBUG
  
  aTrustParams = false;
#endif

  if (!aTrustParams && NS_WARN_IF(!VerifyRequestParams(aParams))) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  nsRefPtr<NormalTransactionOp> actor;

  switch (aParams.type()) {
    case RequestParams::TObjectStoreAddParams:
    case RequestParams::TObjectStorePutParams:
      actor = new ObjectStoreAddOrPutRequestOp(this, aParams);
      break;

    case RequestParams::TObjectStoreGetParams:
      actor =
        new ObjectStoreGetRequestOp(this, aParams,  false);
      break;

    case RequestParams::TObjectStoreGetAllParams:
      actor =
        new ObjectStoreGetRequestOp(this, aParams,  true);
      break;

    case RequestParams::TObjectStoreGetAllKeysParams:
      actor =
        new ObjectStoreGetAllKeysRequestOp(this,
                                     aParams.get_ObjectStoreGetAllKeysParams());
      break;

    case RequestParams::TObjectStoreDeleteParams:
      actor =
        new ObjectStoreDeleteRequestOp(this,
                                       aParams.get_ObjectStoreDeleteParams());
      break;

    case RequestParams::TObjectStoreClearParams:
      actor =
        new ObjectStoreClearRequestOp(this,
                                      aParams.get_ObjectStoreClearParams());
      break;

    case RequestParams::TObjectStoreCountParams:
      actor =
        new ObjectStoreCountRequestOp(this,
                                      aParams.get_ObjectStoreCountParams());
      break;

    case RequestParams::TIndexGetParams:
      actor = new IndexGetRequestOp(this, aParams,  false);
      break;

    case RequestParams::TIndexGetKeyParams:
      actor = new IndexGetKeyRequestOp(this, aParams,  false);
      break;

    case RequestParams::TIndexGetAllParams:
      actor = new IndexGetRequestOp(this, aParams,  true);
      break;

    case RequestParams::TIndexGetAllKeysParams:
      actor = new IndexGetKeyRequestOp(this, aParams,  true);
      break;

    case RequestParams::TIndexCountParams:
      actor = new IndexCountRequestOp(this, aParams);
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  MOZ_ASSERT(actor);

  
  return actor.forget().take();
}

bool
TransactionBase::StartRequest(PBackgroundIDBRequestParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  auto* op = static_cast<NormalTransactionOp*>(aActor);

  if (NS_WARN_IF(!op->Init(this))) {
    op->Cleanup();
    return false;
  }

  op->DispatchToConnectionPool();
  return true;
}

bool
TransactionBase::DeallocRequest(PBackgroundIDBRequestParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  
  nsRefPtr<NormalTransactionOp> actor =
    dont_AddRef(static_cast<NormalTransactionOp*>(aActor));
  return true;
}

PBackgroundIDBCursorParent*
TransactionBase::AllocCursor(const OpenCursorParams& aParams, bool aTrustParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != OpenCursorParams::T__None);

#ifdef DEBUG
  
  aTrustParams = false;
#endif

  OpenCursorParams::Type type = aParams.type();
  nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata;
  nsRefPtr<FullIndexMetadata> indexMetadata;
  Cursor::Direction direction;

  switch (type) {
    case OpenCursorParams::TObjectStoreOpenCursorParams: {
      const ObjectStoreOpenCursorParams& params =
        aParams.get_ObjectStoreOpenCursorParams();
      objectStoreMetadata = GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      if (aTrustParams &&
          NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      direction = params.direction();
      break;
    }

    case OpenCursorParams::TObjectStoreOpenKeyCursorParams: {
      const ObjectStoreOpenKeyCursorParams& params =
        aParams.get_ObjectStoreOpenKeyCursorParams();
      objectStoreMetadata = GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      if (aTrustParams &&
          NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      direction = params.direction();
      break;
    }

    case OpenCursorParams::TIndexOpenCursorParams: {
      const IndexOpenCursorParams& params = aParams.get_IndexOpenCursorParams();
      objectStoreMetadata = GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      if (aTrustParams &&
          NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      direction = params.direction();
      break;
    }

    case OpenCursorParams::TIndexOpenKeyCursorParams: {
      const IndexOpenKeyCursorParams& params =
        aParams.get_IndexOpenKeyCursorParams();
      objectStoreMetadata = GetMetadataForObjectStoreId(params.objectStoreId());
      if (NS_WARN_IF(!objectStoreMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      indexMetadata =
        GetMetadataForIndexId(objectStoreMetadata, params.indexId());
      if (NS_WARN_IF(!indexMetadata)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      if (aTrustParams &&
          NS_WARN_IF(!VerifyRequestParams(params.optionalKeyRange()))) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }
      direction = params.direction();
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  nsRefPtr<Cursor> actor =
    new Cursor(this, type, objectStoreMetadata, indexMetadata, direction);

  
  return actor.forget().take();
}

bool
TransactionBase::StartCursor(PBackgroundIDBCursorParent* aActor,
                             const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != OpenCursorParams::T__None);

  auto* op = static_cast<Cursor*>(aActor);

  if (NS_WARN_IF(!op->Start(aParams))) {
    return false;
  }

  return true;
}

bool
TransactionBase::DeallocCursor(PBackgroundIDBCursorParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  
  nsRefPtr<Cursor> actor = dont_AddRef(static_cast<Cursor*>(aActor));
  return true;
}





NormalTransaction::NormalTransaction(
                     Database* aDatabase,
                     TransactionBase::Mode aMode,
                     nsTArray<nsRefPtr<FullObjectStoreMetadata>>& aObjectStores)
  : TransactionBase(aDatabase, aMode)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!aObjectStores.IsEmpty());

  mObjectStores.SwapElements(aObjectStores);
}

bool
NormalTransaction::IsSameProcessActor()
{
  AssertIsOnBackgroundThread();

  PBackgroundParent* actor = Manager()->Manager()->Manager();
  MOZ_ASSERT(actor);

  return !BackgroundParent::IsOtherProcessActor(actor);
}

void
NormalTransaction::SendCompleteNotification(nsresult aResult)
{
  AssertIsOnBackgroundThread();

  if (!IsActorDestroyed()) {
    unused << SendComplete(aResult);
  }
}

void
NormalTransaction::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();

  NoteActorDestroyed();

  if (!mCommittedOrAborted) {
    if (NS_SUCCEEDED(mResultCode)) {
      IDB_REPORT_INTERNAL_ERR();
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    mForceAborted = true;

    MaybeCommitOrAbort();
  }
}

bool
NormalTransaction::RecvDeleteMe()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!IsActorDestroyed());

  return PBackgroundIDBTransactionParent::Send__delete__(this);
}

bool
NormalTransaction::RecvCommit()
{
  AssertIsOnBackgroundThread();

  return TransactionBase::RecvCommit();
}

bool
NormalTransaction::RecvAbort(const nsresult& aResultCode)
{
  AssertIsOnBackgroundThread();

  return TransactionBase::RecvAbort(aResultCode);
}

PBackgroundIDBRequestParent*
NormalTransaction::AllocPBackgroundIDBRequestParent(
                                                   const RequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  return AllocRequest(aParams, IsSameProcessActor());
}

bool
NormalTransaction::RecvPBackgroundIDBRequestConstructor(
                                            PBackgroundIDBRequestParent* aActor,
                                            const RequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  return StartRequest(aActor);
}

bool
NormalTransaction::DeallocPBackgroundIDBRequestParent(
                                            PBackgroundIDBRequestParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  return DeallocRequest(aActor);
}

PBackgroundIDBCursorParent*
NormalTransaction::AllocPBackgroundIDBCursorParent(
                                                const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();

  return AllocCursor(aParams, IsSameProcessActor());
}

bool
NormalTransaction::RecvPBackgroundIDBCursorConstructor(
                                             PBackgroundIDBCursorParent* aActor,
                                             const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != OpenCursorParams::T__None);

  return StartCursor(aActor, aParams);
}

bool
NormalTransaction::DeallocPBackgroundIDBCursorParent(
                                             PBackgroundIDBCursorParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  return DeallocCursor(aActor);
}





VersionChangeTransaction::VersionChangeTransaction(
                                                OpenDatabaseOp* aOpenDatabaseOp)
  : TransactionBase(aOpenDatabaseOp->mDatabase,
                    IDBTransaction::VERSION_CHANGE)
  , mOpenDatabaseOp(aOpenDatabaseOp)
  , mActorWasAlive(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aOpenDatabaseOp);
}

VersionChangeTransaction::~VersionChangeTransaction()
{
#ifdef DEBUG
  
  
  FakeActorDestroyed();
#endif
}

bool
VersionChangeTransaction::IsSameProcessActor()
{
  AssertIsOnBackgroundThread();

  PBackgroundParent* actor = Manager()->Manager()->Manager();
  MOZ_ASSERT(actor);

  return !BackgroundParent::IsOtherProcessActor(actor);
}

void
VersionChangeTransaction::SetActorAlive()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorWasAlive);
  MOZ_ASSERT(!IsActorDestroyed());

  mActorWasAlive = true;

  
  
  AddRef();
}

bool
VersionChangeTransaction::CopyDatabaseMetadata()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mOldMetadata);

  const nsRefPtr<FullDatabaseMetadata> origMetadata =
    GetDatabase()->Metadata();
  MOZ_ASSERT(origMetadata);

  nsRefPtr<FullDatabaseMetadata> newMetadata = origMetadata->Duplicate();
  if (NS_WARN_IF(!newMetadata)) {
    return false;
  }

  
  DatabaseActorInfo* info;
  MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(origMetadata->mDatabaseId,
                                              &info));
  MOZ_ASSERT(!info->mLiveDatabases.IsEmpty());
  MOZ_ASSERT(info->mMetadata == origMetadata);

  mOldMetadata = info->mMetadata.forget();
  info->mMetadata.swap(newMetadata);

  
  for (uint32_t count = info->mLiveDatabases.Length(), index = 0;
       index < count;
       index++) {
    info->mLiveDatabases[index]->mMetadata = info->mMetadata;
  }

  return true;
}

void
VersionChangeTransaction::UpdateMetadata(nsresult aResult)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(GetDatabase());
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT(!!mActorWasAlive == !!mOpenDatabaseOp->mDatabase);
  MOZ_ASSERT_IF(mActorWasAlive, !mOpenDatabaseOp->mDatabaseId.IsEmpty());

  class MOZ_STACK_CLASS Helper final
  {
  public:
    static PLDHashOperator
    Enumerate(const uint64_t& aKey,
              nsRefPtr<FullObjectStoreMetadata>& aValue,
              void* )
    {
      MOZ_ASSERT(aKey);
      MOZ_ASSERT(aValue);

      if (aValue->mDeleted) {
        return PL_DHASH_REMOVE;
      }

      aValue->mIndexes.Enumerate(Enumerate, nullptr);
#ifdef DEBUG
      aValue->mIndexes.MarkImmutable();
#endif

      return PL_DHASH_NEXT;
    }

  private:
    static PLDHashOperator
    Enumerate(const uint64_t& aKey,
              nsRefPtr<FullIndexMetadata>& aValue,
              void* )
    {
      MOZ_ASSERT(aKey);
      MOZ_ASSERT(aValue);

      if (aValue->mDeleted) {
        return PL_DHASH_REMOVE;
      }

      return PL_DHASH_NEXT;
    }
  };

  if (IsActorDestroyed() || !mActorWasAlive) {
    return;
  }

  nsRefPtr<FullDatabaseMetadata> oldMetadata;
  mOldMetadata.swap(oldMetadata);

  DatabaseActorInfo* info;
  if (!gLiveDatabaseHashtable->Get(oldMetadata->mDatabaseId, &info)) {
    return;
  }

  MOZ_ASSERT(!info->mLiveDatabases.IsEmpty());

  if (NS_SUCCEEDED(aResult)) {
    
    info->mMetadata->mObjectStores.Enumerate(Helper::Enumerate, nullptr);
#ifdef DEBUG
    info->mMetadata->mObjectStores.MarkImmutable();
#endif
  } else {
    
    info->mMetadata = oldMetadata.forget();

    for (uint32_t count = info->mLiveDatabases.Length(), index = 0;
         index < count;
         index++) {
      info->mLiveDatabases[index]->mMetadata = info->mMetadata;
    }
  }
}

void
VersionChangeTransaction::SendCompleteNotification(nsresult aResult)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT_IF(!mActorWasAlive, NS_FAILED(mOpenDatabaseOp->mResultCode));
  MOZ_ASSERT_IF(!mActorWasAlive,
                mOpenDatabaseOp->mState > OpenDatabaseOp::State_SendingResults);

  nsRefPtr<OpenDatabaseOp> openDatabaseOp;
  mOpenDatabaseOp.swap(openDatabaseOp);

  if (!mActorWasAlive) {
    return;
  }

  if (NS_FAILED(aResult) && NS_SUCCEEDED(openDatabaseOp->mResultCode)) {
    openDatabaseOp->mResultCode = aResult;
  }

  openDatabaseOp->mState = OpenDatabaseOp::State_SendingResults;

  if (!IsActorDestroyed()) {
    unused << SendComplete(aResult);
  }

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(openDatabaseOp->Run()));
}

void
VersionChangeTransaction::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();

  NoteActorDestroyed();

  if (!mCommittedOrAborted) {
    if (NS_SUCCEEDED(mResultCode)) {
      IDB_REPORT_INTERNAL_ERR();
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    mForceAborted = true;

    MaybeCommitOrAbort();
  }
}

bool
VersionChangeTransaction::RecvDeleteMe()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!IsActorDestroyed());

  return PBackgroundIDBVersionChangeTransactionParent::Send__delete__(this);
}

bool
VersionChangeTransaction::RecvCommit()
{
  AssertIsOnBackgroundThread();

  return TransactionBase::RecvCommit();
}

bool
VersionChangeTransaction::RecvAbort(const nsresult& aResultCode)
{
  AssertIsOnBackgroundThread();

  return TransactionBase::RecvAbort(aResultCode);
}

bool
VersionChangeTransaction::RecvCreateObjectStore(
                                           const ObjectStoreMetadata& aMetadata)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!aMetadata.id())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const nsRefPtr<FullDatabaseMetadata> dbMetadata = GetDatabase()->Metadata();
  MOZ_ASSERT(dbMetadata);

  if (NS_WARN_IF(aMetadata.id() != dbMetadata->mNextObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  auto* foundMetadata =
    MetadataNameOrIdMatcher<FullObjectStoreMetadata>::Match(
      dbMetadata->mObjectStores, aMetadata.id(), aMetadata.name());

  if (NS_WARN_IF(foundMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullObjectStoreMetadata> newMetadata = new FullObjectStoreMetadata();
  newMetadata->mCommonMetadata = aMetadata;
  newMetadata->mNextAutoIncrementId = aMetadata.autoIncrement() ? 1 : 0;
  newMetadata->mComittedAutoIncrementId = newMetadata->mNextAutoIncrementId;

  if (NS_WARN_IF(!dbMetadata->mObjectStores.Put(aMetadata.id(), newMetadata,
                                                fallible))) {
    return false;
  }

  dbMetadata->mNextObjectStoreId++;

  nsRefPtr<CreateObjectStoreOp> op = new CreateObjectStoreOp(this, aMetadata);

  if (NS_WARN_IF(!op->Init(this))) {
    op->Cleanup();
    return false;
  }

  op->DispatchToConnectionPool();

  return true;
}

bool
VersionChangeTransaction::RecvDeleteObjectStore(const int64_t& aObjectStoreId)
{
  AssertIsOnBackgroundThread();

  class MOZ_STACK_CLASS Helper final
  {
    const int64_t mObjectStoreId;
    bool mIsLastObjectStore;
    DebugOnly<bool> mFoundTargetId;

  public:
    static bool
    IsLastObjectStore(const FullDatabaseMetadata* aDatabaseMetadata,
                      const int64_t aObjectStoreId)
    {
      AssertIsOnBackgroundThread();
      MOZ_ASSERT(aDatabaseMetadata);
      MOZ_ASSERT(aObjectStoreId);

      Helper helper(aObjectStoreId);
      aDatabaseMetadata->mObjectStores.EnumerateRead(&Enumerate, &helper);

      MOZ_ASSERT_IF(helper.mIsLastObjectStore, helper.mFoundTargetId);

      return helper.mIsLastObjectStore;
    }

  private:
    explicit
    Helper(const int64_t aObjectStoreId)
      : mObjectStoreId(aObjectStoreId)
      , mIsLastObjectStore(true)
      , mFoundTargetId(false)
    { }

    static PLDHashOperator
    Enumerate(const uint64_t& aKey,
              FullObjectStoreMetadata* aValue,
              void* aClosure)
    {
      auto* helper = static_cast<Helper*>(aClosure);
      MOZ_ASSERT(helper);

      if (uint64_t(helper->mObjectStoreId) == aKey) {
        helper->mFoundTargetId = true;
      } else if(!aValue->mDeleted) {
        helper->mIsLastObjectStore = false;
        return PL_DHASH_STOP;
      }

      return PL_DHASH_NEXT;
    }
  };

  if (NS_WARN_IF(!aObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const nsRefPtr<FullDatabaseMetadata> dbMetadata = GetDatabase()->Metadata();
  MOZ_ASSERT(dbMetadata);
  MOZ_ASSERT(dbMetadata->mNextObjectStoreId > 0);

  if (NS_WARN_IF(aObjectStoreId >= dbMetadata->mNextObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullObjectStoreMetadata> foundMetadata =
    GetMetadataForObjectStoreId(aObjectStoreId);

  if (NS_WARN_IF(!foundMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  foundMetadata->mDeleted = true;

  nsRefPtr<DeleteObjectStoreOp> op =
    new DeleteObjectStoreOp(this,
                            foundMetadata,
                            Helper::IsLastObjectStore(dbMetadata,
                                                      aObjectStoreId));

  if (NS_WARN_IF(!op->Init(this))) {
    op->Cleanup();
    return false;
  }

  op->DispatchToConnectionPool();

  return true;
}

bool
VersionChangeTransaction::RecvCreateIndex(const int64_t& aObjectStoreId,
                                          const IndexMetadata& aMetadata)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!aObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(!aMetadata.id())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const nsRefPtr<FullDatabaseMetadata> dbMetadata = GetDatabase()->Metadata();
  MOZ_ASSERT(dbMetadata);

  if (NS_WARN_IF(aMetadata.id() != dbMetadata->mNextIndexId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullObjectStoreMetadata> foundObjectStoreMetadata =
    GetMetadataForObjectStoreId(aObjectStoreId);

  if (NS_WARN_IF(!foundObjectStoreMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullIndexMetadata> foundIndexMetadata =
    MetadataNameOrIdMatcher<FullIndexMetadata>::Match(
      foundObjectStoreMetadata->mIndexes, aMetadata.id(), aMetadata.name());

  if (NS_WARN_IF(foundIndexMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullIndexMetadata> newMetadata = new FullIndexMetadata();
  newMetadata->mCommonMetadata = aMetadata;

  if (NS_WARN_IF(!foundObjectStoreMetadata->mIndexes.Put(aMetadata.id(),
                                                         newMetadata,
                                                         fallible))) {
    return false;
  }

  dbMetadata->mNextIndexId++;

  nsRefPtr<CreateIndexOp> op =
    new CreateIndexOp(this, aObjectStoreId, aMetadata);

  if (NS_WARN_IF(!op->Init(this))) {
    op->Cleanup();
    return false;
  }

  op->DispatchToConnectionPool();

  return true;
}

bool
VersionChangeTransaction::RecvDeleteIndex(const int64_t& aObjectStoreId,
                                          const int64_t& aIndexId)
{
  AssertIsOnBackgroundThread();

  class MOZ_STACK_CLASS Helper final
  {
    const int64_t mIndexId;
    bool mIsLastIndex;
    DebugOnly<bool> mFoundTargetId;

  public:
    static bool
    IsLastIndex(const FullObjectStoreMetadata* aObjectStoreMetadata,
                const int64_t aIndexId)
    {
      AssertIsOnBackgroundThread();
      MOZ_ASSERT(aObjectStoreMetadata);
      MOZ_ASSERT(aIndexId);

      Helper helper(aIndexId);
      aObjectStoreMetadata->mIndexes.EnumerateRead(&Enumerate, &helper);

      MOZ_ASSERT_IF(helper.mIsLastIndex, helper.mFoundTargetId);

      return helper.mIsLastIndex;
    }

  private:
    explicit
    Helper(const int64_t aIndexId)
      : mIndexId(aIndexId)
      , mIsLastIndex(true)
      , mFoundTargetId(false)
    { }

    static PLDHashOperator
    Enumerate(const uint64_t& aKey, FullIndexMetadata* aValue, void* aClosure)
    {
      auto* helper = static_cast<Helper*>(aClosure);
      MOZ_ASSERT(helper);

      if (uint64_t(helper->mIndexId) == aKey) {
        helper->mFoundTargetId = true;
      } else if (!aValue->mDeleted) {
        helper->mIsLastIndex = false;
        return PL_DHASH_STOP;
      }

      return PL_DHASH_NEXT;
    }
  };

  if (NS_WARN_IF(!aObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(!aIndexId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const nsRefPtr<FullDatabaseMetadata> dbMetadata = GetDatabase()->Metadata();
  MOZ_ASSERT(dbMetadata);
  MOZ_ASSERT(dbMetadata->mNextObjectStoreId > 0);
  MOZ_ASSERT(dbMetadata->mNextIndexId > 0);

  if (NS_WARN_IF(aObjectStoreId >= dbMetadata->mNextObjectStoreId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(aIndexId >= dbMetadata->mNextIndexId)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullObjectStoreMetadata> foundObjectStoreMetadata =
    GetMetadataForObjectStoreId(aObjectStoreId);

  if (NS_WARN_IF(!foundObjectStoreMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FullIndexMetadata> foundIndexMetadata =
    GetMetadataForIndexId(foundObjectStoreMetadata, aIndexId);

  if (NS_WARN_IF(!foundIndexMetadata)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  foundIndexMetadata->mDeleted = true;

  nsRefPtr<DeleteIndexOp> op =
    new DeleteIndexOp(this,
                      aObjectStoreId,
                      aIndexId,
                      foundIndexMetadata->mCommonMetadata.unique(),
                      Helper::IsLastIndex(foundObjectStoreMetadata, aIndexId));

  if (NS_WARN_IF(!op->Init(this))) {
    op->Cleanup();
    return false;
  }

  op->DispatchToConnectionPool();

  return true;
}

PBackgroundIDBRequestParent*
VersionChangeTransaction::AllocPBackgroundIDBRequestParent(
                                                   const RequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  return AllocRequest(aParams, IsSameProcessActor());
}

bool
VersionChangeTransaction::RecvPBackgroundIDBRequestConstructor(
                                            PBackgroundIDBRequestParent* aActor,
                                            const RequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  return StartRequest(aActor);
}

bool
VersionChangeTransaction::DeallocPBackgroundIDBRequestParent(
                                            PBackgroundIDBRequestParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  return DeallocRequest(aActor);
}

PBackgroundIDBCursorParent*
VersionChangeTransaction::AllocPBackgroundIDBCursorParent(
                                                const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();

  return AllocCursor(aParams, IsSameProcessActor());
}

bool
VersionChangeTransaction::RecvPBackgroundIDBCursorConstructor(
                                             PBackgroundIDBCursorParent* aActor,
                                             const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aParams.type() != OpenCursorParams::T__None);

  return StartCursor(aActor, aParams);
}

bool
VersionChangeTransaction::DeallocPBackgroundIDBCursorParent(
                                             PBackgroundIDBCursorParent* aActor)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aActor);

  return DeallocCursor(aActor);
}





Cursor::Cursor(TransactionBase* aTransaction,
               Type aType,
               FullObjectStoreMetadata* aObjectStoreMetadata,
               FullIndexMetadata* aIndexMetadata,
               Direction aDirection)
  : mTransaction(aTransaction)
  , mBackgroundParent(nullptr)
  , mObjectStoreMetadata(aObjectStoreMetadata)
  , mIndexMetadata(aIndexMetadata)
  , mObjectStoreId(aObjectStoreMetadata->mCommonMetadata.id())
  , mIndexId(aIndexMetadata ? aIndexMetadata->mCommonMetadata.id() : 0)
  , mCurrentlyRunningOp(nullptr)
  , mType(aType)
  , mDirection(aDirection)
  , mUniqueIndex(aIndexMetadata ?
                 aIndexMetadata->mCommonMetadata.unique() :
                 false)
  , mIsSameProcessActor(!BackgroundParent::IsOtherProcessActor(
                           aTransaction->GetBackgroundParent()))
  , mActorDestroyed(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(aType != OpenCursorParams::T__None);
  MOZ_ASSERT(aObjectStoreMetadata);
  MOZ_ASSERT_IF(aType == OpenCursorParams::TIndexOpenCursorParams ||
                  aType == OpenCursorParams::TIndexOpenKeyCursorParams,
                aIndexMetadata);

  if (mType == OpenCursorParams::TObjectStoreOpenCursorParams ||
      mType == OpenCursorParams::TIndexOpenCursorParams) {
    mFileManager = aTransaction->GetDatabase()->GetFileManager();
    MOZ_ASSERT(mFileManager);

    mBackgroundParent = aTransaction->GetBackgroundParent();
    MOZ_ASSERT(mBackgroundParent);
  }

  static_assert(OpenCursorParams::T__None == 0 &&
                  OpenCursorParams::T__Last == 4,
                "Lots of code here assumes only four types of cursors!");
}

bool
Cursor::VerifyRequestParams(const CursorRequestParams& aParams) const
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != CursorRequestParams::T__None);
  MOZ_ASSERT(mObjectStoreMetadata);
  MOZ_ASSERT_IF(mType == OpenCursorParams::TIndexOpenCursorParams ||
                  mType == OpenCursorParams::TIndexOpenKeyCursorParams,
                mIndexMetadata);

#ifdef DEBUG
  {
    nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
      mTransaction->GetMetadataForObjectStoreId(mObjectStoreId);
    if (objectStoreMetadata) {
      MOZ_ASSERT(objectStoreMetadata == mObjectStoreMetadata);
    } else {
      MOZ_ASSERT(mObjectStoreMetadata->mDeleted);
    }

    if (objectStoreMetadata &&
        (mType == OpenCursorParams::TIndexOpenCursorParams ||
         mType == OpenCursorParams::TIndexOpenKeyCursorParams)) {
      nsRefPtr<FullIndexMetadata> indexMetadata =
        mTransaction->GetMetadataForIndexId(objectStoreMetadata, mIndexId);
      if (indexMetadata) {
        MOZ_ASSERT(indexMetadata == mIndexMetadata);
      } else {
        MOZ_ASSERT(mIndexMetadata->mDeleted);
      }
    }
  }
#endif

  if (NS_WARN_IF(mObjectStoreMetadata->mDeleted) ||
      (mIndexMetadata && NS_WARN_IF(mIndexMetadata->mDeleted))) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  switch (aParams.type()) {
    case CursorRequestParams::TContinueParams: {
      const Key& key = aParams.get_ContinueParams().key();
      if (!key.IsUnset()) {
        switch (mDirection) {
          case IDBCursor::NEXT:
          case IDBCursor::NEXT_UNIQUE:
            if (NS_WARN_IF(key <= mKey)) {
              ASSERT_UNLESS_FUZZING();
              return false;
            }
            break;

          case IDBCursor::PREV:
          case IDBCursor::PREV_UNIQUE:
            if (NS_WARN_IF(key >= mKey)) {
              ASSERT_UNLESS_FUZZING();
              return false;
            }
            break;

          default:
            MOZ_CRASH("Should never get here!");
        }
      }
      break;
    }

    case CursorRequestParams::TAdvanceParams:
      if (NS_WARN_IF(!aParams.get_AdvanceParams().count())) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  return true;
}

bool
Cursor::Start(const OpenCursorParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() == mType);
  MOZ_ASSERT(!mActorDestroyed);

  if (NS_WARN_IF(mCurrentlyRunningOp)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  const OptionalKeyRange& optionalKeyRange =
    mType == OpenCursorParams::TObjectStoreOpenCursorParams ?
      aParams.get_ObjectStoreOpenCursorParams().optionalKeyRange() :
    mType == OpenCursorParams::TObjectStoreOpenKeyCursorParams ?
      aParams.get_ObjectStoreOpenKeyCursorParams().optionalKeyRange() :
    mType == OpenCursorParams::TIndexOpenCursorParams ?
      aParams.get_IndexOpenCursorParams().optionalKeyRange() :
      aParams.get_IndexOpenKeyCursorParams().optionalKeyRange();

  if (mTransaction->IsInvalidated()) {
    return true;
  }

  nsRefPtr<OpenOp> openOp = new OpenOp(this, optionalKeyRange);

  if (NS_WARN_IF(!openOp->Init(mTransaction))) {
    openOp->Cleanup();
    return false;
  }

  openOp->DispatchToConnectionPool();
  mCurrentlyRunningOp = openOp;

  return true;
}

void
Cursor::SendResponseInternal(CursorResponse& aResponse,
                             const nsTArray<StructuredCloneFile>& aFiles)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aResponse.type() != CursorResponse::T__None);
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tnsresult,
                NS_FAILED(aResponse.get_nsresult()));
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tnsresult,
                NS_ERROR_GET_MODULE(aResponse.get_nsresult()) ==
                  NS_ERROR_MODULE_DOM_INDEXEDDB);
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tvoid_t, mKey.IsUnset());
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tvoid_t,
                mRangeKey.IsUnset());
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tvoid_t,
                mObjectKey.IsUnset());
  MOZ_ASSERT_IF(aResponse.type() == CursorResponse::Tnsresult ||
                aResponse.type() == CursorResponse::Tvoid_t ||
                aResponse.type() ==
                  CursorResponse::TObjectStoreKeyCursorResponse ||
                aResponse.type() == CursorResponse::TIndexKeyCursorResponse,
                aFiles.IsEmpty());
  MOZ_ASSERT(!mActorDestroyed);
  MOZ_ASSERT(mCurrentlyRunningOp);

  if (!aFiles.IsEmpty()) {
    MOZ_ASSERT(aResponse.type() == CursorResponse::TObjectStoreCursorResponse ||
               aResponse.type() == CursorResponse::TIndexCursorResponse);
    MOZ_ASSERT(mFileManager);
    MOZ_ASSERT(mBackgroundParent);

    FallibleTArray<PBlobParent*> actors;
    FallibleTArray<intptr_t> fileInfos;
    nsresult rv = ConvertBlobsToActors(mBackgroundParent,
                                       mFileManager,
                                       aFiles,
                                       actors,
                                       fileInfos);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResponse = ClampResultCode(rv);
    } else {
      SerializedStructuredCloneReadInfo* serializedInfo = nullptr;
      switch (aResponse.type()) {
        case CursorResponse::TObjectStoreCursorResponse:
          serializedInfo =
            &aResponse.get_ObjectStoreCursorResponse().cloneInfo();
          break;

        case CursorResponse::TIndexCursorResponse:
          serializedInfo = &aResponse.get_IndexCursorResponse().cloneInfo();
          break;

        default:
          MOZ_CRASH("Should never get here!");
      }

      MOZ_ASSERT(serializedInfo);
      MOZ_ASSERT(serializedInfo->blobsParent().IsEmpty());
      MOZ_ASSERT(serializedInfo->fileInfos().IsEmpty());

      serializedInfo->blobsParent().SwapElements(actors);
      serializedInfo->fileInfos().SwapElements(fileInfos);
    }
  }

  
  auto* base = static_cast<PBackgroundIDBCursorParent*>(this);
  if (!base->SendResponse(aResponse)) {
    NS_WARNING("Failed to send response!");
  }

  mCurrentlyRunningOp = nullptr;
}

void
Cursor::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  mActorDestroyed = true;

  if (mCurrentlyRunningOp) {
    mCurrentlyRunningOp->NoteActorDestroyed();
  }

  mBackgroundParent = nullptr;

  mObjectStoreMetadata = nullptr;
  mIndexMetadata = nullptr;
}

bool
Cursor::RecvDeleteMe()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!mActorDestroyed);

  if (NS_WARN_IF(mCurrentlyRunningOp)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  return PBackgroundIDBCursorParent::Send__delete__(this);
}

bool
Cursor::RecvContinue(const CursorRequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParams.type() != CursorRequestParams::T__None);
  MOZ_ASSERT(!mActorDestroyed);
  MOZ_ASSERT(mObjectStoreMetadata);
  MOZ_ASSERT_IF(mType == OpenCursorParams::TIndexOpenCursorParams ||
                  mType == OpenCursorParams::TIndexOpenKeyCursorParams,
                mIndexMetadata);

  const bool trustParams =
#ifdef DEBUG
  
    false
#else
    mIsSameProcessActor
#endif
    ;

  if (!trustParams && !VerifyRequestParams(aParams)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mCurrentlyRunningOp)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (NS_WARN_IF(mTransaction->mCommitOrAbortReceived)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  if (mTransaction->IsInvalidated()) {
    return true;
  }

  nsRefPtr<ContinueOp> continueOp = new ContinueOp(this, aParams);
  if (NS_WARN_IF(!continueOp->Init(mTransaction))) {
    continueOp->Cleanup();
    return false;
  }

  continueOp->DispatchToConnectionPool();
  mCurrentlyRunningOp = continueOp;

  return true;
}





FileManager::FileManager(PersistenceType aPersistenceType,
                         const nsACString& aGroup,
                         const nsACString& aOrigin,
                         bool aIsApp,
                         const nsAString& aDatabaseName,
                         bool aEnforcingQuota)
  : mPersistenceType(aPersistenceType)
  , mGroup(aGroup)
  , mOrigin(aOrigin)
  , mDatabaseName(aDatabaseName)
  , mLastFileId(0)
  , mIsApp(aIsApp)
  , mEnforcingQuota(aEnforcingQuota)
  , mInvalidated(false)
{ }

FileManager::~FileManager()
{ }

nsresult
FileManager::Init(nsIFile* aDirectory,
                  mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aConnection);

  bool exists;
  nsresult rv = aDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    bool isDirectory;
    rv = aDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!isDirectory)) {
      return NS_ERROR_FAILURE;
    }
  } else {
    rv = aDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = aDirectory->GetPath(mDirectoryPath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIFile> journalDirectory;
  rv = aDirectory->Clone(getter_AddRefs(journalDirectory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = journalDirectory->Append(NS_LITERAL_STRING(JOURNAL_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = journalDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    bool isDirectory;
    rv = journalDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!isDirectory)) {
      return NS_ERROR_FAILURE;
    }
  }

  rv = journalDirectory->GetPath(mJournalDirectoryPath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, refcount "
    "FROM file"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    int64_t id;
    rv = stmt->GetInt64(0, &id);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    int32_t refcount;
    rv = stmt->GetInt32(1, &refcount);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(refcount > 0);

    nsRefPtr<FileInfo> fileInfo = FileInfo::Create(this, id);
    fileInfo->mDBRefCnt = static_cast<nsrefcnt>(refcount);

    mFileInfos.Put(id, fileInfo);

    mLastFileId = std::max(id, mLastFileId);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
FileManager::Invalidate()
{
  class MOZ_STACK_CLASS Helper final
  {
  public:
    static PLDHashOperator
    ClearDBRefs(const uint64_t& aKey, FileInfo*& aValue, void* aUserArg)
    {
      MOZ_ASSERT(aValue);

      if (aValue->LockedClearDBRefs()) {
        return PL_DHASH_NEXT;
      }

      return PL_DHASH_REMOVE;
    }
  };

  if (IndexedDatabaseManager::IsClosed()) {
    MOZ_ASSERT(false, "Shouldn't be called after shutdown!");
    return NS_ERROR_UNEXPECTED;
  }

  MutexAutoLock lock(IndexedDatabaseManager::FileMutex());

  MOZ_ASSERT(!mInvalidated);
  mInvalidated = true;

  mFileInfos.Enumerate(Helper::ClearDBRefs, nullptr);

  return NS_OK;
}

already_AddRefed<nsIFile>
FileManager::GetDirectory()
{
  return GetFileForPath(mDirectoryPath);
}

already_AddRefed<nsIFile>
FileManager::GetJournalDirectory()
{
  return GetFileForPath(mJournalDirectoryPath);
}

already_AddRefed<nsIFile>
FileManager::EnsureJournalDirectory()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  nsCOMPtr<nsIFile> journalDirectory = GetFileForPath(mJournalDirectoryPath);
  if (NS_WARN_IF(!journalDirectory)) {
    return nullptr;
  }

  bool exists;
  nsresult rv = journalDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  if (exists) {
    bool isDirectory;
    rv = journalDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }

    if (NS_WARN_IF(!isDirectory)) {
      return nullptr;
    }
  } else {
    rv = journalDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }
  }

  return journalDirectory.forget();
}

already_AddRefed<FileInfo>
FileManager::GetFileInfo(int64_t aId)
{
  if (IndexedDatabaseManager::IsClosed()) {
    MOZ_ASSERT(false, "Shouldn't be called after shutdown!");
    return nullptr;
  }

  FileInfo* fileInfo;
  {
    MutexAutoLock lock(IndexedDatabaseManager::FileMutex());
    fileInfo = mFileInfos.Get(aId);
  }

  nsRefPtr<FileInfo> result = fileInfo;
  return result.forget();
}

already_AddRefed<FileInfo>
FileManager::GetNewFileInfo()
{
  MOZ_ASSERT(!IndexedDatabaseManager::IsClosed());

  FileInfo* fileInfo;
  {
    MutexAutoLock lock(IndexedDatabaseManager::FileMutex());

    int64_t id = mLastFileId + 1;

    fileInfo = FileInfo::Create(this, id);

    mFileInfos.Put(id, fileInfo);

    mLastFileId = id;
  }

  nsRefPtr<FileInfo> result = fileInfo;
  return result.forget();
}


already_AddRefed<nsIFile>
FileManager::GetFileForId(nsIFile* aDirectory, int64_t aId)
{
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aId > 0);

  nsAutoString id;
  id.AppendInt(aId);

  nsCOMPtr<nsIFile> file;
  nsresult rv = aDirectory->Clone(getter_AddRefs(file));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  rv = file->Append(id);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return file.forget();
}


nsresult
FileManager::InitDirectory(nsIFile* aDirectory,
                           nsIFile* aDatabaseFile,
                           PersistenceType aPersistenceType,
                           const nsACString& aGroup,
                           const nsACString& aOrigin,
                           uint32_t aTelemetryId)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aDatabaseFile);

  bool exists;
  nsresult rv = aDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    return NS_OK;
  }

  bool isDirectory;
  rv = aDirectory->IsDirectory(&isDirectory);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!isDirectory)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIFile> journalDirectory;
  rv = aDirectory->Clone(getter_AddRefs(journalDirectory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = journalDirectory->Append(NS_LITERAL_STRING(JOURNAL_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = journalDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    rv = journalDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!isDirectory)) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = journalDirectory->GetDirectoryEntries(getter_AddRefs(entries));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool hasElements;
    rv = entries->HasMoreElements(&hasElements);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (hasElements) {
      nsCOMPtr<mozIStorageConnection> connection;
      rv = CreateStorageConnection(aDatabaseFile,
                                   aDirectory,
                                   NullString(),
                                   aPersistenceType,
                                   aGroup,
                                   aOrigin,
                                   aTelemetryId,
                                   getter_AddRefs(connection));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mozStorageTransaction transaction(connection, false);

      rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE VIRTUAL TABLE fs USING filesystem;"
      ));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      nsCOMPtr<mozIStorageStatement> stmt;
      rv = connection->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT name, (name IN (SELECT id FROM file)) "
        "FROM fs "
        "WHERE path = :path"
      ), getter_AddRefs(stmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      nsString path;
      rv = journalDirectory->GetPath(path);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("path"), path);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      bool hasResult;
      while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
        nsString name;
        rv = stmt->GetString(0, name);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        int32_t flag = stmt->AsInt32(1);

        if (!flag) {
          nsCOMPtr<nsIFile> file;
          rv = aDirectory->Clone(getter_AddRefs(file));
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          rv = file->Append(name);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          if (NS_FAILED(file->Remove(false))) {
            NS_WARNING("Failed to remove orphaned file!");
          }
        }

        nsCOMPtr<nsIFile> journalFile;
        rv = journalDirectory->Clone(getter_AddRefs(journalFile));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        rv = journalFile->Append(name);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (NS_FAILED(journalFile->Remove(false))) {
          NS_WARNING("Failed to remove journal file!");
        }
      }

      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE fs;"
      ));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = transaction.Commit();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  return NS_OK;
}


nsresult
FileManager::GetUsage(nsIFile* aDirectory, uint64_t* aUsage)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aUsage);

  bool exists;
  nsresult rv = aDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    *aUsage = 0;
    return NS_OK;
  }

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  uint64_t usage = 0;

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    MOZ_ASSERT(file);

    nsString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (leafName.EqualsLiteral(JOURNAL_DIRECTORY_NAME)) {
      continue;
    }

    int64_t fileSize;
    rv = file->GetFileSize(&fileSize);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    quota::IncrementUsage(&usage, uint64_t(fileSize));
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  *aUsage = usage;
  return NS_OK;
}





QuotaClient* QuotaClient::sInstance = nullptr;

QuotaClient::QuotaClient()
  : mMaintenanceStartTime(0)
  , mMaintenanceRunId(0)
  , mShutdownRequested(false)
  , mIdleObserverRegistered(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sInstance, "We expect this to be a singleton!");
  MOZ_ASSERT(!gTelemetryIdMutex);

  
  
  gTelemetryIdMutex = new Mutex("IndexedDB gTelemetryIdMutex");

  sInstance = this;
}

QuotaClient::~QuotaClient()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance == this, "We expect this to be a singleton!");
  MOZ_ASSERT(gTelemetryIdMutex);
  MOZ_ASSERT(!mMaintenanceThreadPool);
  MOZ_ASSERT_IF(mMaintenanceInfoHashtable, !mMaintenanceInfoHashtable->Count());
  MOZ_ASSERT(!mIdleObserverRegistered);

  
  
  gTelemetryIdHashtable = nullptr;
  gTelemetryIdMutex = nullptr;

  sInstance = nullptr;
}

void
QuotaClient::NoteBackgroundThread(nsIEventTarget* aBackgroundThread)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aBackgroundThread);
  MOZ_ASSERT(!mShutdownRequested);

  mBackgroundThread = aBackgroundThread;
}

NS_IMPL_ISUPPORTS(QuotaClient, nsIObserver)

mozilla::dom::quota::Client::Type
QuotaClient::GetType()
{
  return QuotaClient::IDB;
}

struct FileManagerInitInfo
{
  nsCOMPtr<nsIFile> mDirectory;
  nsCOMPtr<nsIFile> mDatabaseFile;
  nsCOMPtr<nsIFile> mDatabaseWALFile;
};

nsresult
QuotaClient::InitOrigin(PersistenceType aPersistenceType,
                        const nsACString& aGroup,
                        const nsACString& aOrigin,
                        UsageInfo* aUsageInfo)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> directory;
  nsresult rv =
    GetDirectory(aPersistenceType, aOrigin, getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  

  nsAutoTArray<nsString, 20> subdirsToProcess;
  nsTArray<nsCOMPtr<nsIFile>> unknownFiles;
  nsTHashtable<nsStringHashKey> validSubdirs(20);
  nsAutoTArray<FileManagerInitInfo, 20> initInfos;

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  const NS_ConvertASCIItoUTF16 filesSuffix(
    kFileManagerDirectoryNameSuffix,
    LiteralStringLength(kFileManagerDirectoryNameSuffix));

  const NS_ConvertASCIItoUTF16 journalSuffix(
    kSQLiteJournalSuffix,
    LiteralStringLength(kSQLiteJournalSuffix));
  const NS_ConvertASCIItoUTF16 shmSuffix(kSQLiteSHMSuffix,
                                         LiteralStringLength(kSQLiteSHMSuffix));
  const NS_ConvertASCIItoUTF16 walSuffix(kSQLiteWALSuffix,
                                         LiteralStringLength(kSQLiteWALSuffix));

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
         hasMore &&
         (!aUsageInfo || !aUsageInfo->Canceled())) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    MOZ_ASSERT(file);

    nsString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (isDirectory) {
      if (!StringEndsWith(leafName, filesSuffix) ||
          !validSubdirs.GetEntry(leafName)) {
        subdirsToProcess.AppendElement(leafName);
      }
      continue;
    }

    
    
    
    if (leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
      continue;
    }

    
    
    
    if (StringEndsWith(leafName, journalSuffix) ||
        StringEndsWith(leafName, shmSuffix)) {
      continue;
    }

    
    
    if (StringEndsWith(leafName, walSuffix)) {
      continue;
    }

    nsDependentSubstring dbBaseFilename;
    if (!GetDatabaseBaseFilename(leafName, dbBaseFilename)) {
      unknownFiles.AppendElement(file);
      continue;
    }

    nsCOMPtr<nsIFile> fmDirectory;
    rv = directory->Clone(getter_AddRefs(fmDirectory));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsString fmDirectoryBaseName = dbBaseFilename + filesSuffix;

    rv = fmDirectory->Append(fmDirectoryBaseName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> walFile;
    if (aUsageInfo) {
      rv = directory->Clone(getter_AddRefs(walFile));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = walFile->Append(dbBaseFilename + walSuffix);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    FileManagerInitInfo* initInfo = initInfos.AppendElement();
    initInfo->mDirectory.swap(fmDirectory);
    initInfo->mDatabaseFile.swap(file);
    initInfo->mDatabaseWALFile.swap(walFile);

    validSubdirs.PutEntry(fmDirectoryBaseName);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (uint32_t count = subdirsToProcess.Length(), i = 0; i < count; i++) {
    const nsString& subdirName = subdirsToProcess[i];

    
    
    if (StringEndsWith(subdirName, filesSuffix)) {
      if (NS_WARN_IF(!validSubdirs.GetEntry(subdirName))) {
        return NS_ERROR_UNEXPECTED;
      }

      continue;
    }

    
    
    nsString subdirNameWithSuffix = subdirName + filesSuffix;
    if (!validSubdirs.GetEntry(subdirNameWithSuffix)) {
      
      
      
      
      subdirNameWithSuffix = subdirName + NS_LITERAL_STRING(".") + filesSuffix;
      if (NS_WARN_IF(!validSubdirs.GetEntry(subdirNameWithSuffix))) {
        return NS_ERROR_UNEXPECTED;
      }
    }

    
    
    
    nsCOMPtr<nsIFile> subdir;
    rv = directory->Clone(getter_AddRefs(subdir));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = subdir->Append(subdirNameWithSuffix);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool exists;
    rv = subdir->Exists(&exists);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (exists) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    rv = directory->Clone(getter_AddRefs(subdir));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = subdir->Append(subdirName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    DebugOnly<bool> isDirectory;
    MOZ_ASSERT(NS_SUCCEEDED(subdir->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory);

    rv = subdir->RenameTo(nullptr, subdirNameWithSuffix);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  for (uint32_t count = initInfos.Length(), i = 0; i < count; i++) {
    FileManagerInitInfo& initInfo = initInfos[i];
    MOZ_ASSERT(initInfo.mDirectory);
    MOZ_ASSERT(initInfo.mDatabaseFile);
    MOZ_ASSERT_IF(aUsageInfo, initInfo.mDatabaseWALFile);

    rv = FileManager::InitDirectory(initInfo.mDirectory,
                                    initInfo.mDatabaseFile,
                                    aPersistenceType,
                                    aGroup,
                                    aOrigin,
                                    TelemetryIdForFile(initInfo.mDatabaseFile));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (aUsageInfo && !aUsageInfo->Canceled()) {
      int64_t fileSize;
      rv = initInfo.mDatabaseFile->GetFileSize(&fileSize);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(fileSize >= 0);

      aUsageInfo->AppendToDatabaseUsage(uint64_t(fileSize));

      rv = initInfo.mDatabaseWALFile->GetFileSize(&fileSize);
      if (NS_SUCCEEDED(rv)) {
        MOZ_ASSERT(fileSize >= 0);
        aUsageInfo->AppendToDatabaseUsage(uint64_t(fileSize));
      } else if (NS_WARN_IF(rv != NS_ERROR_FILE_NOT_FOUND &&
                            rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)) {
        return rv;
      }

      uint64_t usage;
      rv = FileManager::GetUsage(initInfo.mDirectory, &usage);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      aUsageInfo->AppendToFileUsage(usage);
    }
  }

  
  if (!unknownFiles.IsEmpty()) {
#ifdef DEBUG
    for (uint32_t count = unknownFiles.Length(), i = 0; i < count; i++) {
      nsCOMPtr<nsIFile>& unknownFile = unknownFiles[i];

      nsString leafName;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(unknownFile->GetLeafName(leafName)));

      MOZ_ASSERT(!StringEndsWith(leafName, journalSuffix));
      MOZ_ASSERT(!StringEndsWith(leafName, shmSuffix));
      MOZ_ASSERT(!StringEndsWith(leafName, walSuffix));

      nsString path;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(unknownFile->GetPath(path)));
      MOZ_ASSERT(!path.IsEmpty());

      nsPrintfCString warning("Refusing to open databases for \"%s\" because "
                              "an unexpected file exists in the storage "
                              "area: \"%s\"",
                              PromiseFlatCString(aOrigin).get(),
                              NS_ConvertUTF16toUTF8(path).get());
      NS_WARNING(warning.get());
    }
#endif
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

nsresult
QuotaClient::GetUsageForOrigin(PersistenceType aPersistenceType,
                               const nsACString& aGroup,
                               const nsACString& aOrigin,
                               UsageInfo* aUsageInfo)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aUsageInfo);

  nsCOMPtr<nsIFile> directory;
  nsresult rv =
    GetDirectory(aPersistenceType, aOrigin, getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = GetUsageForDirectoryInternal(directory, aUsageInfo, true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
QuotaClient::OnOriginClearCompleted(PersistenceType aPersistenceType,
                                    const nsACString& aOrigin)
{
  AssertIsOnIOThread();

  if (IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get()) {
    mgr->InvalidateFileManagers(aPersistenceType, aOrigin);
  }
}

void
QuotaClient::ReleaseIOThreadObjects()
{
  AssertIsOnIOThread();

  if (IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get()) {
    mgr->InvalidateAllFileManagers();
  }
}

void
QuotaClient::AbortOperations(const nsACString& aOrigin)
{
  if (mBackgroundThread) {
    nsRefPtr<AbortOperationsRunnable> runnable =
      new AbortOperationsRunnable(aOrigin);

    if (NS_FAILED(mBackgroundThread->Dispatch(runnable, NS_DISPATCH_NORMAL))) {
      
      return;
    }
  }
}

void
QuotaClient::AbortOperationsForProcess(ContentParentId aContentParentId)
{
  if (mBackgroundThread) {
    nsRefPtr<AbortOperationsRunnable> runnable =
      new AbortOperationsRunnable(aContentParentId);

    if (NS_FAILED(mBackgroundThread->Dispatch(runnable, NS_DISPATCH_NORMAL))) {
      
      return;
    }
  }
}

void
QuotaClient::PerformIdleMaintenance()
{
  using namespace mozilla::hal;

  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mShutdownRequested);

  
  
  BatteryInformation batteryInfo;

#ifdef MOZ_WIDGET_ANDROID
  
  
  if (!kRunningXPCShellTests)
#endif
  {
    GetCurrentBatteryInformation(&batteryInfo);
  }

  
  
  if (kRunningXPCShellTests) {
    batteryInfo.level() = 100;
    batteryInfo.charging() = true;
  }

  if (NS_WARN_IF(!batteryInfo.charging())) {
    return;
  }

  
  
  IndexedDatabaseManager* mgr = IndexedDatabaseManager::GetOrCreate();
  if (NS_WARN_IF(!mgr)) {
    return;
  }

  if (kRunningXPCShellTests) {
    
    unused << Observe(nullptr, OBSERVER_TOPIC_IDLE, nullptr);
  } else if (!mIdleObserverRegistered) {
    nsCOMPtr<nsIIdleService> idleService =
      do_GetService(kIdleServiceContractId);
    MOZ_ASSERT(idleService);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      idleService->AddIdleObserver(this, kIdleObserverTimeSec)));

    mIdleObserverRegistered = true;
  }
}

void
QuotaClient::ShutdownWorkThreads()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mShutdownRunnable);
  MOZ_ASSERT(!mShutdownRequested);

  StopIdleMaintenance();

  mShutdownRequested = true;

  if (mBackgroundThread) {
    nsRefPtr<ShutdownWorkThreadsRunnable> runnable =
      new ShutdownWorkThreadsRunnable(this);

    if (NS_SUCCEEDED(mBackgroundThread->Dispatch(runnable,
                                                 NS_DISPATCH_NORMAL))) {
      mShutdownRunnable = Move(runnable);
    }
  }

  FileService::Shutdown();

  if (mMaintenanceThreadPool) {
    mMaintenanceThreadPool->Shutdown();
    mMaintenanceThreadPool = nullptr;
  }

  if (mShutdownRunnable) {
    nsIThread* currentThread = NS_GetCurrentThread();
    MOZ_ASSERT(currentThread);

    while (mShutdownRunnable) {
      MOZ_ALWAYS_TRUE(NS_ProcessNextEvent(currentThread));
    }
  }
}

nsresult
QuotaClient::GetDirectory(PersistenceType aPersistenceType,
                          const nsACString& aOrigin, nsIFile** aDirectory)
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsCOMPtr<nsIFile> directory;
  nsresult rv = quotaManager->GetDirectoryForOrigin(aPersistenceType, aOrigin,
                                                    getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(directory);

  rv = directory->Append(NS_LITERAL_STRING(IDB_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  directory.forget(aDirectory);
  return NS_OK;
}

nsresult
QuotaClient::GetUsageForDirectoryInternal(nsIFile* aDirectory,
                                          UsageInfo* aUsageInfo,
                                          bool aDatabaseFiles)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aUsageInfo);

  nsCOMPtr<nsISimpleEnumerator> entries;
  nsresult rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!entries) {
    return NS_OK;
  }

  const NS_ConvertASCIItoUTF16 journalSuffix(
    kSQLiteJournalSuffix,
    LiteralStringLength(kSQLiteJournalSuffix));
  const NS_ConvertASCIItoUTF16 shmSuffix(kSQLiteSHMSuffix,
                                         LiteralStringLength(kSQLiteSHMSuffix));

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
         hasMore &&
         !aUsageInfo->Canceled()) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    MOZ_ASSERT(file);

    nsString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    if (StringEndsWith(leafName, journalSuffix) ||
        StringEndsWith(leafName, shmSuffix)) {
      continue;
    }

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    if (rv == NS_ERROR_FILE_NOT_FOUND ||
        rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
      continue;
    }

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (isDirectory) {
      if (aDatabaseFiles) {
        rv = GetUsageForDirectoryInternal(file, aUsageInfo, false);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      } else {
        nsString leafName;
        rv = file->GetLeafName(leafName);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (!leafName.EqualsLiteral(JOURNAL_DIRECTORY_NAME)) {
          NS_WARNING("Unknown directory found!");
        }
      }

      continue;
    }

    int64_t fileSize;
    rv = file->GetFileSize(&fileSize);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(fileSize >= 0);

    if (aDatabaseFiles) {
      aUsageInfo->AppendToDatabaseUsage(uint64_t(fileSize));
    } else {
      aUsageInfo->AppendToFileUsage(uint64_t(fileSize));
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
QuotaClient::RemoveIdleObserver()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mIdleObserverRegistered) {
    nsCOMPtr<nsIIdleService> idleService =
      do_GetService(kIdleServiceContractId);
    MOZ_ASSERT(idleService);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      idleService->RemoveIdleObserver(this, kIdleObserverTimeSec)));

    mIdleObserverRegistered = false;
  }
}

void
QuotaClient::StartIdleMaintenance()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mShutdownRequested);

  if (!mMaintenanceThreadPool) {
    nsRefPtr<nsThreadPool> threadPool = new nsThreadPool();

    
    
    
    const uint32_t threadCount =
      std::max(int32_t(PR_GetNumberOfProcessors()), int32_t(1)) +
      2;

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      threadPool->SetThreadLimit(threadCount)));

    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      threadPool->SetIdleThreadLimit(1)));

    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      threadPool->SetIdleThreadTimeout(5 * PR_MSEC_PER_SEC)));

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      threadPool->SetName(NS_LITERAL_CSTRING("IndexedDB Mnt"))));

    mMaintenanceThreadPool = Move(threadPool);
  }

  mMaintenanceStartTime = PR_Now();
  MOZ_ASSERT(mMaintenanceStartTime);

  if (!mMaintenanceInfoHashtable) {
    mMaintenanceInfoHashtable = MakeUnique<MaintenanceInfoHashtable>();
  }

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethodWithArg<uint32_t>(
      this,
      &QuotaClient::FindDatabasesForIdleMaintenance,
      mMaintenanceRunId);
  MOZ_ASSERT(runnable);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mMaintenanceThreadPool->Dispatch(runnable, NS_DISPATCH_NORMAL)));
}

void
QuotaClient::StopIdleMaintenance()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mShutdownRequested);

  RemoveIdleObserver();

  mMaintenanceRunId++;
}

void
QuotaClient::FindDatabasesForIdleMaintenance(uint32_t aRunId)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mMaintenanceThreadPool);

  
  
  
  
  
  

  if (IdleMaintenanceMustEnd(aRunId)) {
    return;
  }

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  nsCOMPtr<nsIFile> storageDir = GetFileForPath(quotaManager->GetStoragePath());
  MOZ_ASSERT(storageDir);

  bool exists;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(storageDir->Exists(&exists)));
  if (!exists) {
    return;
  }

  bool isDirectory;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(storageDir->IsDirectory(&isDirectory)));
  if (NS_WARN_IF(!isDirectory)) {
    return;
  }

  
  
  static const PersistenceType kPersistenceTypes[] = {
    PERSISTENCE_TYPE_PERSISTENT,
    PERSISTENCE_TYPE_DEFAULT,
    PERSISTENCE_TYPE_TEMPORARY
  };

  static_assert((sizeof(kPersistenceTypes) / sizeof(kPersistenceTypes[0])) ==
                  size_t(PERSISTENCE_TYPE_INVALID),
                "Something changed with available persistence types!");

  NS_NAMED_LITERAL_STRING(idbDirName, IDB_DIRECTORY_NAME);
  NS_NAMED_LITERAL_STRING(sqliteExtension, ".sqlite");

  for (const PersistenceType persistenceType : kPersistenceTypes) {
    
    if (IdleMaintenanceMustEnd(aRunId)) {
      return;
    }

    nsAutoCString persistenceTypeString;
    if (persistenceType == PERSISTENCE_TYPE_PERSISTENT) {
      
      persistenceTypeString.AssignLiteral("permanent");
    } else {
      PersistenceTypeToText(persistenceType, persistenceTypeString);
    }

    nsCOMPtr<nsIFile> persistenceDir;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      storageDir->Clone(getter_AddRefs(persistenceDir))));
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      persistenceDir->Append(NS_ConvertASCIItoUTF16(persistenceTypeString))));

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(persistenceDir->Exists(&exists)));
    if (!exists) {
      continue;
    }

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(persistenceDir->IsDirectory(&isDirectory)));
    if (NS_WARN_IF(!isDirectory)) {
      continue;
    }

    nsCOMPtr<nsISimpleEnumerator> persistenceDirEntries;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      persistenceDir->GetDirectoryEntries(
        getter_AddRefs(persistenceDirEntries))));
    if (!persistenceDirEntries) {
      continue;
    }

    while (true) {
      
      if (IdleMaintenanceMustEnd(aRunId)) {
        return;
      }

      bool persistenceDirHasMoreEntries;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        persistenceDirEntries->HasMoreElements(&persistenceDirHasMoreEntries)));

      if (!persistenceDirHasMoreEntries) {
        break;
      }

      nsCOMPtr<nsISupports> persistenceDirEntry;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        persistenceDirEntries->GetNext(getter_AddRefs(persistenceDirEntry))));

      nsCOMPtr<nsIFile> originDir = do_QueryInterface(persistenceDirEntry);
      MOZ_ASSERT(originDir);

      MOZ_ASSERT(NS_SUCCEEDED(originDir->Exists(&exists)));
      MOZ_ASSERT(exists);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(originDir->IsDirectory(&isDirectory)));
      if (!isDirectory) {
        continue;
      }

      nsCOMPtr<nsIFile> idbDir;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(originDir->Clone(getter_AddRefs(idbDir))));

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(idbDir->Append(idbDirName)));

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(idbDir->Exists(&exists)));
      if (!exists) {
        continue;
      }

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(idbDir->IsDirectory(&isDirectory)));
      if (NS_WARN_IF(!isDirectory)) {
        continue;
      }

      nsCOMPtr<nsISimpleEnumerator> idbDirEntries;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        idbDir->GetDirectoryEntries(getter_AddRefs(idbDirEntries))));
      if (!idbDirEntries) {
        continue;
      }

      nsCString group;
      nsCString origin;
      bool isApp;
      nsTArray<nsString> databasePaths;

      while (true) {
        
        if (IdleMaintenanceMustEnd(aRunId)) {
          return;
        }

        bool idbDirHasMoreEntries;
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
          idbDirEntries->HasMoreElements(&idbDirHasMoreEntries)));

        if (!idbDirHasMoreEntries) {
          break;
        }

        nsCOMPtr<nsISupports> idbDirEntry;
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
          idbDirEntries->GetNext(getter_AddRefs(idbDirEntry))));

        nsCOMPtr<nsIFile> idbDirFile = do_QueryInterface(idbDirEntry);
        MOZ_ASSERT(idbDirFile);

        nsString idbFilePath;
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(idbDirFile->GetPath(idbFilePath)));

        if (!StringEndsWith(idbFilePath, sqliteExtension)) {
          continue;
        }

        MOZ_ASSERT(NS_SUCCEEDED(idbDirFile->Exists(&exists)));
        MOZ_ASSERT(exists);

        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(idbDirFile->IsDirectory(&isDirectory)));
        if (isDirectory) {
          continue;
        }

        
        if (databasePaths.IsEmpty()) {
          MOZ_ASSERT(group.IsEmpty());
          MOZ_ASSERT(origin.IsEmpty());

          int64_t dummyTimeStamp;
          if (NS_WARN_IF(NS_FAILED(
                QuotaManager::GetDirectoryMetadata(originDir,
                                                   &dummyTimeStamp,
                                                   group,
                                                   origin,
                                                   &isApp)))) {
            
            continue;
          }
        }

        MOZ_ASSERT(!databasePaths.Contains(idbFilePath));

        databasePaths.AppendElement(idbFilePath);
      }

      if (!databasePaths.IsEmpty()) {
        nsCOMPtr<nsIRunnable> runnable =
          NS_NewRunnableMethodWithArgs<uint32_t, MultipleMaintenanceInfo&&>(
            this,
            &QuotaClient::GetDirectoryLockForIdleMaintenance,
            aRunId,
            MultipleMaintenanceInfo(group,
                                    origin,
                                    persistenceType,
                                    isApp,
                                    Move(databasePaths)));
        MOZ_ASSERT(runnable);

        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
      }
    }
  }
}

void
QuotaClient::GetDirectoryLockForIdleMaintenance(
                                     uint32_t aRunId,
                                     MultipleMaintenanceInfo&& aMaintenanceInfo)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (IdleMaintenanceMustEnd(aRunId)) {
    return;
  }

  MOZ_ASSERT(mMaintenanceInfoHashtable);

  nsAutoCString key;
  key.AppendInt(aMaintenanceInfo.mPersistenceType);
  key.Append('*');
  key.Append(aMaintenanceInfo.mOrigin);

  MOZ_ASSERT(!mMaintenanceInfoHashtable->Get(key));

  MultipleMaintenanceInfo* maintenanceInfo =
    new MultipleMaintenanceInfo(Move(aMaintenanceInfo));

  mMaintenanceInfoHashtable->Put(key, maintenanceInfo);

  nsRefPtr<GetDirectoryLockListener> listener =
    new GetDirectoryLockListener(this, aRunId, key);

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  quotaManager->OpenDirectory(maintenanceInfo->mPersistenceType,
                              maintenanceInfo->mGroup,
                              maintenanceInfo->mOrigin,
                              maintenanceInfo->mIsApp,
                              Client::IDB,
                               false,
                              listener);
}

void
QuotaClient::ScheduleIdleMaintenance(uint32_t aRunId,
                                     const nsACString& aKey,
                                     const MultipleMaintenanceInfo& aMaintenanceInfo)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aKey.IsEmpty());

  MOZ_ASSERT(mMaintenanceThreadPool);

  for (const nsString& databasePath : aMaintenanceInfo.mDatabasePaths) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethodWithArgs<uint32_t,
                                   nsCString,
                                   SingleMaintenanceInfo&&>(
        this,
        &QuotaClient::PerformIdleMaintenanceOnDatabase,
        aRunId,
        aKey,
        SingleMaintenanceInfo(aMaintenanceInfo.mGroup,
                              aMaintenanceInfo.mOrigin,
                              aMaintenanceInfo.mPersistenceType,
                              databasePath));
    MOZ_ASSERT(runnable);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mMaintenanceThreadPool->Dispatch(runnable, NS_DISPATCH_NORMAL)));
  }
}

void
QuotaClient::PerformIdleMaintenanceOnDatabase(
                                       uint32_t aRunId,
                                       const nsACString& aKey,
                                       SingleMaintenanceInfo&& aMaintenanceInfo)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mMaintenanceThreadPool);
  MOZ_ASSERT(mMaintenanceStartTime);
  MOZ_ASSERT(!aMaintenanceInfo.mDatabasePath.IsEmpty());
  MOZ_ASSERT(!aMaintenanceInfo.mGroup.IsEmpty());
  MOZ_ASSERT(!aMaintenanceInfo.mOrigin.IsEmpty());

  PerformIdleMaintenanceOnDatabaseInternal(aRunId, aMaintenanceInfo);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethodWithArgs<nsCString, nsString>(
      this,
      &QuotaClient::MaybeReleaseDirectoryLockForIdleMaintenance,
      aKey,
      aMaintenanceInfo.mDatabasePath);
  MOZ_ASSERT(runnable);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
}

void
QuotaClient::PerformIdleMaintenanceOnDatabaseInternal(
                                  uint32_t aRunId,
                                  const SingleMaintenanceInfo& aMaintenanceInfo)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mMaintenanceThreadPool);
  MOZ_ASSERT(mMaintenanceStartTime);
  MOZ_ASSERT(!aMaintenanceInfo.mDatabasePath.IsEmpty());
  MOZ_ASSERT(!aMaintenanceInfo.mGroup.IsEmpty());
  MOZ_ASSERT(!aMaintenanceInfo.mOrigin.IsEmpty());

  nsCOMPtr<nsIFile> databaseFile =
    GetFileForPath(aMaintenanceInfo.mDatabasePath);
  MOZ_ASSERT(databaseFile);

  nsCOMPtr<mozIStorageConnection> connection;
  nsresult rv = GetStorageConnection(databaseFile,
                                     aMaintenanceInfo.mPersistenceType,
                                     aMaintenanceInfo.mGroup,
                                     aMaintenanceInfo.mOrigin,
                                     TelemetryIdForFile(databaseFile),
                                     getter_AddRefs(connection));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  if (IdleMaintenanceMustEnd(aRunId)) {
    return;
  }

  AutoProgressHandler progressHandler(this, aRunId);
  if (NS_WARN_IF(NS_FAILED(progressHandler.Register(connection)))) {
    return;
  }

  bool databaseIsOk;
  rv = CheckIntegrity(connection, &databaseIsOk);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  if (NS_WARN_IF(!databaseIsOk)) {
    
    
    MOZ_ASSERT(false, "Database corruption detected!");
    return;
  }

  if (IdleMaintenanceMustEnd(aRunId)) {
    return;
  }

  MaintenanceAction maintenanceAction;
  rv = DetermineMaintenanceAction(connection, databaseFile, &maintenanceAction);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  if (IdleMaintenanceMustEnd(aRunId)) {
    return;
  }

  switch (maintenanceAction) {
    case MaintenanceAction_Nothing:
      break;

    case MaintenanceAction_IncrementalVacuum:
      IncrementalVacuum(connection);
      break;

    case MaintenanceAction_FullVacuum:
      FullVacuum(connection, databaseFile);
      break;

    default:
      MOZ_CRASH("Unknown MaintenanceAction!");
  }
}

nsresult
QuotaClient::CheckIntegrity(mozIStorageConnection* aConnection, bool* aOk)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(aOk);

  nsresult rv;

  
  
  {
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "PRAGMA integrity_check(1);"
    ), getter_AddRefs(stmt));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(hasResult);

    nsString result;
    rv = stmt->GetString(0, result);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!result.EqualsLiteral("ok"))) {
      *aOk = false;
      return NS_OK;
    }
  }

  
  {
    int32_t foreignKeysWereEnabled;
    {
      nsCOMPtr<mozIStorageStatement> stmt;
      rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
        "PRAGMA foreign_keys;"
      ), getter_AddRefs(stmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      bool hasResult;
      rv = stmt->ExecuteStep(&hasResult);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(hasResult);

      rv = stmt->GetInt32(0, &foreignKeysWereEnabled);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    bool changedForeignKeys;
    if (foreignKeysWereEnabled) {
      changedForeignKeys = false;
    } else {
      rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "PRAGMA foreign_keys = ON;"
      ));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      changedForeignKeys = true;
    }

    bool foreignKeyError;
    {
      nsCOMPtr<mozIStorageStatement> stmt;
      rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
        "PRAGMA foreign_key_check;"
      ), getter_AddRefs(stmt));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->ExecuteStep(&foreignKeyError);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    if (changedForeignKeys) {
      nsAutoCString stmtSQL;
      stmtSQL.AssignLiteral("PRAGMA foreign_keys = ");
      if (foreignKeysWereEnabled) {
        stmtSQL.AppendLiteral("ON");
      } else {
        stmtSQL.AppendLiteral("OFF");
      }
      stmtSQL.Append(';');

      rv = aConnection->ExecuteSimpleSQL(stmtSQL);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    if (foreignKeyError) {
      *aOk = false;
      return NS_OK;
    }
  }

  *aOk = true;
  return NS_OK;
}

nsresult
QuotaClient::DetermineMaintenanceAction(mozIStorageConnection* aConnection,
                                        nsIFile* aDatabaseFile,
                                        MaintenanceAction* aMaintenanceAction)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(aDatabaseFile);
  MOZ_ASSERT(aMaintenanceAction);

  int32_t schemaVersion;
  nsresult rv = aConnection->GetSchemaVersion(&schemaVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  if (schemaVersion < MakeSchemaVersion(18, 0)) {
    *aMaintenanceAction = MaintenanceAction_Nothing;
    return NS_OK;
  }

  bool lowDiskSpace = IndexedDatabaseManager::InLowDiskSpaceMode();

  if (kRunningXPCShellTests) {
    
    
    
    lowDiskSpace = ((PR_Now() / PR_USEC_PER_MSEC) % 2) == 0;
  }

  
  
  
  if (lowDiskSpace) {
    *aMaintenanceAction = MaintenanceAction_IncrementalVacuum;
    return NS_OK;
  }

  
  
  mozStorageTransaction transaction(aConnection,
                                     false);

  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT last_vacuum_time, last_vacuum_size "
      "FROM database;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  PRTime lastVacuumTime;
  rv = stmt->GetInt64(0, &lastVacuumTime);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int64_t lastVacuumSize;
  rv = stmt->GetInt64(1, &lastVacuumSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(lastVacuumSize > 0);

  
  if (NS_WARN_IF(mMaintenanceStartTime <= lastVacuumTime)) {
    *aMaintenanceAction = MaintenanceAction_Nothing;
    return NS_OK;
  }

  if (mMaintenanceStartTime - lastVacuumTime < kMinVacuumAge) {
    *aMaintenanceAction = MaintenanceAction_IncrementalVacuum;
    return NS_OK;
  }

  
  
  rv = aConnection->EnableModule(NS_LITERAL_CSTRING("dbstat"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE VIRTUAL TABLE __stats__ USING dbstat;"
    "CREATE TEMP TABLE __temp_stats__ AS SELECT * FROM __stats__;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT SUM(__ts1__.pageno != __ts2__.pageno + 1) * 100.0 / COUNT(*) "
      "FROM __temp_stats__ AS __ts1__, __temp_stats__ AS __ts2__ "
      "WHERE __ts1__.name = __ts2__.name "
      "AND __ts1__.rowid = __ts2__.rowid + 1;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  int32_t percentUnordered;
  rv = stmt->GetInt32(0, &percentUnordered);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(percentUnordered >= 0);
  MOZ_ASSERT(percentUnordered <= 100);

  if (percentUnordered >= kPercentUnorderedThreshold) {
    *aMaintenanceAction = MaintenanceAction_FullVacuum;
    return NS_OK;
  }

  
  int64_t currentFileSize;
  rv = aDatabaseFile->GetFileSize(&currentFileSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (currentFileSize <= lastVacuumSize ||
      (((currentFileSize - lastVacuumSize) * 100 / currentFileSize) <
         kPercentFileSizeGrowthThreshold)) {
    *aMaintenanceAction = MaintenanceAction_IncrementalVacuum;
    return NS_OK;
  }

  
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "PRAGMA freelist_count;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  int32_t freelistCount;
  rv = stmt->GetInt32(0, &freelistCount);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(freelistCount >= 0);

  
  
  if (freelistCount > kMaxFreelistThreshold) {
    *aMaintenanceAction = MaintenanceAction_IncrementalVacuum;
    return NS_OK;
  }

  
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT SUM(unused) * 100.0 / SUM(pgsize) "
      "FROM __temp_stats__;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(hasResult);

  int32_t percentUnused;
  rv = stmt->GetInt32(0, &percentUnused);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(percentUnused >= 0);
  MOZ_ASSERT(percentUnused <= 100);

  *aMaintenanceAction = percentUnused >= kPercentUnusedThreshold ?
                        MaintenanceAction_FullVacuum :
                        MaintenanceAction_IncrementalVacuum;
  return NS_OK;
}

void
QuotaClient::IncrementalVacuum(mozIStorageConnection* aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "PRAGMA incremental_vacuum;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }
}

void
QuotaClient::FullVacuum(mozIStorageConnection* aConnection,
                        nsIFile* aDatabaseFile)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(aDatabaseFile);

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "VACUUM;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  PRTime vacuumTime = PR_Now();
  MOZ_ASSERT(vacuumTime > 0);

  int64_t fileSize;
  rv = aDatabaseFile->GetFileSize(&fileSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  MOZ_ASSERT(fileSize > 0);

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE database "
      "SET last_vacuum_time = :time"
        ", last_vacuum_size = :size;"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("time"), vacuumTime);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("size"), fileSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }
}

void
QuotaClient::MaybeReleaseDirectoryLockForIdleMaintenance(
                                                 const nsACString& aKey,
                                                 const nsAString& aDatabasePath)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aKey.IsEmpty());
  MOZ_ASSERT(!aDatabasePath.IsEmpty());
  MOZ_ASSERT(mMaintenanceInfoHashtable);

  MultipleMaintenanceInfo* maintenanceInfo;
  MOZ_ALWAYS_TRUE(mMaintenanceInfoHashtable->Get(aKey, &maintenanceInfo));
  MOZ_ASSERT(maintenanceInfo);

  MOZ_ALWAYS_TRUE(maintenanceInfo->mDatabasePaths.RemoveElement(aDatabasePath));

  if (maintenanceInfo->mDatabasePaths.IsEmpty()) {
    
    maintenanceInfo->mDirectoryLock = nullptr;

    
    mMaintenanceInfoHashtable->Remove(aKey);
  }
}

NS_IMETHODIMP
QuotaClient::Observe(nsISupports* aSubject,
                     const char* aTopic,
                     const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!strcmp(aTopic, OBSERVER_TOPIC_IDLE)) {
    StartIdleMaintenance();
    return NS_OK;
  }

  if (!strcmp(aTopic, OBSERVER_TOPIC_ACTIVE)) {
    StopIdleMaintenance();
    return NS_OK;
  }

  MOZ_ASSERT_UNREACHABLE("Should never get here!");
  return NS_OK;
}

nsresult
QuotaClient::
AutoProgressHandler::Register(mozIStorageConnection* aConnection)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);

  
  
  static const int32_t kProgressGranularity = 50;

  nsCOMPtr<mozIStorageProgressHandler> oldHandler;
  nsresult rv = aConnection->SetProgressHandler(kProgressGranularity,
                                                this,
                                                getter_AddRefs(oldHandler));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!oldHandler);
  mConnection = aConnection;

  return NS_OK;
}

void
QuotaClient::
AutoProgressHandler::Unregister()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mConnection);

  nsCOMPtr<mozIStorageProgressHandler> oldHandler;
  nsresult rv = mConnection->RemoveProgressHandler(getter_AddRefs(oldHandler));
  unused << NS_WARN_IF(NS_FAILED(rv));

  MOZ_ASSERT_IF(NS_SUCCEEDED(rv), oldHandler == this);
}

NS_IMETHODIMP_(MozExternalRefCountType)
QuotaClient::
AutoProgressHandler::AddRef()
{
  NS_ASSERT_OWNINGTHREAD(QuotaClient::AutoProgressHandler);

  mDEBUGRefCnt++;
  return 2;
}

NS_IMETHODIMP_(MozExternalRefCountType)
QuotaClient::
AutoProgressHandler::Release()
{
  NS_ASSERT_OWNINGTHREAD(QuotaClient::AutoProgressHandler);

  mDEBUGRefCnt--;
  return 1;
}

NS_IMPL_QUERY_INTERFACE(QuotaClient::AutoProgressHandler,
                        mozIStorageProgressHandler)

NS_IMETHODIMP
QuotaClient::
AutoProgressHandler::OnProgress(mozIStorageConnection* aConnection,
                                bool* _retval)
{
  NS_ASSERT_OWNINGTHREAD(QuotaClient::AutoProgressHandler);
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(mConnection == aConnection);
  MOZ_ASSERT(_retval);

  *_retval = mQuotaClient->IdleMaintenanceMustEnd(mRunId);
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(QuotaClient::ShutdownWorkThreadsRunnable,
                             nsRunnable)

NS_IMETHODIMP
QuotaClient::
ShutdownWorkThreadsRunnable::Run()
{
  if (NS_IsMainThread()) {
    MOZ_ASSERT(QuotaClient::GetInstance() == mQuotaClient);
    MOZ_ASSERT(mQuotaClient->mShutdownRunnable == this);

    mQuotaClient->mShutdownRunnable = nullptr;
    mQuotaClient = nullptr;

    return NS_OK;
  }

  AssertIsOnBackgroundThread();

  nsRefPtr<ConnectionPool> connectionPool = gConnectionPool.get();
  if (connectionPool) {
    connectionPool->Shutdown();

    gConnectionPool = nullptr;
  }

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));

  return NS_OK;
}


PLDHashOperator
QuotaClient::
AbortOperationsRunnable::MatchOrigin(const nsACString& aKey,
                                     DatabaseActorInfo* aValue,
                                     void* aClosure)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!aKey.IsEmpty());
  MOZ_ASSERT(aValue);
  MOZ_ASSERT(aClosure);

  auto* closure = static_cast<AbortOperationsRunnable*>(aClosure);

  for (Database* database : aValue->mLiveDatabases) {
    if (closure->mOrigin.IsVoid() || closure->mOrigin == database->Origin()) {
      closure->mDatabases.AppendElement(database);
    }
  }

  return PL_DHASH_NEXT;
}


PLDHashOperator
QuotaClient::
AbortOperationsRunnable::MatchContentParentId(const nsACString& aKey,
                                              DatabaseActorInfo* aValue,
                                              void* aClosure)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(!aKey.IsEmpty());
  MOZ_ASSERT(aValue);
  MOZ_ASSERT(aClosure);

  auto* closure = static_cast<AbortOperationsRunnable*>(aClosure);

  for (Database* database : aValue->mLiveDatabases) {
    if (database->IsOwnedByProcess(closure->mContentParentId)) {
      closure->mDatabases.AppendElement(database);
    }
  }

  return PL_DHASH_NEXT;
}

NS_IMPL_ISUPPORTS_INHERITED0(QuotaClient::AbortOperationsRunnable, nsRunnable)

NS_IMETHODIMP
QuotaClient::
AbortOperationsRunnable::Run()
{
  AssertIsOnBackgroundThread();

  if (!gLiveDatabaseHashtable) {
    return NS_OK;
  }

  if (mOrigin.IsEmpty()) {
    gLiveDatabaseHashtable->EnumerateRead(MatchContentParentId, this);
  } else {
    gLiveDatabaseHashtable->EnumerateRead(MatchOrigin, this);
  }

  for (Database* database : mDatabases) {
    database->Invalidate();
  }

  mDatabases.Clear();

  return NS_OK;
}

void
QuotaClient::
GetDirectoryLockListener::DirectoryLockAcquired(DirectoryLock* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());

  MultipleMaintenanceInfo* maintenanceInfo;
  MOZ_ALWAYS_TRUE(
    mQuotaClient->mMaintenanceInfoHashtable->Get(mKey, &maintenanceInfo));
  MOZ_ASSERT(maintenanceInfo);
  MOZ_ASSERT(!maintenanceInfo->mDirectoryLock);

  if (mQuotaClient->IdleMaintenanceMustEnd(mRunId)) {
#ifdef DEBUG
    maintenanceInfo->mDatabasePaths.Clear();
#endif

    mQuotaClient->mMaintenanceInfoHashtable->Remove(mKey);
    return;
  }

  maintenanceInfo->mDirectoryLock = aLock;

  mQuotaClient->ScheduleIdleMaintenance(mRunId, mKey, *maintenanceInfo);
}

void
QuotaClient::
GetDirectoryLockListener::DirectoryLockFailed()
{
  MOZ_ASSERT(NS_IsMainThread());

  DebugOnly<MultipleMaintenanceInfo*> maintenanceInfo;
  MOZ_ASSERT(
    mQuotaClient->mMaintenanceInfoHashtable->Get(mKey, &maintenanceInfo));
  MOZ_ASSERT(maintenanceInfo);
  MOZ_ASSERT(!maintenanceInfo->mDirectoryLock);

  mQuotaClient->mMaintenanceInfoHashtable->Remove(mKey);
}





NS_IMPL_ISUPPORTS(CompressDataBlobsFunction, mozIStorageFunction)
NS_IMPL_ISUPPORTS(EncodeKeysFunction, mozIStorageFunction)


void
DatabaseOperationBase::GetBindingClauseForKeyRange(
                                            const SerializedKeyRange& aKeyRange,
                                            const nsACString& aKeyColumnName,
                                            nsAutoCString& aBindingClause)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(!aKeyColumnName.IsEmpty());

  NS_NAMED_LITERAL_CSTRING(andStr, " AND ");
  NS_NAMED_LITERAL_CSTRING(spacecolon, " :");
  NS_NAMED_LITERAL_CSTRING(lowerKey, "lower_key");

  if (aKeyRange.isOnly()) {
    
    aBindingClause = andStr + aKeyColumnName + NS_LITERAL_CSTRING(" =") +
                     spacecolon + lowerKey;
    return;
  }

  aBindingClause.Truncate();

  if (!aKeyRange.lower().IsUnset()) {
    
    aBindingClause.Append(andStr + aKeyColumnName);
    aBindingClause.AppendLiteral(" >");
    if (!aKeyRange.lowerOpen()) {
      aBindingClause.AppendLiteral("=");
    }
    aBindingClause.Append(spacecolon + lowerKey);
  }

  if (!aKeyRange.upper().IsUnset()) {
    
    aBindingClause.Append(andStr + aKeyColumnName);
    aBindingClause.AppendLiteral(" <");
    if (!aKeyRange.upperOpen()) {
      aBindingClause.AppendLiteral("=");
    }
    aBindingClause.Append(spacecolon + NS_LITERAL_CSTRING("upper_key"));
  }

  MOZ_ASSERT(!aBindingClause.IsEmpty());
}


uint64_t
DatabaseOperationBase::ReinterpretDoubleAsUInt64(double aDouble)
{
  
  union {
    double d;
    uint64_t u;
  } pun;
  pun.d = aDouble;
  return pun.u;
}


template <typename T>
nsresult
DatabaseOperationBase::GetStructuredCloneReadInfoFromSource(
                                                 T* aSource,
                                                 uint32_t aDataIndex,
                                                 uint32_t aFileIdsIndex,
                                                 FileManager* aFileManager,
                                                 StructuredCloneReadInfo* aInfo)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aSource);
  MOZ_ASSERT(aFileManager);
  MOZ_ASSERT(aInfo);

#ifdef DEBUG
  {
    int32_t columnType;
    MOZ_ASSERT(NS_SUCCEEDED(aSource->GetTypeOfIndex(aDataIndex, &columnType)));
    MOZ_ASSERT(columnType == mozIStorageStatement::VALUE_TYPE_BLOB);
  }
#endif

  const uint8_t* blobData;
  uint32_t blobDataLength;
  nsresult rv = aSource->GetSharedBlob(aDataIndex, &blobDataLength, &blobData);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool isNull;
  rv = aSource->GetIsNull(aFileIdsIndex, &isNull);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsString fileIds;

  if (isNull) {
    fileIds.SetIsVoid(true);
  } else {
    rv = aSource->GetString(aFileIdsIndex, fileIds);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = GetStructuredCloneReadInfoFromBlob(blobData,
                                          blobDataLength,
                                          fileIds,
                                          aFileManager,
                                          aInfo);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::GetStructuredCloneReadInfoFromBlob(
                                                 const uint8_t* aBlobData,
                                                 uint32_t aBlobDataLength,
                                                 const nsAString& aFileIds,
                                                 FileManager* aFileManager,
                                                 StructuredCloneReadInfo* aInfo)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aFileManager);
  MOZ_ASSERT(aInfo);

  PROFILER_LABEL("IndexedDB",
                 "DatabaseOperationBase::GetStructuredCloneReadInfoFromBlob",
                 js::ProfileEntry::Category::STORAGE);

  const char* compressed = reinterpret_cast<const char*>(aBlobData);
  size_t compressedLength = size_t(aBlobDataLength);

  size_t uncompressedLength;
  if (NS_WARN_IF(!snappy::GetUncompressedLength(compressed, compressedLength,
                                                &uncompressedLength))) {
    return NS_ERROR_FILE_CORRUPTED;
  }

  AutoFallibleTArray<uint8_t, 512> uncompressed;
  if (NS_WARN_IF(!uncompressed.SetLength(uncompressedLength, fallible))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  char* uncompressedBuffer = reinterpret_cast<char*>(uncompressed.Elements());

  if (NS_WARN_IF(!snappy::RawUncompress(compressed, compressedLength,
                                        uncompressedBuffer))) {
    return NS_ERROR_FILE_CORRUPTED;
  }

  aInfo->mData.SwapElements(uncompressed);

  if (!aFileIds.IsVoid()) {
    nsAutoTArray<int64_t, 10> array;
    nsresult rv = ConvertFileIdsToArray(aFileIds, array);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    for (uint32_t count = array.Length(), index = 0; index < count; index++) {
      MOZ_ASSERT(array[index] > 0);

      nsRefPtr<FileInfo> fileInfo = aFileManager->GetFileInfo(array[index]);
      MOZ_ASSERT(fileInfo);

      StructuredCloneFile* file = aInfo->mFiles.AppendElement();
      file->mFileInfo.swap(fileInfo);
    }
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::BindKeyRangeToStatement(
                                            const SerializedKeyRange& aKeyRange,
                                            mozIStorageStatement* aStatement)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aStatement);

  NS_NAMED_LITERAL_CSTRING(lowerKey, "lower_key");

  if (aKeyRange.isOnly()) {
    return aKeyRange.lower().BindToStatement(aStatement, lowerKey);
  }

  nsresult rv;

  if (!aKeyRange.lower().IsUnset()) {
    rv = aKeyRange.lower().BindToStatement(aStatement, lowerKey);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (!aKeyRange.upper().IsUnset()) {
    rv = aKeyRange.upper().BindToStatement(aStatement,
                                           NS_LITERAL_CSTRING("upper_key"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}


void
DatabaseOperationBase::AppendConditionClause(const nsACString& aColumnName,
                                             const nsACString& aArgName,
                                             bool aLessThan,
                                             bool aEquals,
                                             nsAutoCString& aResult)
{
  aResult += NS_LITERAL_CSTRING(" AND ") + aColumnName +
             NS_LITERAL_CSTRING(" ");

  if (aLessThan) {
    aResult.Append('<');
  }
  else {
    aResult.Append('>');
  }

  if (aEquals) {
    aResult.Append('=');
  }

  aResult += NS_LITERAL_CSTRING(" :") + aArgName;
}


nsresult
DatabaseOperationBase::GetUniqueIndexTableForObjectStore(
                                TransactionBase* aTransaction,
                                int64_t aObjectStoreId,
                                Maybe<UniqueIndexTable>& aMaybeUniqueIndexTable)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(aMaybeUniqueIndexTable.isNothing());

  class MOZ_STACK_CLASS Helper final
  {
  public:
    static nsresult
    CopyUniqueValues(const IndexTable& aIndexes,
                     Maybe<UniqueIndexTable>& aMaybeUniqueIndexTable)
    {
      const uint32_t indexCount = aIndexes.Count();
      MOZ_ASSERT(indexCount);

      aMaybeUniqueIndexTable.emplace();

      aIndexes.EnumerateRead(Enumerate, aMaybeUniqueIndexTable.ptr());

      if (NS_WARN_IF(aMaybeUniqueIndexTable.ref().Count() != indexCount)) {
        IDB_REPORT_INTERNAL_ERR();
        aMaybeUniqueIndexTable.reset();
        return NS_ERROR_OUT_OF_MEMORY;
      }

#ifdef DEBUG
      aMaybeUniqueIndexTable.ref().MarkImmutable();
#endif
      return NS_OK;
    }

  private:
    static PLDHashOperator
    Enumerate(const uint64_t& aKey, FullIndexMetadata* aValue, void* aClosure)
    {
      auto* uniqueIndexTable = static_cast<UniqueIndexTable*>(aClosure);
      MOZ_ASSERT(uniqueIndexTable);
      MOZ_ASSERT(!uniqueIndexTable->Get(aValue->mCommonMetadata.id()));

      if (NS_WARN_IF(!uniqueIndexTable->Put(aValue->mCommonMetadata.id(),
                                            aValue->mCommonMetadata.unique(),
                                            fallible))) {
        return PL_DHASH_STOP;
      }

      return PL_DHASH_NEXT;
    }
  };

  const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata = 
    aTransaction->GetMetadataForObjectStoreId(aObjectStoreId);
  MOZ_ASSERT(objectStoreMetadata);

  if (!objectStoreMetadata->mIndexes.Count()) {
    return NS_OK;
  }

  nsresult rv = Helper::CopyUniqueValues(objectStoreMetadata->mIndexes,
                                         aMaybeUniqueIndexTable);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::IndexDataValuesFromUpdateInfos(
                                  const nsTArray<IndexUpdateInfo>& aUpdateInfos,
                                  const UniqueIndexTable& aUniqueIndexTable,
                                  FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(aIndexValues.IsEmpty());
  MOZ_ASSERT_IF(!aUpdateInfos.IsEmpty(), aUniqueIndexTable.Count());

  PROFILER_LABEL("IndexedDB",
                 "DatabaseOperationBase::IndexDataValuesFromUpdateInfos",
                 js::ProfileEntry::Category::STORAGE);

  const uint32_t count = aUpdateInfos.Length();

  if (!count) {
    return NS_OK;
  }

  if (NS_WARN_IF(!aIndexValues.SetCapacity(count, fallible))) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t idxIndex = 0; idxIndex < count; idxIndex++) {
    const IndexUpdateInfo& updateInfo = aUpdateInfos[idxIndex];
    const int64_t& indexId = updateInfo.indexId();
    const Key& key = updateInfo.value();

    bool unique;
    MOZ_ALWAYS_TRUE(aUniqueIndexTable.Get(indexId, &unique));

    MOZ_ALWAYS_TRUE(
      aIndexValues.InsertElementSorted(IndexDataValue(indexId, unique, key),
                                       fallible));
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::InsertIndexTableRows(
                             DatabaseConnection* aConnection,
                             const int64_t aObjectStoreId,
                             const Key& aObjectStoreKey,
                             const FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!aObjectStoreKey.IsUnset());

  PROFILER_LABEL("IndexedDB",
                 "DatabaseOperationBase::InsertIndexTableRows",
                 js::ProfileEntry::Category::STORAGE);

  const uint32_t count = aIndexValues.Length();
  if (!count) {
    return NS_OK;
  }

  NS_NAMED_LITERAL_CSTRING(objectStoreIdString, "object_store_id");
  NS_NAMED_LITERAL_CSTRING(objectDataKeyString, "object_data_key");
  NS_NAMED_LITERAL_CSTRING(indexIdString, "index_id");
  NS_NAMED_LITERAL_CSTRING(valueString, "value");

  DatabaseConnection::CachedStatement insertUniqueStmt;
  DatabaseConnection::CachedStatement insertStmt;

  nsresult rv;

  for (uint32_t index = 0; index < count; index++) {
    const IndexDataValue& info = aIndexValues[index];

    DatabaseConnection::CachedStatement& stmt =
      info.mUnique ? insertUniqueStmt : insertStmt;

    if (stmt) {
      stmt.Reset();
    } else if (info.mUnique) {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "INSERT INTO unique_index_data "
          "(index_id, value, object_store_id, object_data_key) "
          "VALUES (:index_id, :value, :object_store_id, :object_data_key);"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "INSERT OR IGNORE INTO index_data "
          "(index_id, value, object_data_key, object_store_id) "
          "VALUES (:index_id, :value, :object_data_key, :object_store_id);"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = stmt->BindInt64ByName(indexIdString, info.mIndexId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = info.mKey.BindToStatement(stmt, valueString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64ByName(objectStoreIdString, aObjectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aObjectStoreKey.BindToStatement(stmt, objectDataKeyString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (rv == NS_ERROR_STORAGE_CONSTRAINT && info.mUnique) {
      
      
      
      for (int32_t index2 = int32_t(index) - 1;
           index2 >= 0 && aIndexValues[index2].mIndexId == info.mIndexId;
           --index2) {
        if (info.mKey == aIndexValues[index2].mKey) {
          
          
          rv = NS_OK;
          break;
        }
      }
    }

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::DeleteIndexDataTableRows(
                             DatabaseConnection* aConnection,
                             const Key& aObjectStoreKey,
                             const FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!aObjectStoreKey.IsUnset());

  PROFILER_LABEL("IndexedDB",
                 "DatabaseOperationBase::DeleteIndexDataTableRows",
                 js::ProfileEntry::Category::STORAGE);

  const uint32_t count = aIndexValues.Length();
  if (!count) {
    return NS_OK;
  }

  NS_NAMED_LITERAL_CSTRING(indexIdString, "index_id");
  NS_NAMED_LITERAL_CSTRING(valueString, "value");
  NS_NAMED_LITERAL_CSTRING(objectDataKeyString, "object_data_key");

  DatabaseConnection::CachedStatement deleteUniqueStmt;
  DatabaseConnection::CachedStatement deleteStmt;

  nsresult rv;

  for (uint32_t index = 0; index < count; index++) {
    const IndexDataValue& indexValue = aIndexValues[index];

    DatabaseConnection::CachedStatement& stmt =
      indexValue.mUnique ? deleteUniqueStmt : deleteStmt;

    if (stmt) {
      stmt.Reset();
    } else if (indexValue.mUnique) {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "DELETE FROM unique_index_data "
          "WHERE index_id = :index_id "
          "AND value = :value;"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "DELETE FROM index_data "
          "WHERE index_id = :index_id "
          "AND value = :value "
          "AND object_data_key = :object_data_key;"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = stmt->BindInt64ByName(indexIdString, indexValue.mIndexId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = indexValue.mKey.BindToStatement(stmt, valueString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!indexValue.mUnique) {
      rv = aObjectStoreKey.BindToStatement(stmt, objectDataKeyString);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::DeleteObjectStoreDataTableRowsWithIndexes(
                                              DatabaseConnection* aConnection,
                                              const int64_t aObjectStoreId,
                                              const OptionalKeyRange& aKeyRange)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aObjectStoreId);

#ifdef DEBUG
  {
    bool hasIndexes = false;
    MOZ_ASSERT(NS_SUCCEEDED(
      ObjectStoreHasIndexes(aConnection, aObjectStoreId, &hasIndexes)));
    MOZ_ASSERT(hasIndexes,
               "Don't use this slow method if there are no indexes!");
  }
#endif

  PROFILER_LABEL("IndexedDB",
                 "DatabaseOperationBase::"
                 "DeleteObjectStoreDataTableRowsWithIndexes",
                 js::ProfileEntry::Category::STORAGE);

  const bool singleRowOnly =
    aKeyRange.type() == OptionalKeyRange::TSerializedKeyRange &&
    aKeyRange.get_SerializedKeyRange().isOnly();

  NS_NAMED_LITERAL_CSTRING(objectStoreIdString, "object_store_id");
  NS_NAMED_LITERAL_CSTRING(keyString, "key");

  nsresult rv;
  Key objectStoreKey;
  DatabaseConnection::CachedStatement selectStmt;

  if (singleRowOnly) {
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "SELECT index_data_values "
        "FROM object_data "
        "WHERE object_store_id = :object_store_id "
        "AND key = :key;"),
      &selectStmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    objectStoreKey = aKeyRange.get_SerializedKeyRange().lower();

    rv = objectStoreKey.BindToStatement(selectStmt, keyString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    nsAutoCString keyRangeClause;
    if (aKeyRange.type() == OptionalKeyRange::TSerializedKeyRange) {
      GetBindingClauseForKeyRange(aKeyRange.get_SerializedKeyRange(),
                                  keyString,
                                  keyRangeClause);
    }

    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "SELECT index_data_values, key "
        "FROM object_data "
        "WHERE object_store_id = :") +
        objectStoreIdString +
        keyRangeClause +
        NS_LITERAL_CSTRING(";"),
      &selectStmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (aKeyRange.type() == OptionalKeyRange::TSerializedKeyRange) {
      rv = BindKeyRangeToStatement(aKeyRange, selectStmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  rv = selectStmt->BindInt64ByName(objectStoreIdString, aObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DatabaseConnection::CachedStatement deleteStmt;
  AutoFallibleTArray<IndexDataValue, 32> indexValues;

  DebugOnly<uint32_t> resultCountDEBUG = 0;

  bool hasResult;
  while (NS_SUCCEEDED(rv = selectStmt->ExecuteStep(&hasResult)) && hasResult) {
    if (!singleRowOnly) {
      rv = objectStoreKey.SetFromStatement(selectStmt, 1);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      indexValues.ClearAndRetainStorage();
    }

    rv = ReadCompressedIndexDataValues(selectStmt, 0, indexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = DeleteIndexDataTableRows(aConnection, objectStoreKey, indexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (deleteStmt) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(deleteStmt->Reset()));
    } else {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "DELETE FROM object_data "
          "WHERE object_store_id = :object_store_id "
          "AND key = :key;"),
        &deleteStmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = deleteStmt->BindInt64ByName(objectStoreIdString, aObjectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = objectStoreKey.BindToStatement(deleteStmt, keyString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = deleteStmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    resultCountDEBUG++;
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT_IF(singleRowOnly, resultCountDEBUG <= 1);

  return NS_OK;
}


nsresult
DatabaseOperationBase::UpdateIndexValues(
                             DatabaseConnection* aConnection,
                             const int64_t aObjectStoreId,
                             const Key& aObjectStoreKey,
                             const FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!aObjectStoreKey.IsUnset());

  PROFILER_LABEL("IndexedDB",
                 "DatabaunseOperationBase::UpdateIndexValues",
                 js::ProfileEntry::Category::STORAGE);

  UniqueFreePtr<uint8_t> indexDataValues;
  uint32_t indexDataValuesLength;
  nsresult rv = MakeCompressedIndexDataValues(aIndexValues,
                                              indexDataValues,
                                              &indexDataValuesLength);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!indexDataValuesLength == !(indexDataValues.get()));

  DatabaseConnection::CachedStatement updateStmt;
  rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "UPDATE object_data "
      "SET index_data_values = :index_data_values "
      "WHERE object_store_id = :object_store_id "
      "AND key = :key;"),
    &updateStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  NS_NAMED_LITERAL_CSTRING(indexDataValuesString, "index_data_values");

  if (indexDataValues) {
    rv = updateStmt->BindAdoptedBlobByName(indexDataValuesString,
                                           indexDataValues.get(),
                                           indexDataValuesLength);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    indexDataValues.release();
  } else {
    rv = updateStmt->BindNullByName(indexDataValuesString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                                   aObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aObjectStoreKey.BindToStatement(updateStmt, NS_LITERAL_CSTRING("key"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = updateStmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}


nsresult
DatabaseOperationBase::ObjectStoreHasIndexes(DatabaseConnection* aConnection,
                                             const int64_t aObjectStoreId,
                                             bool* aHasIndexes)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(aHasIndexes);

  DatabaseConnection::CachedStatement stmt;

  nsresult rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "SELECT id "
      "FROM object_store_index "
      "WHERE object_store_id = :object_store_id "
      "LIMIT 1;"),
    &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                             aObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  *aHasIndexes = hasResult;
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(DatabaseOperationBase,
                            nsRunnable,
                            mozIStorageProgressHandler)

NS_IMETHODIMP
DatabaseOperationBase::OnProgress(mozIStorageConnection* aConnection,
                                  bool* _retval)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(_retval);

  
  *_retval = !OperationMayProceed();
  return NS_OK;
}

DatabaseOperationBase::
AutoSetProgressHandler::AutoSetProgressHandler()
  : mConnection(nullptr)
  , mDEBUGDatabaseOp(nullptr)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
}

DatabaseOperationBase::
AutoSetProgressHandler::~AutoSetProgressHandler()
{
  MOZ_ASSERT(!IsOnBackgroundThread());

  if (mConnection) {
    nsCOMPtr<mozIStorageProgressHandler> oldHandler;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mConnection->RemoveProgressHandler(getter_AddRefs(oldHandler))));
    MOZ_ASSERT(oldHandler == mDEBUGDatabaseOp);
  }
}

nsresult
DatabaseOperationBase::
AutoSetProgressHandler::Register(mozIStorageConnection* aConnection,
                                 DatabaseOperationBase* aDatabaseOp)
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(aDatabaseOp);
  MOZ_ASSERT(!mConnection);

  nsCOMPtr<mozIStorageProgressHandler> oldProgressHandler;

  nsresult rv =
    aConnection->SetProgressHandler(kStorageProgressGranularity,
                                    aDatabaseOp,
                                    getter_AddRefs(oldProgressHandler));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!oldProgressHandler);

  mConnection = aConnection;
  mDEBUGDatabaseOp = aDatabaseOp;

  return NS_OK;
}

FactoryOp::FactoryOp(Factory* aFactory,
                     already_AddRefed<ContentParent> aContentParent,
                     const CommonFactoryRequestParams& aCommonParams,
                     bool aDeleting)
  : DatabaseOperationBase(aFactory->GetLoggingInfo()->Id(),
                          aFactory->GetLoggingInfo()->NextRequestSN())
  , mFactory(aFactory)
  , mContentParent(Move(aContentParent))
  , mCommonParams(aCommonParams)
  , mState(State_Initial)
  , mIsApp(false)
  , mEnforcingQuota(true)
  , mDeleting(aDeleting)
  , mBlockedDatabaseOpen(false)
  , mChromeWriteAccessAllowed(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFactory);
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnNonMainThread());
}

nsresult
FactoryOp::Open()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_Initial);

  
  nsRefPtr<ContentParent> contentParent;
  mContentParent.swap(contentParent);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  PermissionRequestBase::PermissionValue permission;
  nsresult rv = CheckPermission(contentParent, &permission);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(permission == PermissionRequestBase::kPermissionAllowed ||
             permission == PermissionRequestBase::kPermissionDenied ||
             permission == PermissionRequestBase::kPermissionPrompt);

  if (permission == PermissionRequestBase::kPermissionDenied) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  {
    
    if (NS_WARN_IF(!IndexedDatabaseManager::GetOrCreate())) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    nsCOMPtr<mozIStorageService> ss;
    if (NS_WARN_IF(!(ss = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID)))) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    if (NS_WARN_IF(!QuotaManager::GetOrCreate())) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }

  QuotaClient* quotaClient = QuotaClient::GetInstance();
  if (NS_WARN_IF(!quotaClient)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  quotaClient->NoteBackgroundThread(mOwningThread);

  const DatabaseMetadata& metadata = mCommonParams.metadata();

  QuotaManager::GetStorageId(metadata.persistenceType(),
                             mOrigin,
                             Client::IDB,
                             mDatabaseId);

  mDatabaseId.Append('*');
  mDatabaseId.Append(NS_ConvertUTF16toUTF8(metadata.name()));

  if (permission == PermissionRequestBase::kPermissionPrompt) {
    mState = State_PermissionChallenge;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                         NS_DISPATCH_NORMAL)));
    return NS_OK;
  }

  MOZ_ASSERT(permission == PermissionRequestBase::kPermissionAllowed);

  rv = FinishOpen();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
FactoryOp::ChallengePermission()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_PermissionChallenge);

  const PrincipalInfo& principalInfo = mCommonParams.principalInfo();
  MOZ_ASSERT(principalInfo.type() == PrincipalInfo::TContentPrincipalInfo);

  if (NS_WARN_IF(!SendPermissionChallenge(principalInfo))) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
FactoryOp::RetryCheckPermission()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_PermissionRetry);
  MOZ_ASSERT(mCommonParams.principalInfo().type() ==
               PrincipalInfo::TContentPrincipalInfo);

  
  nsRefPtr<ContentParent> contentParent;
  mContentParent.swap(contentParent);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  PermissionRequestBase::PermissionValue permission;
  nsresult rv = CheckPermission(contentParent, &permission);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(permission == PermissionRequestBase::kPermissionAllowed ||
             permission == PermissionRequestBase::kPermissionDenied ||
             permission == PermissionRequestBase::kPermissionPrompt);

  if (permission == PermissionRequestBase::kPermissionDenied ||
      permission == PermissionRequestBase::kPermissionPrompt) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  MOZ_ASSERT(permission == PermissionRequestBase::kPermissionAllowed);

  rv = FinishOpen();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
FactoryOp::DirectoryOpen()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_DirectoryWorkOpen);
  MOZ_ASSERT(mDirectoryLock);

  
  
  if (!gFactoryOps) {
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  
  bool delayed = false;
  for (uint32_t index = gFactoryOps->Length(); index > 0; index--) {
    nsRefPtr<FactoryOp>& existingOp = (*gFactoryOps)[index - 1];
    if (MustWaitFor(*existingOp)) {
      
      MOZ_ASSERT(!existingOp->mDelayedOp);
      existingOp->mDelayedOp = this;
      delayed = true;
      break;
    }
  }

  
  
  gFactoryOps->AppendElement(this);

  mBlockedDatabaseOpen = true;

  mState = State_DatabaseOpenPending;
  if (!delayed) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));
  }

  return NS_OK;
}

nsresult
FactoryOp::SendToIOThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DatabaseOpenPending);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  
  mState = State_DatabaseWorkOpen;

  nsresult rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  return NS_OK;
}

void
FactoryOp::WaitForTransactions()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_BeginVersionChange ||
             mState == State_WaitingForOtherDatabasesToClose);
  MOZ_ASSERT(!mDatabaseId.IsEmpty());
  MOZ_ASSERT(!IsActorDestroyed());

  nsTArray<nsCString> databaseIds;
  databaseIds.AppendElement(mDatabaseId);

  mState = State_WaitingForTransactionsToComplete;

  nsRefPtr<WaitForTransactionsHelper> helper =
    new WaitForTransactionsHelper(mDatabaseId, this);
  helper->WaitForTransactions();
}

void
FactoryOp::FinishSendResults()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_SendingResults);
  MOZ_ASSERT(mFactory);

  
  nsRefPtr<Factory> factory;
  mFactory.swap(factory);

  if (mBlockedDatabaseOpen) {
    if (mDelayedOp) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(mDelayedOp)));
      mDelayedOp = nullptr;
    }

    MOZ_ASSERT(gFactoryOps);
    gFactoryOps->RemoveElement(this);
  }

  mState = State_Completed;
}

nsresult
FactoryOp::CheckPermission(ContentParent* aContentParent,
                           PermissionRequestBase::PermissionValue* aPermission)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_Initial || mState == State_PermissionRetry);

  const PrincipalInfo& principalInfo = mCommonParams.principalInfo();
  if (principalInfo.type() != PrincipalInfo::TSystemPrincipalInfo &&
      NS_WARN_IF(!Preferences::GetBool(kPrefIndexedDBEnabled, false))) {
    if (aContentParent) {
      
      
      aContentParent->KillHard("IndexedDB CheckPermission 1");
    }
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  if (NS_WARN_IF(mCommonParams.privateBrowsingMode())) {
    
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  PersistenceType persistenceType = mCommonParams.metadata().persistenceType();

  MOZ_ASSERT(principalInfo.type() != PrincipalInfo::TNullPrincipalInfo);

  if (principalInfo.type() == PrincipalInfo::TSystemPrincipalInfo) {
    MOZ_ASSERT(mState == State_Initial);
    MOZ_ASSERT(persistenceType == PERSISTENCE_TYPE_PERSISTENT);

    if (aContentParent) {
      
      
      NS_NAMED_LITERAL_CSTRING(permissionStringBase,
                               PERMISSION_STRING_CHROME_BASE);
      NS_ConvertUTF16toUTF8 databaseName(mCommonParams.metadata().name());
      NS_NAMED_LITERAL_CSTRING(readSuffix, PERMISSION_STRING_CHROME_READ_SUFFIX);
      NS_NAMED_LITERAL_CSTRING(writeSuffix, PERMISSION_STRING_CHROME_WRITE_SUFFIX);

      const nsAutoCString permissionStringWrite =
        permissionStringBase + databaseName + writeSuffix;
      const nsAutoCString permissionStringRead =
        permissionStringBase + databaseName + readSuffix;

      bool canWrite =
        CheckAtLeastOneAppHasPermission(aContentParent, permissionStringWrite);

      bool canRead;
      if (canWrite) {
        MOZ_ASSERT(CheckAtLeastOneAppHasPermission(aContentParent,
                                                   permissionStringRead));
        canRead = true;
      } else {
        canRead =
          CheckAtLeastOneAppHasPermission(aContentParent, permissionStringRead);
      }

      
      if (mDeleting && !canWrite) {
        aContentParent->KillHard("IndexedDB CheckPermission 2");
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      
      if (!canRead) {
        aContentParent->KillHard("IndexedDB CheckPermission 3");
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      mChromeWriteAccessAllowed = canWrite;
    } else {
      mChromeWriteAccessAllowed = true;
    }

    if (State_Initial == mState) {
      QuotaManager::GetInfoForChrome(&mGroup, &mOrigin, &mIsApp);

      MOZ_ASSERT(!QuotaManager::IsFirstPromptRequired(persistenceType, mOrigin,
                                                      mIsApp));

      mEnforcingQuota =
        QuotaManager::IsQuotaEnforced(persistenceType, mOrigin, mIsApp);
    }

    *aPermission = PermissionRequestBase::kPermissionAllowed;
    return NS_OK;
  }

  MOZ_ASSERT(principalInfo.type() == PrincipalInfo::TContentPrincipalInfo);

  nsresult rv;
  nsCOMPtr<nsIPrincipal> principal =
    PrincipalInfoToPrincipal(principalInfo, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCString group;
  nsCString origin;
  bool isApp;
  rv = QuotaManager::GetInfoFromPrincipal(principal, &group, &origin, &isApp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef IDB_MOBILE
  if (persistenceType == PERSISTENCE_TYPE_PERSISTENT &&
      !QuotaManager::IsOriginWhitelistedForPersistentStorage(origin) &&
      !isApp) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }
#endif

  PermissionRequestBase::PermissionValue permission;

  if (QuotaManager::IsFirstPromptRequired(persistenceType, origin, isApp)) {
#ifdef MOZ_CHILD_PERMISSIONS
    if (aContentParent) {
      if (NS_WARN_IF(!AssertAppPrincipal(aContentParent, principal))) {
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      uint32_t intPermission =
        mozilla::CheckPermission(aContentParent, principal, IDB_PREFIX);

      permission =
        PermissionRequestBase::PermissionValueForIntPermission(intPermission);
    } else
#endif 
    {
      rv = PermissionRequestBase::GetCurrentPermission(principal, &permission);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  } else {
    permission = PermissionRequestBase::kPermissionAllowed;
  }

  if (permission != PermissionRequestBase::kPermissionDenied &&
      State_Initial == mState) {
    mGroup = group;
    mOrigin = origin;
    mIsApp = isApp;

    mEnforcingQuota =
      QuotaManager::IsQuotaEnforced(persistenceType, mOrigin, mIsApp);
  }

  *aPermission = permission;
  return NS_OK;
}

nsresult
FactoryOp::SendVersionChangeMessages(DatabaseActorInfo* aDatabaseActorInfo,
                                     Database* aOpeningDatabase,
                                     uint64_t aOldVersion,
                                     const NullableVersion& aNewVersion)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabaseActorInfo);
  MOZ_ASSERT(mState == State_BeginVersionChange);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());
  MOZ_ASSERT(!IsActorDestroyed());

  const uint32_t expectedCount = mDeleting ? 0 : 1;
  const uint32_t liveCount = aDatabaseActorInfo->mLiveDatabases.Length();
  if (liveCount > expectedCount) {
    FallibleTArray<MaybeBlockedDatabaseInfo> maybeBlockedDatabases;
    for (uint32_t index = 0; index < liveCount; index++) {
      Database* database = aDatabaseActorInfo->mLiveDatabases[index];
      if ((!aOpeningDatabase || database != aOpeningDatabase) &&
          !database->IsClosed() &&
          NS_WARN_IF(!maybeBlockedDatabases.AppendElement(database, fallible))) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    if (!maybeBlockedDatabases.IsEmpty()) {
      mMaybeBlockedDatabases.SwapElements(maybeBlockedDatabases);
    }
  }

  if (!mMaybeBlockedDatabases.IsEmpty()) {
    for (uint32_t count = mMaybeBlockedDatabases.Length(), index = 0;
         index < count;
         ) {
      if (mMaybeBlockedDatabases[index]->SendVersionChange(aOldVersion,
                                                           aNewVersion)) {
        index++;
      } else {
        
        
        mMaybeBlockedDatabases.RemoveElementAt(index);
        count--;
      }
    }
  }

  return NS_OK;
}


bool
FactoryOp::CheckAtLeastOneAppHasPermission(ContentParent* aContentParent,
                                           const nsACString& aPermissionString)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aContentParent);
  MOZ_ASSERT(!aPermissionString.IsEmpty());

#ifdef MOZ_CHILD_PERMISSIONS
  const nsTArray<PBrowserParent*>& browsers =
    aContentParent->ManagedPBrowserParent();

  if (!browsers.IsEmpty()) {
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService(APPS_SERVICE_CONTRACTID);
    if (NS_WARN_IF(!appsService)) {
      return false;
    }

    nsCOMPtr<nsIIOService> ioService = do_GetIOService();
    if (NS_WARN_IF(!ioService)) {
      return false;
    }

    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    if (NS_WARN_IF(!secMan)) {
      return false;
    }

    nsCOMPtr<nsIPermissionManager> permMan =
      mozilla::services::GetPermissionManager();
    if (NS_WARN_IF(!permMan)) {
      return false;
    }

    const nsPromiseFlatCString permissionString =
      PromiseFlatCString(aPermissionString);

    for (uint32_t index = 0, count = browsers.Length();
         index < count;
         index++) {
      uint32_t appId =
        TabParent::GetFrom(browsers[index])->OwnOrContainingAppId();
      MOZ_ASSERT(kUnknownAppId != appId && kNoAppId != appId);

      nsCOMPtr<mozIApplication> app;
      nsresult rv = appsService->GetAppByLocalId(appId, getter_AddRefs(app));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      nsString origin;
      rv = app->GetOrigin(origin);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      nsCOMPtr<nsIURI> uri;
      rv = NS_NewURI(getter_AddRefs(uri), origin, nullptr, nullptr, ioService);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      nsCOMPtr<nsIPrincipal> principal;
      rv = secMan->GetAppCodebasePrincipal(uri, appId, false,
                                           getter_AddRefs(principal));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      uint32_t permission;
      rv = permMan->TestExactPermissionFromPrincipal(principal,
                                                     permissionString.get(),
                                                     &permission);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      if (permission == nsIPermissionManager::ALLOW_ACTION) {
        return true;
      }
    }
  }

  return false;
#else
  return true;
#endif 
}

nsresult
FactoryOp::FinishOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_Initial || mState == State_PermissionRetry);
  MOZ_ASSERT(!mOrigin.IsEmpty());
  MOZ_ASSERT(!mDatabaseId.IsEmpty());
  MOZ_ASSERT(!mDirectoryLock);
  MOZ_ASSERT(!mContentParent);
  MOZ_ASSERT(!QuotaClient::IsShuttingDownOnMainThread());

  QuotaManager* quotaManager = QuotaManager::GetOrCreate();
  if (NS_WARN_IF(!quotaManager)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mState = State_DirectoryOpenPending;

  quotaManager->OpenDirectory(mCommonParams.metadata().persistenceType(),
                              mGroup,
                              mOrigin,
                              mIsApp,
                              Client::IDB,
                               false,
                              this);

  return NS_OK;
}

bool
FactoryOp::MustWaitFor(const FactoryOp& aExistingOp)
{
  AssertIsOnOwningThread();

  
  
  return aExistingOp.mCommonParams.metadata().persistenceType() ==
           mCommonParams.metadata().persistenceType() &&
         aExistingOp.mOrigin == mOrigin &&
         aExistingOp.mDatabaseId == mDatabaseId;
}

void
FactoryOp::NoteDatabaseBlocked(Database* aDatabase)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForOtherDatabasesToClose);
  MOZ_ASSERT(!mMaybeBlockedDatabases.IsEmpty());
  MOZ_ASSERT(mMaybeBlockedDatabases.Contains(aDatabase));

  
  
  
  bool sendBlockedEvent = true;

  for (uint32_t count = mMaybeBlockedDatabases.Length(), index = 0;
       index < count;
       index++) {
    MaybeBlockedDatabaseInfo& info = mMaybeBlockedDatabases[index];
    if (info == aDatabase) {
      
      info.mBlocked = true;
    } else if (!info.mBlocked) {
      
      sendBlockedEvent = false;
    }
  }

  if (sendBlockedEvent) {
    SendBlockedNotification();
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(FactoryOp, DatabaseOperationBase)

NS_IMETHODIMP
FactoryOp::Run()
{
  nsresult rv;

  switch (mState) {
    case State_Initial:
      rv = Open();
      break;

    case State_PermissionChallenge:
      rv = ChallengePermission();
      break;

    case State_PermissionRetry:
      rv = RetryCheckPermission();
      break;

    case State_DirectoryWorkOpen:
      rv = DirectoryOpen();
      break;

    case State_DatabaseOpenPending:
      rv = DatabaseOpen();
      break;

    case State_DatabaseWorkOpen:
      rv = DoDatabaseWork();
      break;

    case State_BeginVersionChange:
      rv = BeginVersionChange();
      break;

    case State_WaitingForTransactionsToComplete:
      rv = DispatchToWorkThread();
      break;

    case State_SendingResults:
      SendResults();
      return NS_OK;

    default:
      MOZ_CRASH("Bad state!");
  }

  if (NS_WARN_IF(NS_FAILED(rv)) && mState != State_SendingResults) {
    if (NS_SUCCEEDED(mResultCode)) {
      mResultCode = rv;
    }

    
    
    mState = State_SendingResults;

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                         NS_DISPATCH_NORMAL)));
  }

  return NS_OK;
}

void
FactoryOp::DirectoryLockAcquired(DirectoryLock* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DirectoryOpenPending);
  MOZ_ASSERT(!mDirectoryLock);

  mDirectoryLock = aLock;

  mState = State_DirectoryWorkOpen;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                       NS_DISPATCH_NORMAL)));
}

void
FactoryOp::DirectoryLockFailed()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DirectoryOpenPending);
  MOZ_ASSERT(!mDirectoryLock);

  if (NS_SUCCEEDED(mResultCode)) {
    IDB_REPORT_INTERNAL_ERR();
    mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mState = State_SendingResults;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                       NS_DISPATCH_NORMAL)));
}

void
FactoryOp::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();

  NoteActorDestroyed();
}

bool
FactoryOp::RecvPermissionRetry()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!IsActorDestroyed());
  MOZ_ASSERT(mState == State_PermissionChallenge);

  mContentParent = BackgroundParent::GetContentParent(Manager()->Manager());

  mState = State_PermissionRetry;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));

  return true;
}

OpenDatabaseOp::OpenDatabaseOp(Factory* aFactory,
                               already_AddRefed<ContentParent> aContentParent,
                               const CommonFactoryRequestParams& aParams)
  : FactoryOp(aFactory, Move(aContentParent), aParams,  false)
  , mMetadata(new FullDatabaseMetadata(aParams.metadata()))
  , mRequestedVersion(aParams.metadata().version())
  , mVersionChangeOp(nullptr)
  , mTelemetryId(0)
{
  auto& optionalContentParentId =
    const_cast<OptionalContentId&>(mOptionalContentParentId);

  if (mContentParent) {
    
    
    optionalContentParentId = mContentParent->ChildID();
  } else {
    optionalContentParentId = void_t();
  }
}

void
OpenDatabaseOp::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnOwningThread();

  FactoryOp::ActorDestroy(aWhy);

  if (mVersionChangeOp) {
    mVersionChangeOp->NoteActorDestroyed();
  }
}

nsresult
OpenDatabaseOp::DatabaseOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DatabaseOpenPending);

  nsresult rv = SendToIOThread();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
OpenDatabaseOp::DoDatabaseWork()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == State_DatabaseWorkOpen);

  PROFILER_LABEL("IndexedDB",
                 "OpenDatabaseOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  const nsString& databaseName = mCommonParams.metadata().name();
  PersistenceType persistenceType = mCommonParams.metadata().persistenceType();

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  nsCOMPtr<nsIFile> dbDirectory;

  nsresult rv =
    quotaManager->EnsureOriginIsInitialized(persistenceType,
                                            mGroup,
                                            mOrigin,
                                            mIsApp,
                                            getter_AddRefs(dbDirectory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = dbDirectory->Append(NS_LITERAL_STRING(IDB_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = dbDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    rv = dbDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
#ifdef DEBUG
  else {
    bool isDirectory;
    MOZ_ASSERT(NS_SUCCEEDED(dbDirectory->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory);
  }
#endif

  nsAutoString filename;
  GetDatabaseFilename(databaseName, filename);

  nsCOMPtr<nsIFile> dbFile;
  rv = dbDirectory->Clone(getter_AddRefs(dbFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = dbFile->Append(filename + NS_LITERAL_STRING(".sqlite"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mTelemetryId = TelemetryIdForFile(dbFile);

  rv = dbFile->GetPath(mDatabaseFilePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIFile> fmDirectory;
  rv = dbDirectory->Clone(getter_AddRefs(fmDirectory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  const NS_ConvertASCIItoUTF16 filesSuffix(kFileManagerDirectoryNameSuffix);

  rv = fmDirectory->Append(filename + filesSuffix);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<mozIStorageConnection> connection;
  rv = CreateStorageConnection(dbFile,
                               fmDirectory,
                               databaseName,
                               persistenceType,
                               mGroup,
                               mOrigin,
                               mTelemetryId,
                               getter_AddRefs(connection));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  AutoSetProgressHandler asph;
  rv = asph.Register(connection, this);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = LoadDatabaseInformation(connection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(mMetadata->mNextObjectStoreId > mMetadata->mObjectStores.Count());
  MOZ_ASSERT(mMetadata->mNextIndexId > 0);

  

  
  if (!mRequestedVersion) {
    
    
    if (mMetadata->mCommonMetadata.version() == 0) {
      mRequestedVersion = 1;
    } else {
      
      mRequestedVersion = mMetadata->mCommonMetadata.version();
    }
  }

  if (NS_WARN_IF(mMetadata->mCommonMetadata.version() > mRequestedVersion)) {
    return NS_ERROR_DOM_INDEXEDDB_VERSION_ERR;
  }

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  MOZ_ASSERT(mgr);

  nsRefPtr<FileManager> fileManager =
    mgr->GetFileManager(persistenceType, mOrigin, databaseName);
  if (!fileManager) {
    fileManager = new FileManager(persistenceType,
                                  mGroup,
                                  mOrigin,
                                  mIsApp,
                                  databaseName,
                                  mEnforcingQuota);

    rv = fileManager->Init(fmDirectory, connection);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    mgr->AddFileManager(fileManager);
  }

  mFileManager = fileManager.forget();

  
  
  mState = (mMetadata->mCommonMetadata.version() == mRequestedVersion) ?
           State_SendingResults :
           State_BeginVersionChange;

  rv = mOwningThread->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
OpenDatabaseOp::LoadDatabaseInformation(mozIStorageConnection* aConnection)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(mMetadata);

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT name, version "
    "FROM database"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!hasResult)) {
    return NS_ERROR_FILE_CORRUPTED;
  }

  nsString databaseName;
  rv = stmt->GetString(0, databaseName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(mCommonParams.metadata().name() != databaseName)) {
    return NS_ERROR_FILE_CORRUPTED;
  }

  int64_t version;
  rv = stmt->GetInt64(1, &version);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mMetadata->mCommonMetadata.version() = uint64_t(version);

  ObjectStoreTable& objectStores = mMetadata->mObjectStores;

  
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, auto_increment, name, key_path "
    "FROM object_store"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  Maybe<nsTHashtable<nsUint64HashKey>> usedIds;
  Maybe<nsTHashtable<nsStringHashKey>> usedNames;

  int64_t lastObjectStoreId = 0;

  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    int64_t objectStoreId;
    rv = stmt->GetInt64(0, &objectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!usedIds) {
      usedIds.emplace();
    }

    if (NS_WARN_IF(objectStoreId <= 0) ||
        NS_WARN_IF(usedIds.ref().Contains(objectStoreId))) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    if (NS_WARN_IF(!usedIds.ref().PutEntry(objectStoreId, fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsString name;
    rv = stmt->GetString(2, name);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!usedNames) {
      usedNames.emplace();
    }

    if (NS_WARN_IF(usedNames.ref().Contains(name))) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    if (NS_WARN_IF(!usedNames.ref().PutEntry(name, fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsRefPtr<FullObjectStoreMetadata> metadata = new FullObjectStoreMetadata();
    metadata->mCommonMetadata.id() = objectStoreId;
    metadata->mCommonMetadata.name() = name;

    int32_t columnType;
    rv = stmt->GetTypeOfIndex(3, &columnType);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (columnType == mozIStorageStatement::VALUE_TYPE_NULL) {
      metadata->mCommonMetadata.keyPath() = KeyPath(0);
    } else {
      MOZ_ASSERT(columnType == mozIStorageStatement::VALUE_TYPE_TEXT);

      nsString keyPathSerialization;
      rv = stmt->GetString(3, keyPathSerialization);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      metadata->mCommonMetadata.keyPath() =
        KeyPath::DeserializeFromString(keyPathSerialization);
      if (NS_WARN_IF(!metadata->mCommonMetadata.keyPath().IsValid())) {
        return NS_ERROR_FILE_CORRUPTED;
      }
    }

    int64_t nextAutoIncrementId;
    rv = stmt->GetInt64(1, &nextAutoIncrementId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    metadata->mCommonMetadata.autoIncrement() = !!nextAutoIncrementId;
    metadata->mNextAutoIncrementId = nextAutoIncrementId;
    metadata->mComittedAutoIncrementId = nextAutoIncrementId;

    if (NS_WARN_IF(!objectStores.Put(objectStoreId, metadata, fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    lastObjectStoreId = std::max(lastObjectStoreId, objectStoreId);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  usedIds.reset();
  usedNames.reset();

  
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, object_store_id, name, key_path, unique_index, multientry "
    "FROM object_store_index"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int64_t lastIndexId = 0;

  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    int64_t objectStoreId;
    rv = stmt->GetInt64(1, &objectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata;
    if (NS_WARN_IF(!objectStores.Get(objectStoreId,
                                     getter_AddRefs(objectStoreMetadata)))) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    MOZ_ASSERT(objectStoreMetadata->mCommonMetadata.id() == objectStoreId);

    int64_t indexId;
    rv = stmt->GetInt64(0, &indexId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!usedIds) {
      usedIds.emplace();
    }

    if (NS_WARN_IF(indexId <= 0) ||
        NS_WARN_IF(usedIds.ref().Contains(indexId))) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    if (NS_WARN_IF(!usedIds.ref().PutEntry(indexId, fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsString name;
    rv = stmt->GetString(2, name);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsAutoString hashName;
    hashName.AppendInt(indexId);
    hashName.Append(':');
    hashName.Append(name);

    if (!usedNames) {
      usedNames.emplace();
    }

    if (NS_WARN_IF(usedNames.ref().Contains(hashName))) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    if (NS_WARN_IF(!usedNames.ref().PutEntry(hashName, fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsRefPtr<FullIndexMetadata> indexMetadata = new FullIndexMetadata();
    indexMetadata->mCommonMetadata.id() = indexId;
    indexMetadata->mCommonMetadata.name() = name;

#ifdef DEBUG
    {
      int32_t columnType;
      rv = stmt->GetTypeOfIndex(3, &columnType);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      MOZ_ASSERT(columnType != mozIStorageStatement::VALUE_TYPE_NULL);
    }
#endif

    nsString keyPathSerialization;
    rv = stmt->GetString(3, keyPathSerialization);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    indexMetadata->mCommonMetadata.keyPath() =
      KeyPath::DeserializeFromString(keyPathSerialization);
    if (NS_WARN_IF(!indexMetadata->mCommonMetadata.keyPath().IsValid())) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    int32_t scratch;
    rv = stmt->GetInt32(4, &scratch);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    indexMetadata->mCommonMetadata.unique() = !!scratch;

    rv = stmt->GetInt32(5, &scratch);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    indexMetadata->mCommonMetadata.multiEntry() = !!scratch;

    if (NS_WARN_IF(!objectStoreMetadata->mIndexes.Put(indexId, indexMetadata,
                                                      fallible))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    lastIndexId = std::max(lastIndexId, indexId);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(lastObjectStoreId == INT64_MAX) ||
      NS_WARN_IF(lastIndexId == INT64_MAX)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mMetadata->mNextObjectStoreId = lastObjectStoreId + 1;
  mMetadata->mNextIndexId = lastIndexId + 1;

  return NS_OK;
}

nsresult
OpenDatabaseOp::BeginVersionChange()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_BeginVersionChange);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());
  MOZ_ASSERT(mMetadata->mCommonMetadata.version() <= mRequestedVersion);
  MOZ_ASSERT(!mDatabase);
  MOZ_ASSERT(!mVersionChangeTransaction);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  EnsureDatabaseActor();

  if (mDatabase->IsInvalidated()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  MOZ_ASSERT(!mDatabase->IsClosed());

  DatabaseActorInfo* info;
  MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(mDatabaseId, &info));

  MOZ_ASSERT(info->mLiveDatabases.Contains(mDatabase));
  MOZ_ASSERT(!info->mWaitingFactoryOp);
  MOZ_ASSERT(info->mMetadata == mMetadata);

  nsRefPtr<VersionChangeTransaction> transaction =
    new VersionChangeTransaction(this);

  if (NS_WARN_IF(!transaction->CopyDatabaseMetadata())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  MOZ_ASSERT(info->mMetadata != mMetadata);
  mMetadata = info->mMetadata;

  NullableVersion newVersion = mRequestedVersion;

  nsresult rv =
    SendVersionChangeMessages(info,
                              mDatabase,
                              mMetadata->mCommonMetadata.version(),
                              newVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mVersionChangeTransaction.swap(transaction);

  if (mMaybeBlockedDatabases.IsEmpty()) {
    
    
    WaitForTransactions();
    return NS_OK;
  }

  info->mWaitingFactoryOp = this;

  mState = State_WaitingForOtherDatabasesToClose;
  return NS_OK;
}

void
OpenDatabaseOp::NoteDatabaseClosed(Database* aDatabase)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aDatabase);
  MOZ_ASSERT(mState == State_WaitingForOtherDatabasesToClose ||
             mState == State_DatabaseWorkVersionChange);

  if (mState == State_DatabaseWorkVersionChange) {
    MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());
    MOZ_ASSERT(mRequestedVersion >
                 aDatabase->Metadata()->mCommonMetadata.version(),
               "Must only be closing databases for a previous version!");
    return;
  }

  MOZ_ASSERT(!mMaybeBlockedDatabases.IsEmpty());

  bool actorDestroyed = IsActorDestroyed() || mDatabase->IsActorDestroyed();

  nsresult rv;
  if (actorDestroyed) {
    IDB_REPORT_INTERNAL_ERR();
    rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  } else {
    rv = NS_OK;
  }

  if (mMaybeBlockedDatabases.RemoveElement(aDatabase) &&
      mMaybeBlockedDatabases.IsEmpty()) {
    if (actorDestroyed) {
      DatabaseActorInfo* info;
      MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(mDatabaseId, &info));
      MOZ_ASSERT(info->mWaitingFactoryOp == this);
      info->mWaitingFactoryOp = nullptr;
    } else {
      WaitForTransactions();
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    if (NS_SUCCEEDED(mResultCode)) {
      mResultCode = rv;
    }

    mState = State_SendingResults;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(Run()));
  }
}

void
OpenDatabaseOp::SendBlockedNotification()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForOtherDatabasesToClose);

  if (!IsActorDestroyed()) {
    unused << SendBlocked(mMetadata->mCommonMetadata.version());
  }
}

nsresult
OpenDatabaseOp::DispatchToWorkThread()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForTransactionsToComplete);
  MOZ_ASSERT(mVersionChangeTransaction);
  MOZ_ASSERT(mVersionChangeTransaction->GetMode() ==
               IDBTransaction::VERSION_CHANGE);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mState = State_DatabaseWorkVersionChange;

  
  nsTArray<nsString> objectStoreNames;

  const int64_t loggingSerialNumber =
    mVersionChangeTransaction->LoggingSerialNumber();
  const nsID& backgroundChildLoggingId =
    mVersionChangeTransaction->GetLoggingInfo()->Id();

  if (NS_WARN_IF(!mDatabase->RegisterTransaction(mVersionChangeTransaction))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!gConnectionPool) {
    gConnectionPool = new ConnectionPool();
  }

  nsRefPtr<VersionChangeOp> versionChangeOp = new VersionChangeOp(this);

  uint64_t transactionId =
    gConnectionPool->Start(backgroundChildLoggingId,
                           mVersionChangeTransaction->DatabaseId(),
                           loggingSerialNumber,
                           objectStoreNames,
                            true,
                           versionChangeOp);

  mVersionChangeOp = versionChangeOp;

  mVersionChangeTransaction->NoteActiveRequest();
  mVersionChangeTransaction->SetActive(transactionId);

  return NS_OK;
}

nsresult
OpenDatabaseOp::SendUpgradeNeeded()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_DatabaseWorkVersionChange);
  MOZ_ASSERT(mVersionChangeTransaction);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());
  MOZ_ASSERT(NS_SUCCEEDED(mResultCode));
  MOZ_ASSERT_IF(!IsActorDestroyed(), mDatabase);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  nsRefPtr<VersionChangeTransaction> transaction;
  mVersionChangeTransaction.swap(transaction);

  nsresult rv = EnsureDatabaseActorIsAlive();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  transaction->SetActorAlive();

  if (!mDatabase->SendPBackgroundIDBVersionChangeTransactionConstructor(
                                           transaction,
                                           mMetadata->mCommonMetadata.version(),
                                           mRequestedVersion,
                                           mMetadata->mNextObjectStoreId,
                                           mMetadata->mNextIndexId)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  return NS_OK;
}

void
OpenDatabaseOp::SendResults()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_SendingResults);
  MOZ_ASSERT_IF(NS_SUCCEEDED(mResultCode), mMaybeBlockedDatabases.IsEmpty());
  MOZ_ASSERT_IF(NS_SUCCEEDED(mResultCode), !mVersionChangeTransaction);

  mMaybeBlockedDatabases.Clear();

  
  
  nsRefPtr<OpenDatabaseOp> kungFuDeathGrip;

  DatabaseActorInfo* info;
  if (gLiveDatabaseHashtable &&
      gLiveDatabaseHashtable->Get(mDatabaseId, &info) &&
      info->mWaitingFactoryOp) {
    MOZ_ASSERT(info->mWaitingFactoryOp == this);
    kungFuDeathGrip =
      static_cast<OpenDatabaseOp*>(info->mWaitingFactoryOp.get());
    info->mWaitingFactoryOp = nullptr;
  }

  if (mVersionChangeTransaction) {
    MOZ_ASSERT(NS_FAILED(mResultCode));

    mVersionChangeTransaction->Abort(mResultCode,  true);
    mVersionChangeTransaction = nullptr;
  }

  if (IsActorDestroyed()) {
    if (NS_SUCCEEDED(mResultCode)) {
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  } else {
    FactoryRequestResponse response;

    if (NS_SUCCEEDED(mResultCode)) {
      
      
      mMetadata->mCommonMetadata.version() = mRequestedVersion;

      nsresult rv = EnsureDatabaseActorIsAlive();
      if (NS_SUCCEEDED(rv)) {
        
        
        OpenDatabaseRequestResponse openResponse;
        openResponse.databaseParent() = mDatabase;
        response = openResponse;
      } else {
        response = ClampResultCode(rv);
#ifdef DEBUG
        mResultCode = response.get_nsresult();
#endif
      }
    } else {
#ifdef DEBUG
      
      
      
      mMetadata = nullptr;
#endif
      response = ClampResultCode(mResultCode);
    }

    unused <<
      PBackgroundIDBFactoryRequestParent::Send__delete__(this, response);
  }

  if (mDatabase) {
    MOZ_ASSERT(!mDirectoryLock);

    if (NS_FAILED(mResultCode)) {
      mDatabase->Invalidate();
    }

    
    mDatabase = nullptr;
  } else if (mDirectoryLock) {
    nsCOMPtr<nsIRunnable> callback =
      NS_NewRunnableMethod(this, &OpenDatabaseOp::ConnectionClosedCallback);

    nsRefPtr<WaitForTransactionsHelper> helper =
      new WaitForTransactionsHelper(mDatabaseId, callback);
    helper->WaitForTransactions();
  }

  FinishSendResults();
}

void
OpenDatabaseOp::ConnectionClosedCallback()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(mResultCode));
  MOZ_ASSERT(mDirectoryLock);

  nsRefPtr<UnlockDirectoryRunnable> runnable =
    new UnlockDirectoryRunnable(mDirectoryLock.forget());

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
}

void
OpenDatabaseOp::EnsureDatabaseActor()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_BeginVersionChange ||
             mState == State_DatabaseWorkVersionChange ||
             mState == State_SendingResults);
  MOZ_ASSERT(NS_SUCCEEDED(mResultCode));
  MOZ_ASSERT(!mDatabaseFilePath.IsEmpty());
  MOZ_ASSERT(!IsActorDestroyed());

  if (mDatabase) {
    return;
  }

  MOZ_ASSERT(mMetadata->mDatabaseId.IsEmpty());
  mMetadata->mDatabaseId = mDatabaseId;

  MOZ_ASSERT(mMetadata->mFilePath.IsEmpty());
  mMetadata->mFilePath = mDatabaseFilePath;

  DatabaseActorInfo* info;
  if (gLiveDatabaseHashtable->Get(mDatabaseId, &info)) {
    AssertMetadataConsistency(info->mMetadata);
    mMetadata = info->mMetadata;
  }

  auto factory = static_cast<Factory*>(Manager());

  mDatabase = new Database(factory,
                           mCommonParams.principalInfo(),
                           mOptionalContentParentId,
                           mGroup,
                           mOrigin,
                           mTelemetryId,
                           mMetadata,
                           mFileManager,
                           mDirectoryLock.forget(),
                           mChromeWriteAccessAllowed);

  if (info) {
    info->mLiveDatabases.AppendElement(mDatabase);
  } else {
    info = new DatabaseActorInfo(mMetadata, mDatabase);
    gLiveDatabaseHashtable->Put(mDatabaseId, info);
  }
}

nsresult
OpenDatabaseOp::EnsureDatabaseActorIsAlive()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_DatabaseWorkVersionChange ||
             mState == State_SendingResults);
  MOZ_ASSERT(NS_SUCCEEDED(mResultCode));
  MOZ_ASSERT(!IsActorDestroyed());

  EnsureDatabaseActor();

  if (mDatabase->IsActorAlive()) {
    return NS_OK;
  }

  auto factory = static_cast<Factory*>(Manager());

  DatabaseSpec spec;
  MetadataToSpec(spec);

  
  mDatabase->SetActorAlive();

  if (!factory->SendPBackgroundIDBDatabaseConstructor(mDatabase, spec, this)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  return NS_OK;
}

void
OpenDatabaseOp::MetadataToSpec(DatabaseSpec& aSpec)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mMetadata);

  class MOZ_STACK_CLASS Helper final
  {
    DatabaseSpec& mSpec;
    ObjectStoreSpec* mCurrentObjectStoreSpec;

  public:
    static void
    CopyToSpec(const FullDatabaseMetadata* aMetadata, DatabaseSpec& aSpec)
    {
      AssertIsOnBackgroundThread();
      MOZ_ASSERT(aMetadata);

      aSpec.metadata() = aMetadata->mCommonMetadata;

      Helper helper(aSpec);
      aMetadata->mObjectStores.EnumerateRead(Enumerate, &helper);
    }

  private:
    explicit Helper(DatabaseSpec& aSpec)
      : mSpec(aSpec)
      , mCurrentObjectStoreSpec(nullptr)
    { }

    static PLDHashOperator
    Enumerate(const uint64_t& aKey,
              FullObjectStoreMetadata* aValue,
              void* aClosure)
    {
      MOZ_ASSERT(aKey);
      MOZ_ASSERT(aValue);
      MOZ_ASSERT(aClosure);

      auto* helper = static_cast<Helper*>(aClosure);

      MOZ_ASSERT(!helper->mCurrentObjectStoreSpec);

      
      ObjectStoreSpec* objectStoreSpec =
        helper->mSpec.objectStores().AppendElement();
      objectStoreSpec->metadata() = aValue->mCommonMetadata;

      AutoRestore<ObjectStoreSpec*> ar(helper->mCurrentObjectStoreSpec);
      helper->mCurrentObjectStoreSpec = objectStoreSpec;

      aValue->mIndexes.EnumerateRead(Enumerate, helper);

      return PL_DHASH_NEXT;
    }

    static PLDHashOperator
    Enumerate(const uint64_t& aKey, FullIndexMetadata* aValue, void* aClosure)
    {
      MOZ_ASSERT(aKey);
      MOZ_ASSERT(aValue);
      MOZ_ASSERT(aClosure);

      auto* helper = static_cast<Helper*>(aClosure);

      MOZ_ASSERT(helper->mCurrentObjectStoreSpec);

      
      IndexMetadata* metadata =
        helper->mCurrentObjectStoreSpec->indexes().AppendElement();
      *metadata = aValue->mCommonMetadata;

      return PL_DHASH_NEXT;
    }
  };

  Helper::CopyToSpec(mMetadata, aSpec);
}


#ifdef DEBUG

void
OpenDatabaseOp::AssertMetadataConsistency(const FullDatabaseMetadata* aMetadata)
{
  AssertIsOnBackgroundThread();

  class MOZ_STACK_CLASS Helper final
  {
    const ObjectStoreTable& mOtherObjectStores;
    IndexTable* mCurrentOtherIndexTable;

  public:
    static void
    AssertConsistent(const ObjectStoreTable& aThisObjectStores,
                     const ObjectStoreTable& aOtherObjectStores)
    {
      Helper helper(aOtherObjectStores);
      aThisObjectStores.EnumerateRead(Enumerate, &helper);
    }

  private:
    explicit Helper(const ObjectStoreTable& aOtherObjectStores)
      : mOtherObjectStores(aOtherObjectStores)
      , mCurrentOtherIndexTable(nullptr)
    { }

    static PLDHashOperator
    Enumerate(const uint64_t& ,
              FullObjectStoreMetadata* aThisObjectStore,
              void* aClosure)
    {
      MOZ_ASSERT(aThisObjectStore);
      MOZ_ASSERT(!aThisObjectStore->mDeleted);
      MOZ_ASSERT(aClosure);

      auto* helper = static_cast<Helper*>(aClosure);

      MOZ_ASSERT(!helper->mCurrentOtherIndexTable);

      auto* otherObjectStore =
        MetadataNameOrIdMatcher<FullObjectStoreMetadata>::Match(
          helper->mOtherObjectStores, aThisObjectStore->mCommonMetadata.id());
      MOZ_ASSERT(otherObjectStore);

      MOZ_ASSERT(aThisObjectStore != otherObjectStore);

      MOZ_ASSERT(aThisObjectStore->mCommonMetadata.id() ==
                   otherObjectStore->mCommonMetadata.id());
      MOZ_ASSERT(aThisObjectStore->mCommonMetadata.name() ==
                   otherObjectStore->mCommonMetadata.name());
      MOZ_ASSERT(aThisObjectStore->mCommonMetadata.autoIncrement() ==
                   otherObjectStore->mCommonMetadata.autoIncrement());
      MOZ_ASSERT(aThisObjectStore->mCommonMetadata.keyPath() ==
                   otherObjectStore->mCommonMetadata.keyPath());
      
      
      
      MOZ_ASSERT(aThisObjectStore->mNextAutoIncrementId <=
                   otherObjectStore->mNextAutoIncrementId);
      MOZ_ASSERT(aThisObjectStore->mComittedAutoIncrementId <=
                   otherObjectStore->mComittedAutoIncrementId);
      MOZ_ASSERT(!otherObjectStore->mDeleted);

      MOZ_ASSERT(aThisObjectStore->mIndexes.Count() ==
                   otherObjectStore->mIndexes.Count());

      AutoRestore<IndexTable*> ar(helper->mCurrentOtherIndexTable);
      helper->mCurrentOtherIndexTable = &otherObjectStore->mIndexes;

      aThisObjectStore->mIndexes.EnumerateRead(Enumerate, helper);

      return PL_DHASH_NEXT;
    }

    static PLDHashOperator
    Enumerate(const uint64_t& ,
              FullIndexMetadata* aThisIndex,
              void* aClosure)
    {
      MOZ_ASSERT(aThisIndex);
      MOZ_ASSERT(!aThisIndex->mDeleted);
      MOZ_ASSERT(aClosure);

      auto* helper = static_cast<Helper*>(aClosure);

      MOZ_ASSERT(helper->mCurrentOtherIndexTable);

      auto* otherIndex =
        MetadataNameOrIdMatcher<FullIndexMetadata>::Match(
          *helper->mCurrentOtherIndexTable, aThisIndex->mCommonMetadata.id());
      MOZ_ASSERT(otherIndex);

      MOZ_ASSERT(aThisIndex != otherIndex);

      MOZ_ASSERT(aThisIndex->mCommonMetadata.id() ==
                   otherIndex->mCommonMetadata.id());
      MOZ_ASSERT(aThisIndex->mCommonMetadata.name() ==
                   otherIndex->mCommonMetadata.name());
      MOZ_ASSERT(aThisIndex->mCommonMetadata.keyPath() ==
                   otherIndex->mCommonMetadata.keyPath());
      MOZ_ASSERT(aThisIndex->mCommonMetadata.unique() ==
                   otherIndex->mCommonMetadata.unique());
      MOZ_ASSERT(aThisIndex->mCommonMetadata.multiEntry() ==
                   otherIndex->mCommonMetadata.multiEntry());
      MOZ_ASSERT(!otherIndex->mDeleted);

      return PL_DHASH_NEXT;
    }
  };

  const FullDatabaseMetadata* thisDB = mMetadata;
  const FullDatabaseMetadata* otherDB = aMetadata;

  MOZ_ASSERT(thisDB);
  MOZ_ASSERT(otherDB);
  MOZ_ASSERT(thisDB != otherDB);

  MOZ_ASSERT(thisDB->mCommonMetadata.name() == otherDB->mCommonMetadata.name());
  MOZ_ASSERT(thisDB->mCommonMetadata.version() ==
               otherDB->mCommonMetadata.version());
  MOZ_ASSERT(thisDB->mCommonMetadata.persistenceType() ==
               otherDB->mCommonMetadata.persistenceType());
  MOZ_ASSERT(thisDB->mDatabaseId == otherDB->mDatabaseId);
  MOZ_ASSERT(thisDB->mFilePath == otherDB->mFilePath);

  
  
  
  
  MOZ_ASSERT(thisDB->mNextObjectStoreId <= otherDB->mNextObjectStoreId);
  MOZ_ASSERT(thisDB->mNextIndexId <= otherDB->mNextIndexId);

  MOZ_ASSERT(thisDB->mObjectStores.Count() == otherDB->mObjectStores.Count());

  Helper::AssertConsistent(thisDB->mObjectStores, otherDB->mObjectStores);
}

#endif 

nsresult
OpenDatabaseOp::
VersionChangeOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT(mOpenDatabaseOp->mState == State_DatabaseWorkVersionChange);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  PROFILER_LABEL("IndexedDB",
                 "OpenDatabaseOp::VersionChangeOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld]: "
                 "Beginning database work",
               "IndexedDB %s: P T[%lld]: DB Start",
               IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
               mLoggingSerialNumber);

  Transaction()->SetActiveOnConnectionThread();

  nsresult rv = aConnection->BeginWriteTransaction();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DatabaseConnection::CachedStatement updateStmt;
  rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "UPDATE database "
      "SET version = :version;"),
    &updateStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("version"),
                                   int64_t(mRequestedVersion));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = updateStmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
OpenDatabaseOp::
VersionChangeOp::SendSuccessResult()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT(mOpenDatabaseOp->mState == State_DatabaseWorkVersionChange);
  MOZ_ASSERT(mOpenDatabaseOp->mVersionChangeOp == this);

  nsresult rv = mOpenDatabaseOp->SendUpgradeNeeded();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

bool
OpenDatabaseOp::
VersionChangeOp::SendFailureResult(nsresult aResultCode)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT(mOpenDatabaseOp->mState == State_DatabaseWorkVersionChange);
  MOZ_ASSERT(mOpenDatabaseOp->mVersionChangeOp == this);

  mOpenDatabaseOp->SetFailureCode(aResultCode);
  mOpenDatabaseOp->mState = State_SendingResults;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOpenDatabaseOp->Run()));

  return false;
}

void
OpenDatabaseOp::
VersionChangeOp::Cleanup()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mOpenDatabaseOp);
  MOZ_ASSERT(mOpenDatabaseOp->mVersionChangeOp == this);

  mOpenDatabaseOp->mVersionChangeOp = nullptr;
  mOpenDatabaseOp = nullptr;

#ifdef DEBUG
  
  
  
  NoteActorDestroyed();
#endif

  TransactionDatabaseOperationBase::Cleanup();
}

void
DeleteDatabaseOp::LoadPreviousVersion(nsIFile* aDatabaseFile)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDatabaseFile);
  MOZ_ASSERT(mState == State_DatabaseWorkOpen);
  MOZ_ASSERT(!mPreviousVersion);

  PROFILER_LABEL("IndexedDB",
                 "DeleteDatabaseOp::LoadPreviousVersion",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

  nsCOMPtr<mozIStorageService> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  nsCOMPtr<mozIStorageConnection> connection;
  rv = OpenDatabaseAndHandleBusy(ss, aDatabaseFile, getter_AddRefs(connection));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

#ifdef DEBUG
  {
    nsCOMPtr<mozIStorageStatement> stmt;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      connection->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT name "
          "FROM database"
        ), getter_AddRefs(stmt))));

    bool hasResult;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));

    nsString databaseName;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->GetString(0, databaseName)));

    MOZ_ASSERT(mCommonParams.metadata().name() == databaseName);
  }
#endif

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = connection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT version "
    "FROM database"
  ), getter_AddRefs(stmt));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  if (NS_WARN_IF(!hasResult)) {
    return;
  }

  int64_t version;
  rv = stmt->GetInt64(0, &version);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  mPreviousVersion = uint64_t(version);
}

nsresult
DeleteDatabaseOp::DatabaseOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DatabaseOpenPending);

  
  nsRefPtr<ContentParent> contentParent;
  mContentParent.swap(contentParent);

  nsresult rv = SendToIOThread();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
DeleteDatabaseOp::DoDatabaseWork()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == State_DatabaseWorkOpen);

  PROFILER_LABEL("IndexedDB",
                 "DeleteDatabaseOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  const nsString& databaseName = mCommonParams.metadata().name();
  PersistenceType persistenceType = mCommonParams.metadata().persistenceType();

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  nsCOMPtr<nsIFile> directory;
  nsresult rv = quotaManager->GetDirectoryForOrigin(persistenceType,
                                                    mOrigin,
                                                    getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = directory->Append(NS_LITERAL_STRING(IDB_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = directory->GetPath(mDatabaseDirectoryPath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoString filename;
  GetDatabaseFilename(databaseName, filename);

  mDatabaseFilenameBase = filename;

  nsCOMPtr<nsIFile> dbFile;
  rv = directory->Clone(getter_AddRefs(dbFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = dbFile->Append(filename + NS_LITERAL_STRING(".sqlite"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = dbFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    
    
    LoadPreviousVersion(dbFile);

    mState = State_BeginVersionChange;
  } else {
    mState = State_SendingResults;
  }

  rv = mOwningThread->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
DeleteDatabaseOp::BeginVersionChange()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_BeginVersionChange);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  DatabaseActorInfo* info;
  if (gLiveDatabaseHashtable->Get(mDatabaseId, &info)) {
    MOZ_ASSERT(!info->mWaitingFactoryOp);

    NullableVersion newVersion = null_t();

    nsresult rv =
      SendVersionChangeMessages(info, nullptr, mPreviousVersion, newVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!mMaybeBlockedDatabases.IsEmpty()) {
      info->mWaitingFactoryOp = this;

      mState = State_WaitingForOtherDatabasesToClose;
      return NS_OK;
    }
  }

  
  
  WaitForTransactions();
  return NS_OK;
}

nsresult
DeleteDatabaseOp::DispatchToWorkThread()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForTransactionsToComplete);
  MOZ_ASSERT(mMaybeBlockedDatabases.IsEmpty());

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mState = State_DatabaseWorkVersionChange;

  nsRefPtr<VersionChangeOp> versionChangeOp = new VersionChangeOp(this);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(versionChangeOp)));

  return NS_OK;
}

void
DeleteDatabaseOp::NoteDatabaseClosed(Database* aDatabase)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForOtherDatabasesToClose);
  MOZ_ASSERT(!mMaybeBlockedDatabases.IsEmpty());

  bool actorDestroyed = IsActorDestroyed();

  nsresult rv;
  if (actorDestroyed) {
    IDB_REPORT_INTERNAL_ERR();
    rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  } else {
    rv = NS_OK;
  }

  if (mMaybeBlockedDatabases.RemoveElement(aDatabase) &&
      mMaybeBlockedDatabases.IsEmpty()) {
    if (actorDestroyed) {
      DatabaseActorInfo* info;
      MOZ_ALWAYS_TRUE(gLiveDatabaseHashtable->Get(mDatabaseId, &info));
      MOZ_ASSERT(info->mWaitingFactoryOp == this);
      info->mWaitingFactoryOp = nullptr;
    } else {
      WaitForTransactions();
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    if (NS_SUCCEEDED(mResultCode)) {
      mResultCode = rv;
    }

    mState = State_SendingResults;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(Run()));
  }
}

void
DeleteDatabaseOp::SendBlockedNotification()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_WaitingForOtherDatabasesToClose);

  if (!IsActorDestroyed()) {
    unused << SendBlocked(0);
  }
}

void
DeleteDatabaseOp::SendResults()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mState == State_SendingResults);

  if (!IsActorDestroyed()) {
    FactoryRequestResponse response;

    if (NS_SUCCEEDED(mResultCode)) {
      response = DeleteDatabaseRequestResponse(mPreviousVersion);
    } else {
      response = ClampResultCode(mResultCode);
    }

    unused <<
      PBackgroundIDBFactoryRequestParent::Send__delete__(this, response);
  }

  if (mDirectoryLock) {
    nsRefPtr<UnlockDirectoryRunnable> runnable =
      new UnlockDirectoryRunnable(mDirectoryLock.forget());

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
  }

  FinishSendResults();
}

nsresult
DeleteDatabaseOp::
VersionChangeOp::RunOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mDeleteDatabaseOp->mState == State_DatabaseWorkVersionChange);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  nsresult rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  return NS_OK;
}


nsresult
DeleteDatabaseOp::
VersionChangeOp::DeleteFile(nsIFile* aDirectory,
                            const nsAString& aFilename,
                            QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(!aFilename.IsEmpty());
  MOZ_ASSERT_IF(aQuotaManager, mDeleteDatabaseOp->mEnforcingQuota);

  MOZ_ASSERT(mDeleteDatabaseOp->mState == State_DatabaseWorkVersionChange);

  PROFILER_LABEL("IndexedDB",
                 "DeleteDatabaseOp::VersionChangeOp::DeleteFile",
                 js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<nsIFile> file;
  nsresult rv = aDirectory->Clone(getter_AddRefs(file));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = file->Append(aFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int64_t fileSize;

  if (aQuotaManager) {
    rv = file->GetFileSize(&fileSize);
    if (rv == NS_ERROR_FILE_NOT_FOUND ||
        rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
      return NS_OK;
    }

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(fileSize >= 0);
  }

  rv = file->Remove(false);
  if (rv == NS_ERROR_FILE_NOT_FOUND ||
      rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
    return NS_OK;
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (aQuotaManager && fileSize > 0) {
    const PersistenceType& persistenceType =
      mDeleteDatabaseOp->mCommonParams.metadata().persistenceType();

    aQuotaManager->DecreaseUsageForOrigin(persistenceType,
                                          mDeleteDatabaseOp->mGroup,
                                          mDeleteDatabaseOp->mOrigin,
                                          fileSize);
  }

  return NS_OK;
}

nsresult
DeleteDatabaseOp::
VersionChangeOp::RunOnIOThread()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mDeleteDatabaseOp->mState == State_DatabaseWorkVersionChange);

  PROFILER_LABEL("IndexedDB",
                 "DeleteDatabaseOp::VersionChangeOp::RunOnIOThread",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(QuotaClient::IsShuttingDownOnNonMainThread()) ||
      !OperationMayProceed()) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  const PersistenceType& persistenceType =
    mDeleteDatabaseOp->mCommonParams.metadata().persistenceType();

  QuotaManager* quotaManager =
    mDeleteDatabaseOp->mEnforcingQuota ?
    QuotaManager::Get() :
    nullptr;

  MOZ_ASSERT_IF(mDeleteDatabaseOp->mEnforcingQuota, quotaManager);

  nsCOMPtr<nsIFile> directory =
    GetFileForPath(mDeleteDatabaseOp->mDatabaseDirectoryPath);
  if (NS_WARN_IF(!directory)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  
  nsAutoString filename =
    mDeleteDatabaseOp->mDatabaseFilenameBase + NS_LITERAL_STRING(".sqlite");

  nsresult rv = DeleteFile(directory, filename, quotaManager);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  const NS_ConvertASCIItoUTF16 journalSuffix(
    kSQLiteJournalSuffix,
    LiteralStringLength(kSQLiteJournalSuffix));

  filename = mDeleteDatabaseOp->mDatabaseFilenameBase + journalSuffix;

  rv = DeleteFile(directory, filename,  nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  const NS_ConvertASCIItoUTF16 shmSuffix(kSQLiteSHMSuffix,
                                         LiteralStringLength(kSQLiteSHMSuffix));

  filename = mDeleteDatabaseOp->mDatabaseFilenameBase + shmSuffix;

  rv = DeleteFile(directory, filename,  nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  const NS_ConvertASCIItoUTF16 walSuffix(kSQLiteWALSuffix,
                                         LiteralStringLength(kSQLiteWALSuffix));

  filename = mDeleteDatabaseOp->mDatabaseFilenameBase + walSuffix;

  rv = DeleteFile(directory, filename, quotaManager);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIFile> fmDirectory;
  rv = directory->Clone(getter_AddRefs(fmDirectory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  const NS_ConvertASCIItoUTF16 filesSuffix(
    kFileManagerDirectoryNameSuffix,
    LiteralStringLength(kFileManagerDirectoryNameSuffix));

  rv = fmDirectory->Append(mDeleteDatabaseOp->mDatabaseFilenameBase +
                           filesSuffix);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = fmDirectory->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    bool isDirectory;
    rv = fmDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!isDirectory)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    uint64_t usage = 0;

    if (mDeleteDatabaseOp->mEnforcingQuota) {
      rv = FileManager::GetUsage(fmDirectory, &usage);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = fmDirectory->Remove(true);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      
      if (mDeleteDatabaseOp->mEnforcingQuota) {
        uint64_t newUsage;
        if (NS_SUCCEEDED(FileManager::GetUsage(fmDirectory, &newUsage))) {
          MOZ_ASSERT(newUsage <= usage);
          usage = usage - newUsage;
        }
      }
    }

    if (mDeleteDatabaseOp->mEnforcingQuota && usage) {
      quotaManager->DecreaseUsageForOrigin(persistenceType,
                                           mDeleteDatabaseOp->mGroup,
                                           mDeleteDatabaseOp->mOrigin,
                                           usage);
    }

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  MOZ_ASSERT(mgr);

  const nsString& databaseName =
    mDeleteDatabaseOp->mCommonParams.metadata().name();

  mgr->InvalidateFileManager(persistenceType,
                             mDeleteDatabaseOp->mOrigin,
                             databaseName);

  rv = mOwningThread->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
DeleteDatabaseOp::
VersionChangeOp::RunOnOwningThread()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mDeleteDatabaseOp->mState == State_DatabaseWorkVersionChange);

  nsRefPtr<DeleteDatabaseOp> deleteOp;
  mDeleteDatabaseOp.swap(deleteOp);

  if (deleteOp->IsActorDestroyed()) {
    IDB_REPORT_INTERNAL_ERR();
    deleteOp->SetFailureCode(NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  } else {
    DatabaseActorInfo* info;
    if (gLiveDatabaseHashtable->Get(deleteOp->mDatabaseId, &info) &&
        info->mWaitingFactoryOp) {
      MOZ_ASSERT(info->mWaitingFactoryOp == deleteOp);
      info->mWaitingFactoryOp = nullptr;
    }

    if (NS_FAILED(mResultCode)) {
      if (NS_SUCCEEDED(deleteOp->ResultCode())) {
        deleteOp->SetFailureCode(mResultCode);
      }
    } else {
      
      
      if (info) {
        MOZ_ASSERT(!info->mLiveDatabases.IsEmpty());

        FallibleTArray<Database*> liveDatabases;
        if (NS_WARN_IF(!liveDatabases.AppendElements(info->mLiveDatabases,
                                                     fallible))) {
          deleteOp->SetFailureCode(NS_ERROR_OUT_OF_MEMORY);
        } else {
#ifdef DEBUG
          
          
          info = nullptr;
#endif
          for (uint32_t count = liveDatabases.Length(), index = 0;
               index < count;
               index++) {
            nsRefPtr<Database> database = liveDatabases[index];
            database->Invalidate();
          }

          MOZ_ASSERT(!gLiveDatabaseHashtable->Get(deleteOp->mDatabaseId));
        }
      }
    }
  }

  deleteOp->mState = State_SendingResults;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(deleteOp->Run()));

#ifdef DEBUG
  
  
  
  NoteActorDestroyed();
#endif
}

nsresult
DeleteDatabaseOp::
VersionChangeOp::Run()
{
  nsresult rv;

  if (NS_IsMainThread()) {
    rv = RunOnMainThread();
  } else if (!IsOnBackgroundThread()) {
    rv = RunOnIOThread();
  } else {
    RunOnOwningThread();
    rv = NS_OK;
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    if (NS_SUCCEEDED(mResultCode)) {
      mResultCode = rv;
    }

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                         NS_DISPATCH_NORMAL)));
  }

  return NS_OK;
}

TransactionDatabaseOperationBase::TransactionDatabaseOperationBase(
                                                  TransactionBase* aTransaction)
  : DatabaseOperationBase(aTransaction->GetLoggingInfo()->Id(),
                          aTransaction->GetLoggingInfo()->NextRequestSN())
  , mTransaction(aTransaction)
  , mTransactionLoggingSerialNumber(aTransaction->LoggingSerialNumber())
  , mTransactionIsAborted(aTransaction->IsAborted())
{
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(LoggingSerialNumber());
}

TransactionDatabaseOperationBase::TransactionDatabaseOperationBase(
                                                  TransactionBase* aTransaction,
                                                  uint64_t aLoggingSerialNumber)
  : DatabaseOperationBase(aTransaction->GetLoggingInfo()->Id(),
                          aLoggingSerialNumber)
  , mTransaction(aTransaction)
  , mTransactionLoggingSerialNumber(aTransaction->LoggingSerialNumber())
  , mTransactionIsAborted(aTransaction->IsAborted())
{
  MOZ_ASSERT(aTransaction);
}

TransactionDatabaseOperationBase::~TransactionDatabaseOperationBase()
{
  MOZ_ASSERT(!mTransaction,
             "TransactionDatabaseOperationBase::Cleanup() was not called by a "
             "subclass!");
}

#ifdef DEBUG

void
TransactionDatabaseOperationBase::AssertIsOnConnectionThread() const
{
  MOZ_ASSERT(mTransaction);
  mTransaction->AssertIsOnConnectionThread();
}

#endif 

void
TransactionDatabaseOperationBase::DispatchToConnectionPool()
{
  AssertIsOnOwningThread();

  gConnectionPool->Dispatch(mTransaction->TransactionId(), this);

  mTransaction->NoteActiveRequest();
}

void
TransactionDatabaseOperationBase::RunOnConnectionThread()
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(mTransaction);
  MOZ_ASSERT(NS_SUCCEEDED(mResultCode));

  PROFILER_LABEL("IndexedDB",
                 "TransactionDatabaseOperationBase::RunOnConnectionThread",
                 js::ProfileEntry::Category::STORAGE);

  

  if (mTransactionIsAborted) {
    
    mResultCode = NS_ERROR_DOM_INDEXEDDB_ABORT_ERR;
  } else if (mTransaction->IsInvalidatedOnAnyThread()) {
    
    mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  } else if (!OperationMayProceed()) {
    
    
    IDB_REPORT_INTERNAL_ERR();
    mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  } else {
    Database* database = mTransaction->GetDatabase();
    MOZ_ASSERT(database);

    
    nsresult rv = database->EnsureConnection();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mResultCode = rv;
    } else {
      DatabaseConnection* connection = database->GetConnection();
      MOZ_ASSERT(connection);
      MOZ_ASSERT(connection->GetStorageConnection());

      AutoSetProgressHandler autoProgress;
      if (mLoggingSerialNumber) {
        rv = autoProgress.Register(connection->GetStorageConnection(), this);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          mResultCode = rv;
        }
      }

      if (NS_SUCCEEDED(rv)) {
        if (mLoggingSerialNumber) {
          IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld] Request[%llu]: "
                         "Beginning database work",
                       "IndexedDB %s: P T[%lld] R[%llu]: DB Start",
                       IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
                       mTransactionLoggingSerialNumber,
                       mLoggingSerialNumber);
        }

        rv = DoDatabaseWork(connection);

        if (mLoggingSerialNumber) {
          IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld] Request[%llu]: "
                         "Finished database work",
                       "IndexedDB %s: P T[%lld] R[%llu]: DB End",
                       IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
                       mTransactionLoggingSerialNumber,
                       mLoggingSerialNumber);
        }

        if (NS_FAILED(rv)) {
          mResultCode = rv;
        }
      }
    }
  }

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mOwningThread->Dispatch(this,
                                                       NS_DISPATCH_NORMAL)));
}

void
TransactionDatabaseOperationBase::RunOnOwningThread()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mTransaction);

  if (NS_WARN_IF(IsActorDestroyed())) {
    
    if (NS_SUCCEEDED(mResultCode)) {
      IDB_REPORT_INTERNAL_ERR();
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  } else {
    if (mTransaction->IsInvalidated()) {
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    } else if (mTransaction->IsAborted()) {
      
      
      mResultCode = NS_ERROR_DOM_INDEXEDDB_ABORT_ERR;
    } else if (NS_SUCCEEDED(mResultCode)) {
      
      mResultCode = SendSuccessResult();
    }

    if (NS_FAILED(mResultCode)) {
      
      if (!SendFailureResult(mResultCode)) {
        
        mTransaction->Abort(mResultCode,  false);
      }
    }
  }

  if (mLoggingSerialNumber) {
    mTransaction->NoteFinishedRequest();
  }

  Cleanup();
}

bool
TransactionDatabaseOperationBase::Init(TransactionBase* aTransaction)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);

  return true;
}

void
TransactionDatabaseOperationBase::Cleanup()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mTransaction);

  mTransaction = nullptr;
}

NS_IMETHODIMP
TransactionDatabaseOperationBase::Run()
{
  MOZ_ASSERT(mTransaction);

  if (IsOnBackgroundThread()) {
    RunOnOwningThread();
  } else {
    RunOnConnectionThread();
  }

  return NS_OK;
}

TransactionBase::
CommitOp::CommitOp(TransactionBase* aTransaction, nsresult aResultCode)
  : DatabaseOperationBase(aTransaction->GetLoggingInfo()->Id(),
                          aTransaction->GetLoggingInfo()->NextRequestSN())
  , mTransaction(aTransaction)
  , mResultCode(aResultCode)
{
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(LoggingSerialNumber());
}

nsresult
TransactionBase::
CommitOp::WriteAutoIncrementCounts()
{
  MOZ_ASSERT(mTransaction);
  mTransaction->AssertIsOnConnectionThread();
  MOZ_ASSERT(mTransaction->GetMode() == IDBTransaction::READ_WRITE ||
             mTransaction->GetMode() == IDBTransaction::READ_WRITE_FLUSH ||
             mTransaction->GetMode() == IDBTransaction::VERSION_CHANGE);

  const nsTArray<nsRefPtr<FullObjectStoreMetadata>>& metadataArray =
    mTransaction->mModifiedAutoIncrementObjectStoreMetadataArray;

  if (!metadataArray.IsEmpty()) {
    NS_NAMED_LITERAL_CSTRING(osid, "osid");
    NS_NAMED_LITERAL_CSTRING(ai, "ai");

    Database* database = mTransaction->GetDatabase();
    MOZ_ASSERT(database);

    DatabaseConnection* connection = database->GetConnection();
    MOZ_ASSERT(connection);

    DatabaseConnection::CachedStatement stmt;
    nsresult rv;

    for (uint32_t count = metadataArray.Length(), index = 0;
         index < count;
         index++) {
      const nsRefPtr<FullObjectStoreMetadata>& metadata = metadataArray[index];
      MOZ_ASSERT(!metadata->mDeleted);
      MOZ_ASSERT(metadata->mNextAutoIncrementId > 1);

      if (stmt) {
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->Reset()));
      } else {
        rv = connection->GetCachedStatement(
          NS_LITERAL_CSTRING("UPDATE object_store "
                             "SET auto_increment = :") + ai +
          NS_LITERAL_CSTRING(" WHERE id = :") + osid +
          NS_LITERAL_CSTRING(";"),
          &stmt);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      rv = stmt->BindInt64ByName(osid, metadata->mCommonMetadata.id());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindInt64ByName(ai, metadata->mNextAutoIncrementId);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->Execute();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  return NS_OK;
}

void
TransactionBase::
CommitOp::CommitOrRollbackAutoIncrementCounts()
{
  MOZ_ASSERT(mTransaction);
  mTransaction->AssertIsOnConnectionThread();
  MOZ_ASSERT(mTransaction->GetMode() == IDBTransaction::READ_WRITE ||
             mTransaction->GetMode() == IDBTransaction::READ_WRITE_FLUSH ||
             mTransaction->GetMode() == IDBTransaction::VERSION_CHANGE);

  nsTArray<nsRefPtr<FullObjectStoreMetadata>>& metadataArray =
    mTransaction->mModifiedAutoIncrementObjectStoreMetadataArray;

  if (!metadataArray.IsEmpty()) {
    bool committed = NS_SUCCEEDED(mResultCode);

    for (uint32_t count = metadataArray.Length(), index = 0;
         index < count;
         index++) {
      nsRefPtr<FullObjectStoreMetadata>& metadata = metadataArray[index];

      if (committed) {
        metadata->mComittedAutoIncrementId = metadata->mNextAutoIncrementId;
      } else {
        metadata->mNextAutoIncrementId = metadata->mComittedAutoIncrementId;
      }
    }
  }
}

#ifdef DEBUG

void
TransactionBase::
CommitOp::AssertForeignKeyConsistency(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(mTransaction);
  mTransaction->AssertIsOnConnectionThread();
  MOZ_ASSERT(mTransaction->GetMode() != IDBTransaction::READ_ONLY);

  DatabaseConnection::CachedStatement pragmaStmt;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aConnection->GetCachedStatement(NS_LITERAL_CSTRING("PRAGMA foreign_keys;"),
                                    &pragmaStmt)));

  bool hasResult;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(pragmaStmt->ExecuteStep(&hasResult)));

  MOZ_ASSERT(hasResult);

  int32_t foreignKeysEnabled;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(pragmaStmt->GetInt32(0, &foreignKeysEnabled)));

  MOZ_ASSERT(foreignKeysEnabled, "Database doesn't have foreign keys enabled!");

  DatabaseConnection::CachedStatement checkStmt;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aConnection->GetCachedStatement(
      NS_LITERAL_CSTRING("PRAGMA foreign_key_check;"),
      &checkStmt)));

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(checkStmt->ExecuteStep(&hasResult)));

  MOZ_ASSERT(!hasResult, "Database has inconsisistent foreign keys!");
}

#endif 

NS_IMPL_ISUPPORTS_INHERITED0(TransactionBase::CommitOp, DatabaseOperationBase)

NS_IMETHODIMP
TransactionBase::
CommitOp::Run()
{
  MOZ_ASSERT(mTransaction);
  mTransaction->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "TransactionBase::CommitOp::Run",
                 js::ProfileEntry::Category::STORAGE);

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld] Request[%llu]: "
                 "Beginning database work",
               "IndexedDB %s: P T[%lld] R[%llu]: DB Start",
               IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
               mTransaction->LoggingSerialNumber(),
               mLoggingSerialNumber);

  if (mTransaction->GetMode() != IDBTransaction::READ_ONLY &&
      mTransaction->mHasBeenActiveOnConnectionThread) {
    Database* database = mTransaction->GetDatabase();
    MOZ_ASSERT(database);

    if (DatabaseConnection* connection = database->GetConnection()) {
      
      DatabaseConnection::UpdateRefcountFunction* fileRefcountFunction =
        connection->GetUpdateRefcountFunction();

      if (NS_SUCCEEDED(mResultCode)) {
        if (fileRefcountFunction) {
          mResultCode = fileRefcountFunction->WillCommit();
          NS_WARN_IF_FALSE(NS_SUCCEEDED(mResultCode), "WillCommit() failed!");
        }

        if (NS_SUCCEEDED(mResultCode)) {
          mResultCode = WriteAutoIncrementCounts();
          NS_WARN_IF_FALSE(NS_SUCCEEDED(mResultCode),
                           "WriteAutoIncrementCounts() failed!");

          if (NS_SUCCEEDED(mResultCode)) {
            AssertForeignKeyConsistency(connection);

            mResultCode = connection->CommitWriteTransaction();
            NS_WARN_IF_FALSE(NS_SUCCEEDED(mResultCode), "Commit failed!");

            if (NS_SUCCEEDED(mResultCode) &&
                mTransaction->GetMode() == IDBTransaction::READ_WRITE_FLUSH) {
              mResultCode = connection->Checkpoint();
            }

            if (NS_SUCCEEDED(mResultCode) && fileRefcountFunction) {
              fileRefcountFunction->DidCommit();
            }
          }
        }
      }

      if (NS_FAILED(mResultCode)) {
        if (fileRefcountFunction) {
          fileRefcountFunction->DidAbort();
        }

        connection->RollbackWriteTransaction();
      }

      CommitOrRollbackAutoIncrementCounts();

      connection->FinishWriteTransaction();
    }
  }

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld] Request[%llu]: "
                 "Finished database work",
               "IndexedDB %s: P T[%lld] R[%llu]: DB End",
               IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
               mTransaction->LoggingSerialNumber(),
               mLoggingSerialNumber);

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld]: "
                 "Finished database work",
               "IndexedDB %s: P T[%lld]: DB End",
               IDB_LOG_ID_STRING(mBackgroundChildLoggingId),
               mLoggingSerialNumber);

  return NS_OK;
}

void
TransactionBase::
CommitOp::TransactionFinishedBeforeUnblock()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mTransaction);

  PROFILER_LABEL("IndexedDB",
                 "CommitOp::TransactionFinishedBeforeUnblock",
                 js::ProfileEntry::Category::STORAGE);

  if (!IsActorDestroyed()) {
    mTransaction->UpdateMetadata(mResultCode);
  }
}

void
TransactionBase::
CommitOp::TransactionFinishedAfterUnblock()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mTransaction);

  IDB_LOG_MARK("IndexedDB %s: Parent Transaction[%lld]: "
                 "Finished with result 0x%x",
               "IndexedDB %s: P T[%lld]: Transaction finished (0x%x)",
               IDB_LOG_ID_STRING(mTransaction->GetLoggingInfo()->Id()),
               mTransaction->LoggingSerialNumber(),
               mResultCode);

  mTransaction->SendCompleteNotification(ClampResultCode(mResultCode));

  Database* database = mTransaction->GetDatabase();
  MOZ_ASSERT(database);

  database->UnregisterTransaction(mTransaction);

  mTransaction = nullptr;

#ifdef DEBUG
  
  
  NoteActorDestroyed();
#endif
}

nsresult
VersionChangeTransactionOp::SendSuccessResult()
{
  AssertIsOnOwningThread();

  
  return NS_OK;
}

bool
VersionChangeTransactionOp::SendFailureResult(nsresult aResultCode)
{
  AssertIsOnOwningThread();

  
  return false;
}

void
VersionChangeTransactionOp::Cleanup()
{
  AssertIsOnOwningThread();

#ifdef DEBUG
  
  
  
  NoteActorDestroyed();
#endif

  TransactionDatabaseOperationBase::Cleanup();
}

nsresult
CreateObjectStoreOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "CreateObjectStoreOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(IndexedDatabaseManager::InLowDiskSpaceMode())) {
    return NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
  }

#ifdef DEBUG
  {
    
    
    
    DatabaseConnection::CachedStatement stmt;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "SELECT name "
          "FROM object_store "
          "WHERE name = :name;"),
        &stmt)));

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mMetadata.name())));

    bool hasResult;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));
    MOZ_ASSERT(!hasResult);
  }
#endif

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DatabaseConnection::CachedStatement stmt;
  rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "INSERT INTO object_store (id, auto_increment, name, key_path) "
      "VALUES (:id, :auto_increment, :name, :key_path);"),
    &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), mMetadata.id());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("auto_increment"),
                             mMetadata.autoIncrement() ? 1 : 0);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mMetadata.name());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  NS_NAMED_LITERAL_CSTRING(keyPath, "key_path");

  if (mMetadata.keyPath().IsValid()) {
    nsAutoString keyPathSerialization;
    mMetadata.keyPath().SerializeToString(keyPathSerialization);

    rv = stmt->BindStringByName(keyPath, keyPathSerialization);
  } else {
    rv = stmt->BindNullByName(keyPath);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef DEBUG
  {
    int64_t id;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetStorageConnection()->GetLastInsertRowID(&id)));
    MOZ_ASSERT(mMetadata.id() == id);
  }
#endif

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
DeleteObjectStoreOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "DeleteObjectStoreOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  NS_NAMED_LITERAL_CSTRING(objectStoreIdString, "object_store_id");

#ifdef DEBUG
  {
    
    DatabaseConnection::CachedStatement stmt;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "SELECT id "
          "FROM object_store;"),
        &stmt)));

    bool foundThisObjectStore = false;
    bool foundOtherObjectStore = false;

    while (true) {
      bool hasResult;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));

      if (!hasResult) {
        break;
      }

      int64_t id;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->GetInt64(0, &id)));

      if (id == mMetadata->mCommonMetadata.id()) {
        foundThisObjectStore = true;
      } else {
        foundOtherObjectStore = true;
      }
    }

    MOZ_ASSERT_IF(mIsLastObjectStore,
                  foundThisObjectStore && !foundOtherObjectStore);
    MOZ_ASSERT_IF(!mIsLastObjectStore,
                  foundThisObjectStore && foundOtherObjectStore);
  }
#endif

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mIsLastObjectStore) {
    
    DatabaseConnection::CachedStatement stmt;
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM index_data;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM unique_index_data;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM object_data;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM object_store_index;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM object_store;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    bool hasIndexes;
    rv = ObjectStoreHasIndexes(aConnection,
                               mMetadata->mCommonMetadata.id(),
                               &hasIndexes);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (hasIndexes) {
      rv = DeleteObjectStoreDataTableRowsWithIndexes(
        aConnection,
        mMetadata->mCommonMetadata.id(),
        void_t());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      
      DatabaseConnection::CachedStatement stmt;
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "DELETE FROM object_store_index "
          "WHERE object_store_id = :object_store_id;"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindInt64ByName(objectStoreIdString,
                                 mMetadata->mCommonMetadata.id());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->Execute();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      
      
      DatabaseConnection::CachedStatement stmt;
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "DELETE FROM object_data "
          "WHERE object_store_id = :object_store_id;"),
        &stmt);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->BindInt64ByName(objectStoreIdString,
                                 mMetadata->mCommonMetadata.id());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = stmt->Execute();
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    DatabaseConnection::CachedStatement stmt;
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM object_store "
        "WHERE id = :object_store_id;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64ByName(objectStoreIdString,
                               mMetadata->mCommonMetadata.id());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

#ifdef DEBUG
    {
      int32_t deletedRowCount;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        aConnection->GetStorageConnection()->
          GetAffectedRows(&deletedRowCount)));
      MOZ_ASSERT(deletedRowCount == 1);
    }
#endif
  }

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mMetadata->mCommonMetadata.autoIncrement()) {
    Transaction()->ForgetModifiedAutoIncrementObjectStore(mMetadata);
  }

  return NS_OK;
}

CreateIndexOp::CreateIndexOp(VersionChangeTransaction* aTransaction,
                             const int64_t aObjectStoreId,
                             const IndexMetadata& aMetadata)
  : VersionChangeTransactionOp(aTransaction)
  , mMetadata(aMetadata)
  , mFileManager(aTransaction->GetDatabase()->GetFileManager())
  , mDatabaseId(aTransaction->DatabaseId())
  , mObjectStoreId(aObjectStoreId)
{
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(aMetadata.id());
  MOZ_ASSERT(mFileManager);
  MOZ_ASSERT(!mDatabaseId.IsEmpty());
}

unsigned int CreateIndexOp::sThreadLocalIndex = kBadThreadLocalIndex;

nsresult
CreateIndexOp::InsertDataFromObjectStore(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!IndexedDatabaseManager::InLowDiskSpaceMode());
  MOZ_ASSERT(mMaybeUniqueIndexTable);

  PROFILER_LABEL("IndexedDB",
                 "CreateIndexOp::InsertDataFromObjectStore",
                 js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<mozIStorageConnection> storageConnection =
    aConnection->GetStorageConnection();
  MOZ_ASSERT(storageConnection);

  ThreadLocalJSRuntime* runtime = ThreadLocalJSRuntime::GetOrCreate();
  if (NS_WARN_IF(!runtime)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  JSContext* cx = runtime->Context();
  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, runtime->Global());

  nsRefPtr<UpdateIndexDataValuesFunction> updateFunction =
    new UpdateIndexDataValuesFunction(this, aConnection, cx);

  NS_NAMED_LITERAL_CSTRING(updateFunctionName, "update_index_data_values");

  nsresult rv =
    storageConnection->CreateFunction(updateFunctionName,
                                      4,
                                      updateFunction);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = InsertDataFromObjectStoreInternal(aConnection);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    storageConnection->RemoveFunction(updateFunctionName)));

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
CreateIndexOp::InsertDataFromObjectStoreInternal(
                                                DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(!IndexedDatabaseManager::InLowDiskSpaceMode());
  MOZ_ASSERT(mMaybeUniqueIndexTable);

  nsCOMPtr<mozIStorageConnection> storageConnection =
    aConnection->GetStorageConnection();
  MOZ_ASSERT(storageConnection);

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "UPDATE object_data "
      "SET index_data_values = update_index_data_values "
        "(key, index_data_values, file_ids, data) "
      "WHERE object_store_id = :object_store_id;"),
    &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                             mObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

bool
CreateIndexOp::Init(TransactionBase* aTransaction)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);

  struct MOZ_STACK_CLASS Helper final
  {
    static void
    Destroy(void* aThreadLocal)
    {
      delete static_cast<ThreadLocalJSRuntime*>(aThreadLocal);
    }
  };

  if (sThreadLocalIndex == kBadThreadLocalIndex) {
    if (NS_WARN_IF(PR_SUCCESS !=
                     PR_NewThreadPrivateIndex(&sThreadLocalIndex,
                                              &Helper::Destroy))) {
      return false;
    }
  }

  MOZ_ASSERT(sThreadLocalIndex != kBadThreadLocalIndex);

  nsresult rv =
    GetUniqueIndexTableForObjectStore(aTransaction,
                                      mObjectStoreId,
                                      mMaybeUniqueIndexTable);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  return true;
}

nsresult
CreateIndexOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "CreateIndexOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(IndexedDatabaseManager::InLowDiskSpaceMode())) {
    return NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
  }

#ifdef DEBUG
  {
    
    
    
    DatabaseConnection::CachedStatement stmt;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "SELECT name "
          "FROM object_store_index "
          "WHERE object_store_id = :osid "
          "AND name = :name;"),
        &stmt)));
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mObjectStoreId)));
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mMetadata.name())));

    bool hasResult;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));

    MOZ_ASSERT(!hasResult);
  }
#endif

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DatabaseConnection::CachedStatement stmt;
  rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_index (id, name, key_path, unique_index, "
                                    "multientry, object_store_id) "
    "VALUES (:id, :name, :key_path, :unique, :multientry, :osid)"),
    &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), mMetadata.id());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mMetadata.name());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoString keyPathSerialization;
  mMetadata.keyPath().SerializeToString(keyPathSerialization);
  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("key_path"),
                              keyPathSerialization);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("unique"),
                             mMetadata.unique() ? 1 : 0);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("multientry"),
                             mMetadata.multiEntry() ? 1 : 0);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef DEBUG
  {
    int64_t id;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetStorageConnection()->GetLastInsertRowID(&id)));
    MOZ_ASSERT(mMetadata.id() == id);
  }
#endif

  rv = InsertDataFromObjectStore(aConnection);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

const JSClass CreateIndexOp::ThreadLocalJSRuntime::kGlobalClass = {
  "IndexedDBTransactionThreadGlobal",
  JSCLASS_GLOBAL_FLAGS,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   JS_GlobalObjectTraceHook
};


auto
CreateIndexOp::
ThreadLocalJSRuntime::GetOrCreate() -> ThreadLocalJSRuntime*
{
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(CreateIndexOp::kBadThreadLocalIndex !=
             CreateIndexOp::sThreadLocalIndex);

  auto* runtime = static_cast<ThreadLocalJSRuntime*>(
    PR_GetThreadPrivate(CreateIndexOp::sThreadLocalIndex));
  if (runtime) {
    return runtime;
  }

  nsAutoPtr<ThreadLocalJSRuntime> newRuntime(new ThreadLocalJSRuntime());

  if (NS_WARN_IF(!newRuntime->Init())) {
    return nullptr;
  }

  DebugOnly<PRStatus> status =
    PR_SetThreadPrivate(CreateIndexOp::sThreadLocalIndex, newRuntime);
  MOZ_ASSERT(status == PR_SUCCESS);

  return newRuntime.forget();
}

bool
CreateIndexOp::
ThreadLocalJSRuntime::Init()
{
  MOZ_ASSERT(!IsOnBackgroundThread());

  mRuntime = JS_NewRuntime(kRuntimeHeapSize);
  if (NS_WARN_IF(!mRuntime)) {
    return false;
  }

  
  JS_SetNativeStackQuota(mRuntime, 128 * sizeof(size_t) * 1024); 

  mContext = JS_NewContext(mRuntime, 0);
  if (NS_WARN_IF(!mContext)) {
    return false;
  }

  JSAutoRequest ar(mContext);

  mGlobal = JS_NewGlobalObject(mContext, &kGlobalClass, nullptr,
                               JS::FireOnNewGlobalHook);
  if (NS_WARN_IF(!mGlobal)) {
    return false;
  }

  return true;
}

NS_IMPL_ISUPPORTS(CreateIndexOp::UpdateIndexDataValuesFunction,
                  mozIStorageFunction);

NS_IMETHODIMP
CreateIndexOp::
UpdateIndexDataValuesFunction::OnFunctionCall(mozIStorageValueArray* aValues,
                                              nsIVariant** _retval)
{
  MOZ_ASSERT(aValues);
  MOZ_ASSERT(_retval);
  MOZ_ASSERT(mConnection);
  mConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mOp);
  MOZ_ASSERT(mCx);

  PROFILER_LABEL("IndexedDB",
                 "CreateIndexOp::UpdateIndexDataValuesFunction::OnFunctionCall",
                 js::ProfileEntry::Category::STORAGE);

#ifdef DEBUG
  {
    uint32_t argCount;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetNumEntries(&argCount)));
    MOZ_ASSERT(argCount == 4); 

    int32_t valueType;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(0, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(1, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_NULL ||
               valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(2, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_NULL ||
               valueType == mozIStorageValueArray::VALUE_TYPE_TEXT);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aValues->GetTypeOfIndex(3, &valueType)));
    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);
  }
#endif

  StructuredCloneReadInfo cloneInfo;
  nsresult rv =
    GetStructuredCloneReadInfoFromValueArray(aValues,
                                              3,
                                              2,
                                             mOp->mFileManager,
                                             &cloneInfo);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  JS::Rooted<JS::Value> clone(mCx);
  if (NS_WARN_IF(!IDBObjectStore::DeserializeIndexValue(mCx,
                                                        cloneInfo,
                                                        &clone))) {
    return NS_ERROR_DOM_DATA_CLONE_ERR;
  }

  const IndexMetadata& metadata = mOp->mMetadata;
  const int64_t& objectStoreId = mOp->mObjectStoreId;

  nsAutoTArray<IndexUpdateInfo, 32> updateInfos;
  rv = IDBObjectStore::AppendIndexUpdateInfo(metadata.id(),
                                             metadata.keyPath(),
                                             metadata.unique(),
                                             metadata.multiEntry(),
                                             mCx,
                                             clone,
                                             updateInfos);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (updateInfos.IsEmpty()) {
    

    nsCOMPtr<nsIVariant> unmodifiedValue;

    
    int32_t valueType;
    rv = aValues->GetTypeOfIndex(1, &valueType);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_NULL ||
               valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);

    if (valueType == mozIStorageValueArray::VALUE_TYPE_NULL) {
      unmodifiedValue = new storage::NullVariant();
      unmodifiedValue.forget(_retval);
      return NS_OK;
    }

    MOZ_ASSERT(valueType == mozIStorageValueArray::VALUE_TYPE_BLOB);

    const uint8_t* blobData;
    uint32_t blobDataLength;
    rv = aValues->GetSharedBlob(1, &blobDataLength, &blobData);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    std::pair<uint8_t *, int> copiedBlobDataPair(
      static_cast<uint8_t*>(malloc(blobDataLength)),
      blobDataLength);

    if (!copiedBlobDataPair.first) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(copiedBlobDataPair.first, blobData, blobDataLength);

    unmodifiedValue = new storage::AdoptedBlobVariant(copiedBlobDataPair);
    unmodifiedValue.forget(_retval);

    return NS_OK;
  }

  Key key;
  rv = key.SetFromValueArray(aValues, 0);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  AutoFallibleTArray<IndexDataValue, 32> indexValues;
  rv = ReadCompressedIndexDataValues(aValues, 1, indexValues);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  const bool hadPreviousIndexValues = !indexValues.IsEmpty();

  const uint32_t updateInfoCount = updateInfos.Length();

  if (NS_WARN_IF(!indexValues.SetCapacity(indexValues.Length() +
                                          updateInfoCount, fallible))) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  for (uint32_t index = 0; index < updateInfoCount; index++) {
    const IndexUpdateInfo& info = updateInfos[index];

    MOZ_ALWAYS_TRUE(
      indexValues.InsertElementSorted(IndexDataValue(metadata.id(),
                                                     metadata.unique(),
                                                     info.value()),
                                      fallible));
  }

  UniqueFreePtr<uint8_t> indexValuesBlob;
  uint32_t indexValuesBlobLength;
  rv = MakeCompressedIndexDataValues(indexValues,
                                     indexValuesBlob,
                                     &indexValuesBlobLength);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!indexValuesBlobLength == !(indexValuesBlob.get()));

  nsCOMPtr<nsIVariant> value;

  if (!indexValuesBlob) {
    value = new storage::NullVariant();

    value.forget(_retval);
    return NS_OK;
  }

  
  
  if (hadPreviousIndexValues) {
    indexValues.ClearAndRetainStorage();

    MOZ_ASSERT(indexValues.Capacity() >= updateInfoCount);

    for (uint32_t index = 0; index < updateInfoCount; index++) {
      const IndexUpdateInfo& info = updateInfos[index];

      MOZ_ALWAYS_TRUE(
        indexValues.InsertElementSorted(IndexDataValue(metadata.id(),
                                                       metadata.unique(),
                                                       info.value()),
                                        fallible));
    }
  }

  rv = InsertIndexTableRows(mConnection, objectStoreId, key, indexValues);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  std::pair<uint8_t *, int> copiedBlobDataPair(indexValuesBlob.get(),
                                               indexValuesBlobLength);

  value = new storage::AdoptedBlobVariant(copiedBlobDataPair);

  indexValuesBlob.release();

  value.forget(_retval);
  return NS_OK;
}

DeleteIndexOp::DeleteIndexOp(VersionChangeTransaction* aTransaction,
                             const int64_t aObjectStoreId,
                             const int64_t aIndexId,
                             const bool aUnique,
                             const bool aIsLastIndex)
  : VersionChangeTransactionOp(aTransaction)
  , mObjectStoreId(aObjectStoreId)
  , mIndexId(aIndexId)
  , mUnique(aUnique)
  , mIsLastIndex(aIsLastIndex)
{
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(aIndexId);
}

nsresult
DeleteIndexOp::RemoveReferencesToIndex(
                                   DatabaseConnection* aConnection,
                                   const Key& aObjectStoreKey,
                                   FallibleTArray<IndexDataValue>& aIndexValues)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!IsOnBackgroundThread());
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(!aObjectStoreKey.IsUnset());
  MOZ_ASSERT_IF(!mIsLastIndex, !aIndexValues.IsEmpty());

  struct MOZ_STACK_CLASS IndexIdComparator final
  {
    bool
    Equals(const IndexDataValue& aA, const IndexDataValue& aB) const
    {
      
      return aA.mIndexId == aB.mIndexId;
    };

    bool
    LessThan(const IndexDataValue& aA, const IndexDataValue& aB) const
    {
      return aA.mIndexId < aB.mIndexId;
    };
  };

  PROFILER_LABEL("IndexedDB",
                 "DeleteIndexOp::RemoveReferencesToIndex",
                 js::ProfileEntry::Category::STORAGE);

  if (mIsLastIndex) {
    
    
    DatabaseConnection::CachedStatement stmt;
    nsresult rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "UPDATE object_data "
        "SET index_data_values = NULL "
        "WHERE object_store_id = :object_store_id "
        "AND key = :key;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                               mObjectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = aObjectStoreKey.BindToStatement(stmt, NS_LITERAL_CSTRING("key"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    return NS_OK;
  }

  IndexDataValue search;
  search.mIndexId = mIndexId;

  
  
  size_t firstElementIndex =
    aIndexValues.BinaryIndexOf(search, IndexIdComparator());
  if (NS_WARN_IF(firstElementIndex == aIndexValues.NoIndex) ||
      NS_WARN_IF(aIndexValues[firstElementIndex].mIndexId != mIndexId)) {
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_FILE_CORRUPTED;
  }

  MOZ_ASSERT(aIndexValues[firstElementIndex].mIndexId == mIndexId);

  
  while (firstElementIndex) {
    if (aIndexValues[firstElementIndex - 1].mIndexId == mIndexId) {
      firstElementIndex--;
    } else {
      break;
    }
  }

  MOZ_ASSERT(aIndexValues[firstElementIndex].mIndexId == mIndexId);

  const size_t indexValuesLength = aIndexValues.Length();

  
  size_t lastElementIndex = firstElementIndex;

  while (lastElementIndex < indexValuesLength) {
    if (aIndexValues[lastElementIndex].mIndexId == mIndexId) {
      lastElementIndex++;
    } else {
      break;
    }
  }

  MOZ_ASSERT(lastElementIndex > firstElementIndex);
  MOZ_ASSERT_IF(lastElementIndex < indexValuesLength,
                aIndexValues[lastElementIndex].mIndexId != mIndexId);
  MOZ_ASSERT(aIndexValues[lastElementIndex - 1].mIndexId == mIndexId);

  aIndexValues.RemoveElementsAt(firstElementIndex,
                                lastElementIndex - firstElementIndex);

  nsresult rv = UpdateIndexValues(aConnection,
                                  mObjectStoreId,
                                  aObjectStoreKey,
                                  aIndexValues);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
DeleteIndexOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

#ifdef DEBUG
  {
    
    DatabaseConnection::CachedStatement stmt;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "SELECT id "
          "FROM object_store_index "
          "WHERE object_store_id = :object_store_id;"),
        &stmt)));

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      stmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                            mObjectStoreId)));

    bool foundThisIndex = false;
    bool foundOtherIndex = false;

    while (true) {
      bool hasResult;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)));

      if (!hasResult) {
        break;
      }

      int64_t id;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(stmt->GetInt64(0, &id)));

      if (id == mIndexId) {
        foundThisIndex = true;
      } else {
        foundOtherIndex = true;
      }
    }

    MOZ_ASSERT_IF(mIsLastIndex, foundThisIndex && !foundOtherIndex);
    MOZ_ASSERT_IF(!mIsLastIndex, foundThisIndex && foundOtherIndex);
  }
#endif

  PROFILER_LABEL("IndexedDB",
                 "DeleteIndexOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DatabaseConnection::CachedStatement selectStmt;

  
  
  
  if (mUnique) {
    if (mIsLastIndex) {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "/* do not warn (bug someone else) */ "
        "SELECT value, object_data_key "
          "FROM unique_index_data "
          "WHERE index_id = :index_id "
          "ORDER BY object_data_key ASC;"),
        &selectStmt);
    } else {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "/* do not warn (bug out) */ "
        "SELECT unique_index_data.value, "
               "unique_index_data.object_data_key, "
               "object_data.index_data_values "
          "FROM unique_index_data "
          "JOIN object_data "
          "ON unique_index_data.object_data_key = object_data.key "
          "WHERE unique_index_data.index_id = :index_id "
          "AND object_data.object_store_id = :object_store_id "
          "ORDER BY unique_index_data.object_data_key ASC;"),
        &selectStmt);
    }
  } else {
    if (mIsLastIndex) {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "/* do not warn (bug me not) */ "
        "SELECT value, object_data_key "
          "FROM index_data "
          "WHERE index_id = :index_id "
          "AND object_store_id = :object_store_id "
          "ORDER BY object_data_key ASC;"),
        &selectStmt);
    } else {
      rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
        "/* do not warn (bug off) */ "
        "SELECT index_data.value, "
               "index_data.object_data_key, "
               "object_data.index_data_values "
          "FROM index_data "
          "JOIN object_data "
          "ON index_data.object_data_key = object_data.key "
          "WHERE index_data.index_id = :index_id "
          "AND object_data.object_store_id = :object_store_id "
          "ORDER BY index_data.object_data_key ASC;"),
        &selectStmt);
    }
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  NS_NAMED_LITERAL_CSTRING(indexIdString, "index_id");

  rv = selectStmt->BindInt64ByName(indexIdString, mIndexId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!mUnique || !mIsLastIndex) {
    rv = selectStmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                                     mObjectStoreId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  NS_NAMED_LITERAL_CSTRING(valueString, "value");
  NS_NAMED_LITERAL_CSTRING(objectDataKeyString, "object_data_key");

  DatabaseConnection::CachedStatement deleteIndexRowStmt;
  DatabaseConnection::CachedStatement nullIndexDataValuesStmt;

  Key lastObjectStoreKey;
  AutoFallibleTArray<IndexDataValue, 32> lastIndexValues;

  bool hasResult;
  while (NS_SUCCEEDED(rv = selectStmt->ExecuteStep(&hasResult)) && hasResult) {
    
    Key indexKey;
    rv = indexKey.SetFromStatement(selectStmt, 0);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(indexKey.IsUnset())) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_FILE_CORRUPTED;
    }

    
    
    const uint8_t* objectStoreKeyData;
    uint32_t objectStoreKeyDataLength;
    rv = selectStmt->GetSharedBlob(1,
                                   &objectStoreKeyDataLength,
                                   &objectStoreKeyData);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (NS_WARN_IF(!objectStoreKeyDataLength)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_FILE_CORRUPTED;
    }

    nsDependentCString currentObjectStoreKeyBuffer(
      reinterpret_cast<const char*>(objectStoreKeyData),
      objectStoreKeyDataLength);
    if (currentObjectStoreKeyBuffer != lastObjectStoreKey.GetBuffer()) {
      
      if (!lastObjectStoreKey.IsUnset()) {
        
        
        rv = RemoveReferencesToIndex(aConnection,
                                      lastObjectStoreKey,
                                      lastIndexValues);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      
      lastObjectStoreKey = Key(currentObjectStoreKeyBuffer);

      
      if (!mIsLastIndex) {
        lastIndexValues.ClearAndRetainStorage();
        rv = ReadCompressedIndexDataValues(selectStmt, 2, lastIndexValues);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (NS_WARN_IF(lastIndexValues.IsEmpty())) {
          IDB_REPORT_INTERNAL_ERR();
          return NS_ERROR_FILE_CORRUPTED;
        }
      }
    }

    
    if (deleteIndexRowStmt) {
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(deleteIndexRowStmt->Reset()));
    } else {
      if (mUnique) {
        rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
          "DELETE FROM unique_index_data "
            "WHERE index_id = :index_id "
            "AND value = :value;"),
          &deleteIndexRowStmt);
      } else {
        rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
          "DELETE FROM index_data "
            "WHERE index_id = :index_id "
            "AND value = :value "
            "AND object_data_key = :object_data_key;"),
          &deleteIndexRowStmt);
      }
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = deleteIndexRowStmt->BindInt64ByName(indexIdString, mIndexId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = indexKey.BindToStatement(deleteIndexRowStmt, valueString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!mUnique) {
      rv = lastObjectStoreKey.BindToStatement(deleteIndexRowStmt,
                                              objectDataKeyString);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }

    rv = deleteIndexRowStmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (!lastObjectStoreKey.IsUnset()) {
    MOZ_ASSERT_IF(!mIsLastIndex, !lastIndexValues.IsEmpty());

    rv = RemoveReferencesToIndex(aConnection,
                                 lastObjectStoreKey,
                                 lastIndexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  DatabaseConnection::CachedStatement deleteStmt;
  rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "DELETE FROM object_store_index "
      "WHERE id = :index_id;"),
    &deleteStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = deleteStmt->BindInt64ByName(indexIdString, mIndexId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = deleteStmt->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef DEBUG
  {
    int32_t deletedRowCount;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      aConnection->GetStorageConnection()->GetAffectedRows(&deletedRowCount)));
    MOZ_ASSERT(deletedRowCount == 1);
  }
#endif

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}


nsresult
NormalTransactionOp::ObjectStoreHasIndexes(NormalTransactionOp* aOp,
                                           DatabaseConnection* aConnection,
                                           const int64_t aObjectStoreId,
                                           const bool aMayHaveIndexes,
                                           bool* aHasIndexes)
{
  MOZ_ASSERT(aOp);
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(aHasIndexes);

  bool hasIndexes;
  if (aOp->Transaction()->GetMode() == IDBTransaction::VERSION_CHANGE &&
      aMayHaveIndexes) {
    
    
    
    
    nsresult rv =
      DatabaseOperationBase::ObjectStoreHasIndexes(aConnection,
                                                   aObjectStoreId,
                                                   &hasIndexes);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    MOZ_ASSERT(NS_SUCCEEDED(
      DatabaseOperationBase::ObjectStoreHasIndexes(aConnection,
                                                   aObjectStoreId,
                                                   &hasIndexes)));
    MOZ_ASSERT(aMayHaveIndexes == hasIndexes);

    hasIndexes = aMayHaveIndexes;
  }

  *aHasIndexes = hasIndexes;
  return NS_OK;
}

nsresult
NormalTransactionOp::SendSuccessResult()
{
  AssertIsOnOwningThread();

  if (!IsActorDestroyed()) {
    RequestResponse response;
    GetResponse(response);

    MOZ_ASSERT(response.type() != RequestResponse::T__None);

    if (response.type() == RequestResponse::Tnsresult) {
      MOZ_ASSERT(NS_FAILED(response.get_nsresult()));

      return response.get_nsresult();
    }

    if (NS_WARN_IF(!PBackgroundIDBRequestParent::Send__delete__(this,
                                                                response))) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }

  mResponseSent = true;

  return NS_OK;
}

bool
NormalTransactionOp::SendFailureResult(nsresult aResultCode)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aResultCode));

  bool result = false;

  if (!IsActorDestroyed()) {
    result =
      PBackgroundIDBRequestParent::Send__delete__(this,
                                                  ClampResultCode(aResultCode));
  }

  mResponseSent = true;

  return result;
}

void
NormalTransactionOp::Cleanup()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT_IF(!IsActorDestroyed(), mResponseSent);

  TransactionDatabaseOperationBase::Cleanup();
}

void
NormalTransactionOp::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnOwningThread();

  NoteActorDestroyed();
}

ObjectStoreAddOrPutRequestOp::ObjectStoreAddOrPutRequestOp(
                                                  TransactionBase* aTransaction,
                                                  const RequestParams& aParams)
  : NormalTransactionOp(aTransaction)
  , mParams(aParams.type() == RequestParams::TObjectStoreAddParams ?
              aParams.get_ObjectStoreAddParams().commonParams() :
              aParams.get_ObjectStorePutParams().commonParams())
  , mGroup(aTransaction->GetDatabase()->Group())
  , mOrigin(aTransaction->GetDatabase()->Origin())
  , mPersistenceType(aTransaction->GetDatabase()->Type())
  , mOverwrite(aParams.type() == RequestParams::TObjectStorePutParams)
  , mObjectStoreMayHaveIndexes(false)
{
  MOZ_ASSERT(aParams.type() == RequestParams::TObjectStoreAddParams ||
             aParams.type() == RequestParams::TObjectStorePutParams);

  mMetadata =
    aTransaction->GetMetadataForObjectStoreId(mParams.objectStoreId());
  MOZ_ASSERT(mMetadata);

  const_cast<bool&>(mObjectStoreMayHaveIndexes) = mMetadata->HasLiveIndexes();
}

nsresult
ObjectStoreAddOrPutRequestOp::RemoveOldIndexDataValues(
                                                DatabaseConnection* aConnection)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aConnection);
  MOZ_ASSERT(mOverwrite);
  MOZ_ASSERT(!mResponse.IsUnset());

#ifdef DEBUG
  {
    bool hasIndexes = false;
    MOZ_ASSERT(NS_SUCCEEDED(
      DatabaseOperationBase::ObjectStoreHasIndexes(aConnection,
                                                   mParams.objectStoreId(),
                                                   &hasIndexes)));
    MOZ_ASSERT(hasIndexes,
               "Don't use this slow method if there are no indexes!");
  }
#endif

  DatabaseConnection::CachedStatement indexValuesStmt;
  nsresult rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
    "SELECT index_data_values "
      "FROM object_data "
      "WHERE object_store_id = :object_store_id "
      "AND key = :key;"),
    &indexValuesStmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = indexValuesStmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                                        mParams.objectStoreId());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mResponse.BindToStatement(indexValuesStmt, NS_LITERAL_CSTRING("key"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasResult;
  rv = indexValuesStmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasResult) {
    AutoFallibleTArray<IndexDataValue, 32> existingIndexValues;
    rv = ReadCompressedIndexDataValues(indexValuesStmt,
                                        0,
                                        existingIndexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = DeleteIndexDataTableRows(aConnection, mResponse, existingIndexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

nsresult
ObjectStoreAddOrPutRequestOp::CopyFileData(nsIInputStream* aInputStream,
                                           nsIOutputStream* aOutputStream)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aInputStream);
  MOZ_ASSERT(aOutputStream);

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreAddOrPutRequestOp::CopyFileData",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

  do {
    char copyBuffer[kFileCopyBufferSize];

    uint32_t numRead;
    rv = aInputStream->Read(copyBuffer, sizeof(copyBuffer), &numRead);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      break;
    }

    if (!numRead) {
      break;
    }

    uint32_t numWrite;
    rv = aOutputStream->Write(copyBuffer, numRead, &numWrite);
    if (rv == NS_ERROR_FILE_NO_DEVICE_SPACE) {
      rv = NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
    }
    if (NS_WARN_IF(NS_FAILED(rv))) {
      break;
    }

    if (NS_WARN_IF(numWrite != numRead)) {
      rv = NS_ERROR_FAILURE;
      break;
    }
  } while (true);

  if (NS_SUCCEEDED(rv)) {
    rv = aOutputStream->Flush();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  nsresult rv2 = aOutputStream->Close();
  if (NS_WARN_IF(NS_FAILED(rv2))) {
    return NS_SUCCEEDED(rv) ? rv2 : rv;
  }

  return rv;
}

bool
ObjectStoreAddOrPutRequestOp::Init(TransactionBase* aTransaction)
{
  AssertIsOnOwningThread();

  const nsTArray<IndexUpdateInfo>& indexUpdateInfos =
    mParams.indexUpdateInfos();

  if (!indexUpdateInfos.IsEmpty()) {
    const uint32_t count = indexUpdateInfos.Length();

    mUniqueIndexTable.emplace();

    for (uint32_t index = 0; index < count; index++) {
      const IndexUpdateInfo& updateInfo = indexUpdateInfos[index];

      nsRefPtr<FullIndexMetadata> indexMetadata;
      MOZ_ALWAYS_TRUE(mMetadata->mIndexes.Get(updateInfo.indexId(),
                                              getter_AddRefs(indexMetadata)));

      MOZ_ASSERT(!indexMetadata->mDeleted);

      const int64_t& indexId = indexMetadata->mCommonMetadata.id();
      const bool& unique = indexMetadata->mCommonMetadata.unique();

      MOZ_ASSERT(indexId == updateInfo.indexId());
      MOZ_ASSERT_IF(!indexMetadata->mCommonMetadata.multiEntry(),
                    !mUniqueIndexTable.ref().Get(indexId));

      if (NS_WARN_IF(!mUniqueIndexTable.ref().Put(indexId, unique, fallible))) {
        return false;
      }
    }
  } else if (mOverwrite) {
    mUniqueIndexTable.emplace();
  }

#ifdef DEBUG
  if (mUniqueIndexTable.isSome()) {
    mUniqueIndexTable.ref().MarkImmutable();
  }
#endif

  const nsTArray<DatabaseFileOrMutableFileId>& files = mParams.files();

  if (!files.IsEmpty()) {
    const uint32_t count = files.Length();

    if (NS_WARN_IF(!mStoredFileInfos.SetCapacity(count, fallible))) {
      return false;
    }

    nsRefPtr<FileManager> fileManager =
      aTransaction->GetDatabase()->GetFileManager();
    MOZ_ASSERT(fileManager);

    for (uint32_t index = 0; index < count; index++) {
      const DatabaseFileOrMutableFileId& fileOrFileId = files[index];
      MOZ_ASSERT(fileOrFileId.type() ==
                   DatabaseFileOrMutableFileId::
                     TPBackgroundIDBDatabaseFileParent ||
                 fileOrFileId.type() == DatabaseFileOrMutableFileId::Tint64_t);

      StoredFileInfo* storedFileInfo = mStoredFileInfos.AppendElement(fallible);
      MOZ_ASSERT(storedFileInfo);

      switch (fileOrFileId.type()) {
        case DatabaseFileOrMutableFileId::TPBackgroundIDBDatabaseFileParent: {
          storedFileInfo->mFileActor =
            static_cast<DatabaseFile*>(
              fileOrFileId.get_PBackgroundIDBDatabaseFileParent());
          MOZ_ASSERT(storedFileInfo->mFileActor);

          storedFileInfo->mFileInfo = storedFileInfo->mFileActor->GetFileInfo();
          MOZ_ASSERT(storedFileInfo->mFileInfo);

          storedFileInfo->mInputStream =
            storedFileInfo->mFileActor->GetInputStream();
          if (storedFileInfo->mInputStream && !mFileManager) {
            mFileManager = fileManager;
          }
          break;
        }

        case DatabaseFileOrMutableFileId::Tint64_t:
          storedFileInfo->mFileInfo =
            fileManager->GetFileInfo(fileOrFileId.get_int64_t());
          MOZ_ASSERT(storedFileInfo->mFileInfo);
          break;

        case DatabaseFileOrMutableFileId::TPBackgroundIDBDatabaseFileChild:
        default:
          MOZ_CRASH("Should never get here!");
      }
    }
  }

  return true;
}

nsresult
ObjectStoreAddOrPutRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(aConnection->GetStorageConnection());
  MOZ_ASSERT_IF(mFileManager, !mStoredFileInfos.IsEmpty());

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreAddOrPutRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  if (NS_WARN_IF(IndexedDatabaseManager::InLowDiskSpaceMode())) {
    return NS_ERROR_DOM_INDEXEDDB_QUOTA_ERR;
  }

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool objectStoreHasIndexes;
  rv = ObjectStoreHasIndexes(this,
                             aConnection,
                             mParams.objectStoreId(),
                             mObjectStoreMayHaveIndexes,
                             &objectStoreHasIndexes);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  Key& key = mResponse;
  key = mParams.key();

  const bool keyUnset = key.IsUnset();
  const int64_t osid = mParams.objectStoreId();
  const KeyPath& keyPath = mMetadata->mCommonMetadata.keyPath();

  
  
  if (mOverwrite && !keyUnset && objectStoreHasIndexes) {
    rv = RemoveOldIndexDataValues(aConnection);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  
  
  
  DatabaseConnection::CachedStatement stmt;
  if (!mOverwrite || keyUnset) {
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "INSERT INTO object_data "
        "(object_store_id, key, file_ids, data) "
        "VALUES (:osid, :key, :file_ids, :data);"),
      &stmt);
  } else {
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "INSERT OR REPLACE INTO object_data "
        "(object_store_id, key, file_ids, data) "
        "VALUES (:osid, :key, :file_ids, :data);"),
    &stmt);
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), osid);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!keyUnset || mMetadata->mCommonMetadata.autoIncrement(),
             "Should have key unless autoIncrement");

  int64_t autoIncrementNum = 0;

  if (mMetadata->mCommonMetadata.autoIncrement()) {
    if (keyUnset) {
      autoIncrementNum = mMetadata->mNextAutoIncrementId;

      MOZ_ASSERT(autoIncrementNum > 0);

      if (autoIncrementNum > (1LL << 53)) {
        return NS_ERROR_DOM_INDEXEDDB_CONSTRAINT_ERR;
      }

      key.SetFromInteger(autoIncrementNum);
    } else if (key.IsFloat() &&
               key.ToFloat() >= mMetadata->mNextAutoIncrementId) {
      autoIncrementNum = floor(key.ToFloat());
    }

    if (keyUnset && keyPath.IsValid()) {
      const SerializedStructuredCloneWriteInfo& cloneInfo = mParams.cloneInfo();
      MOZ_ASSERT(cloneInfo.offsetToKeyProp());
      MOZ_ASSERT(cloneInfo.data().Length() > sizeof(uint64_t));
      MOZ_ASSERT(cloneInfo.offsetToKeyProp() <=
                 (cloneInfo.data().Length() - sizeof(uint64_t)));

      
      
      
      uint8_t* keyPropPointer =
        const_cast<uint8_t*>(cloneInfo.data().Elements() +
                             cloneInfo.offsetToKeyProp());
      uint64_t keyPropValue =
        ReinterpretDoubleAsUInt64(static_cast<double>(autoIncrementNum));

      LittleEndian::writeUint64(keyPropPointer, keyPropValue);
    }
  }

  key.BindToStatement(stmt, NS_LITERAL_CSTRING("key"));

  
  const char* uncompressed =
    reinterpret_cast<const char*>(mParams.cloneInfo().data().Elements());
  size_t uncompressedLength = mParams.cloneInfo().data().Length();

  
  
  {
    size_t compressedLength = snappy::MaxCompressedLength(uncompressedLength);

    char* compressed = static_cast<char*>(malloc(compressedLength));
    if (NS_WARN_IF(!compressed)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    snappy::RawCompress(uncompressed, uncompressedLength, compressed,
                        &compressedLength);

    uint8_t* dataBuffer = reinterpret_cast<uint8_t*>(compressed);
    size_t dataBufferLength = compressedLength;

    
    
    rv = stmt->BindAdoptedBlobByName(NS_LITERAL_CSTRING("data"), dataBuffer,
                                     dataBufferLength);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      free(compressed);
      return rv;
    }
  }

  nsCOMPtr<nsIFile> fileDirectory;
  nsCOMPtr<nsIFile> journalDirectory;

  if (mFileManager) {
    fileDirectory = mFileManager->GetDirectory();
    if (NS_WARN_IF(!fileDirectory)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    journalDirectory = mFileManager->EnsureJournalDirectory();
    if (NS_WARN_IF(!journalDirectory)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    DebugOnly<bool> exists;
    MOZ_ASSERT(NS_SUCCEEDED(fileDirectory->Exists(&exists)));
    MOZ_ASSERT(exists);

    DebugOnly<bool> isDirectory;
    MOZ_ASSERT(NS_SUCCEEDED(fileDirectory->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory);

    MOZ_ASSERT(NS_SUCCEEDED(journalDirectory->Exists(&exists)));
    MOZ_ASSERT(exists);

    MOZ_ASSERT(NS_SUCCEEDED(journalDirectory->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory);
  }

  if (!mStoredFileInfos.IsEmpty()) {
    nsAutoString fileIds;

    for (uint32_t count = mStoredFileInfos.Length(), index = 0;
         index < count;
         index++) {
      StoredFileInfo& storedFileInfo = mStoredFileInfos[index];
      MOZ_ASSERT(storedFileInfo.mFileInfo);

      const int64_t id = storedFileInfo.mFileInfo->Id();

      nsCOMPtr<nsIInputStream> inputStream;
      storedFileInfo.mInputStream.swap(inputStream);

      if (inputStream) {
        MOZ_ASSERT(fileDirectory);
        MOZ_ASSERT(journalDirectory);

        nsCOMPtr<nsIFile> diskFile =
          mFileManager->GetFileForId(fileDirectory, id);
        if (NS_WARN_IF(!diskFile)) {
          IDB_REPORT_INTERNAL_ERR();
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }

        bool exists;
        rv = diskFile->Exists(&exists);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          IDB_REPORT_INTERNAL_ERR();
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }

        if (exists) {
          bool isFile;
          rv = diskFile->IsFile(&isFile);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          if (NS_WARN_IF(!isFile)) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          uint64_t inputStreamSize;
          rv = inputStream->Available(&inputStreamSize);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          int64_t fileSize;
          rv = diskFile->GetFileSize(&fileSize);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          if (NS_WARN_IF(fileSize < 0)) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          if (NS_WARN_IF(uint64_t(fileSize) != inputStreamSize)) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }
        } else {
          
          nsCOMPtr<nsIFile> journalFile =
            mFileManager->GetFileForId(journalDirectory, id);
          if (NS_WARN_IF(!journalFile)) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          rv = journalFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          
          nsRefPtr<FileOutputStream> outputStream =
            FileOutputStream::Create(mPersistenceType,
                                     mGroup,
                                     mOrigin,
                                     diskFile);
          if (NS_WARN_IF(!outputStream)) {
            IDB_REPORT_INTERNAL_ERR();
            return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }

          rv = CopyFileData(inputStream, outputStream);
          if (NS_FAILED(rv) &&
              NS_ERROR_GET_MODULE(rv) != NS_ERROR_MODULE_DOM_INDEXEDDB) {
            IDB_REPORT_INTERNAL_ERR();
            rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          }
          if (NS_WARN_IF(NS_FAILED(rv))) {
            
            nsresult rv2;
            int64_t fileSize;

            if (mFileManager->EnforcingQuota()) {
              rv2 = diskFile->GetFileSize(&fileSize);
              if (NS_WARN_IF(NS_FAILED(rv2))) {
                return rv;
              }
            }

            rv2 = diskFile->Remove(false);
            if (NS_WARN_IF(NS_FAILED(rv2))) {
              return rv;
            }

            if (mFileManager->EnforcingQuota()) {
              QuotaManager* quotaManager = QuotaManager::Get();
              MOZ_ASSERT(quotaManager);

              quotaManager->DecreaseUsageForOrigin(mFileManager->Type(),
                                                   mFileManager->Group(),
                                                   mFileManager->Origin(),
                                                   fileSize);
            }

            return rv;
          }

          storedFileInfo.mCopiedSuccessfully = true;
        }
      }

      if (index) {
        fileIds.Append(' ');
      }
      fileIds.AppendInt(id);
    }

    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("file_ids"), fileIds);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("file_ids"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = stmt->Execute();
  if (rv == NS_ERROR_STORAGE_CONSTRAINT) {
    MOZ_ASSERT(!keyUnset, "Generated key had a collision!");
    return rv;
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (!mParams.indexUpdateInfos().IsEmpty()) {
    MOZ_ASSERT(mUniqueIndexTable.isSome());

    
    AutoFallibleTArray<IndexDataValue, 32> indexValues;
    rv = IndexDataValuesFromUpdateInfos(mParams.indexUpdateInfos(),
                                        mUniqueIndexTable.ref(),
                                        indexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = UpdateIndexValues(aConnection, osid, key, indexValues);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = InsertIndexTableRows(aConnection, osid, key, indexValues);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (autoIncrementNum) {
    mMetadata->mNextAutoIncrementId = autoIncrementNum + 1;
    Transaction()->NoteModifiedAutoIncrementObjectStore(mMetadata);
  }

  return NS_OK;
}

void
ObjectStoreAddOrPutRequestOp::GetResponse(RequestResponse& aResponse)
{
  AssertIsOnOwningThread();

  if (mOverwrite) {
    aResponse = ObjectStorePutResponse(mResponse);
  } else {
    aResponse = ObjectStoreAddResponse(mResponse);
  }
}

void
ObjectStoreAddOrPutRequestOp::Cleanup()
{
  AssertIsOnOwningThread();

  if (!mStoredFileInfos.IsEmpty()) {
    for (uint32_t count = mStoredFileInfos.Length(), index = 0;
         index < count;
         index++) {
      StoredFileInfo& storedFileInfo = mStoredFileInfos[index];
      nsRefPtr<DatabaseFile>& fileActor = storedFileInfo.mFileActor;

      MOZ_ASSERT_IF(!fileActor, !storedFileInfo.mCopiedSuccessfully);

      if (fileActor && storedFileInfo.mCopiedSuccessfully) {
        fileActor->ClearInputStream();
      }
    }

    mStoredFileInfos.Clear();
  }

  NormalTransactionOp::Cleanup();
}

ObjectStoreGetRequestOp::ObjectStoreGetRequestOp(TransactionBase* aTransaction,
                                                 const RequestParams& aParams,
                                                 bool aGetAll)
  : NormalTransactionOp(aTransaction)
  , mObjectStoreId(aGetAll ?
                     aParams.get_ObjectStoreGetAllParams().objectStoreId() :
                     aParams.get_ObjectStoreGetParams().objectStoreId())
  , mFileManager(aTransaction->GetDatabase()->GetFileManager())
  , mOptionalKeyRange(aGetAll ?
                        aParams.get_ObjectStoreGetAllParams()
                               .optionalKeyRange() :
                        OptionalKeyRange(aParams.get_ObjectStoreGetParams()
                                                .keyRange()))
  , mBackgroundParent(aTransaction->GetBackgroundParent())
  , mLimit(aGetAll ? aParams.get_ObjectStoreGetAllParams().limit() : 1)
  , mGetAll(aGetAll)
{
  MOZ_ASSERT(aParams.type() == RequestParams::TObjectStoreGetParams ||
             aParams.type() == RequestParams::TObjectStoreGetAllParams);
  MOZ_ASSERT(mObjectStoreId);
  MOZ_ASSERT(mFileManager);
  MOZ_ASSERT_IF(!aGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
  MOZ_ASSERT(mBackgroundParent);
}

nsresult
ObjectStoreGetRequestOp::ConvertResponse(
                             uint32_t aIndex,
                             SerializedStructuredCloneReadInfo& aSerializedInfo)
{
  MOZ_ASSERT(aIndex < mResponse.Length());

  StructuredCloneReadInfo& info = mResponse[aIndex];

  info.mData.SwapElements(aSerializedInfo.data());

  FallibleTArray<PBlobParent*> blobs;
  FallibleTArray<intptr_t> fileInfos;
  nsresult rv = ConvertBlobsToActors(mBackgroundParent,
                                     mFileManager,
                                     info.mFiles,
                                     blobs,
                                     fileInfos);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(aSerializedInfo.blobsParent().IsEmpty());
  MOZ_ASSERT(aSerializedInfo.fileInfos().IsEmpty());

  aSerializedInfo.blobsParent().SwapElements(blobs);
  aSerializedInfo.fileInfos().SwapElements(fileInfos);

  return NS_OK;
}

nsresult
ObjectStoreGetRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT_IF(!mGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
  MOZ_ASSERT_IF(!mGetAll, mLimit == 1);

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreGetRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                NS_LITERAL_CSTRING("key"),
                                keyRangeClause);
  }

  nsCString limitClause;
  if (mLimit) {
    limitClause.AssignLiteral(" LIMIT ");
    limitClause.AppendInt(mLimit);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT file_ids, data "
                       "FROM object_data "
                       "WHERE object_store_id = :osid") +
    keyRangeClause +
    NS_LITERAL_CSTRING(" ORDER BY key ASC") +
    limitClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    StructuredCloneReadInfo* cloneInfo = mResponse.AppendElement(fallible);
    if (NS_WARN_IF(!cloneInfo)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = GetStructuredCloneReadInfoFromStatement(stmt, 1, 0, mFileManager,
                                                 cloneInfo);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT_IF(!mGetAll, mResponse.Length() <= 1);

  return NS_OK;
}

void
ObjectStoreGetRequestOp::GetResponse(RequestResponse& aResponse)
{
  MOZ_ASSERT_IF(mLimit, mResponse.Length() <= mLimit);

  if (mGetAll) {
    aResponse = ObjectStoreGetAllResponse();

    if (!mResponse.IsEmpty()) {
      FallibleTArray<SerializedStructuredCloneReadInfo> fallibleCloneInfos;
      if (NS_WARN_IF(!fallibleCloneInfos.SetLength(mResponse.Length(),
                                                   fallible))) {
        aResponse = NS_ERROR_OUT_OF_MEMORY;
        return;
      }

      for (uint32_t count = mResponse.Length(), index = 0;
           index < count;
           index++) {
        nsresult rv = ConvertResponse(index, fallibleCloneInfos[index]);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          aResponse = rv;
          return;
        }
      }

      nsTArray<SerializedStructuredCloneReadInfo>& cloneInfos =
        aResponse.get_ObjectStoreGetAllResponse().cloneInfos();

      fallibleCloneInfos.SwapElements(cloneInfos);
    }

    return;
  }

  aResponse = ObjectStoreGetResponse();

  if (!mResponse.IsEmpty()) {
    SerializedStructuredCloneReadInfo& serializedInfo =
      aResponse.get_ObjectStoreGetResponse().cloneInfo();

    nsresult rv = ConvertResponse(0, serializedInfo);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResponse = rv;
    }
  }
}

nsresult
ObjectStoreGetAllKeysRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreGetAllKeysRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mParams.optionalKeyRange().type() == OptionalKeyRange::TSerializedKeyRange;

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      NS_LITERAL_CSTRING("key"),
      keyRangeClause);
  }

  nsAutoCString limitClause;
  if (uint32_t limit = mParams.limit()) {
    limitClause.AssignLiteral(" LIMIT ");
    limitClause.AppendInt(limit);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT key "
                       "FROM object_data "
                       "WHERE object_store_id = :osid") +
    keyRangeClause +
    NS_LITERAL_CSTRING(" ORDER BY key ASC") +
    limitClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"),
                             mParams.objectStoreId());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  while(NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    Key* key = mResponse.AppendElement(fallible);
    if (NS_WARN_IF(!key)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = key->SetFromStatement(stmt, 0);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

void
ObjectStoreGetAllKeysRequestOp::GetResponse(RequestResponse& aResponse)
{
  aResponse = ObjectStoreGetAllKeysResponse();

  if (!mResponse.IsEmpty()) {
    nsTArray<Key>& response =
      aResponse.get_ObjectStoreGetAllKeysResponse().keys();
    mResponse.SwapElements(response);
  }
}

ObjectStoreDeleteRequestOp::ObjectStoreDeleteRequestOp(
                                         TransactionBase* aTransaction,
                                         const ObjectStoreDeleteParams& aParams)
  : NormalTransactionOp(aTransaction)
  , mParams(aParams)
  , mObjectStoreMayHaveIndexes(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);

  nsRefPtr<FullObjectStoreMetadata> metadata =
    aTransaction->GetMetadataForObjectStoreId(mParams.objectStoreId());
  MOZ_ASSERT(metadata);

  const_cast<bool&>(mObjectStoreMayHaveIndexes) = metadata->HasLiveIndexes();
}

nsresult
ObjectStoreDeleteRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreDeleteRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool objectStoreHasIndexes;
  rv = ObjectStoreHasIndexes(this,
                             aConnection,
                             mParams.objectStoreId(),
                             mObjectStoreMayHaveIndexes,
                             &objectStoreHasIndexes);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (objectStoreHasIndexes) {
    rv = DeleteObjectStoreDataTableRowsWithIndexes(aConnection,
                                                   mParams.objectStoreId(),
                                                   mParams.keyRange());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    NS_NAMED_LITERAL_CSTRING(objectStoreIdString, "object_store_id");

    nsAutoCString keyRangeClause;
    GetBindingClauseForKeyRange(mParams.keyRange(),
                                NS_LITERAL_CSTRING("key"),
                                keyRangeClause);

    DatabaseConnection::CachedStatement stmt;
    rv = aConnection->GetCachedStatement(
      NS_LITERAL_CSTRING("DELETE FROM object_data "
                           "WHERE object_store_id = :") + objectStoreIdString +
      keyRangeClause +
      NS_LITERAL_CSTRING(";"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64ByName(objectStoreIdString, mParams.objectStoreId());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = BindKeyRangeToStatement(mParams.keyRange(), stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

ObjectStoreClearRequestOp::ObjectStoreClearRequestOp(
                                          TransactionBase* aTransaction,
                                          const ObjectStoreClearParams& aParams)
  : NormalTransactionOp(aTransaction)
  , mParams(aParams)
  , mObjectStoreMayHaveIndexes(false)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);

  nsRefPtr<FullObjectStoreMetadata> metadata =
    aTransaction->GetMetadataForObjectStoreId(mParams.objectStoreId());
  MOZ_ASSERT(metadata);

  const_cast<bool&>(mObjectStoreMayHaveIndexes) = metadata->HasLiveIndexes();
}

nsresult
ObjectStoreClearRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreClearRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  DatabaseConnection::AutoSavepoint autoSave;
  nsresult rv = autoSave.Start(Transaction());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool objectStoreHasIndexes;
  rv = ObjectStoreHasIndexes(this,
                             aConnection,
                             mParams.objectStoreId(),
                             mObjectStoreMayHaveIndexes,
                             &objectStoreHasIndexes);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (objectStoreHasIndexes) {
    rv = DeleteObjectStoreDataTableRowsWithIndexes(aConnection,
                                                   mParams.objectStoreId(),
                                                   void_t());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    DatabaseConnection::CachedStatement stmt;
    rv = aConnection->GetCachedStatement(NS_LITERAL_CSTRING(
      "DELETE FROM object_data "
        "WHERE object_store_id = :object_store_id;"),
      &stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("object_store_id"),
                               mParams.objectStoreId());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = stmt->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = autoSave.Commit();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
ObjectStoreCountRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "ObjectStoreCountRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mParams.optionalKeyRange().type() == OptionalKeyRange::TSerializedKeyRange;

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      NS_LITERAL_CSTRING("key"),
      keyRangeClause);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT count(*) "
                       "FROM object_data "
                       "WHERE object_store_id = :osid") +
    keyRangeClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"),
                             mParams.objectStoreId());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!hasResult)) {
    MOZ_ASSERT(false, "This should never be possible!");
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  int64_t count = stmt->AsInt64(0);
  if (NS_WARN_IF(count < 0)) {
    MOZ_ASSERT(false, "This should never be possible!");
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mResponse.count() = count;

  return NS_OK;
}


already_AddRefed<FullIndexMetadata>
IndexRequestOpBase::IndexMetadataForParams(TransactionBase* aTransaction,
                                           const RequestParams& aParams)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aTransaction);
  MOZ_ASSERT(aParams.type() == RequestParams::TIndexGetParams ||
             aParams.type() == RequestParams::TIndexGetKeyParams ||
             aParams.type() == RequestParams::TIndexGetAllParams ||
             aParams.type() == RequestParams::TIndexGetAllKeysParams ||
             aParams.type() == RequestParams::TIndexCountParams);

  uint64_t objectStoreId;
  uint64_t indexId;

  switch (aParams.type()) {
    case RequestParams::TIndexGetParams: {
      const IndexGetParams& params = aParams.get_IndexGetParams();
      objectStoreId = params.objectStoreId();
      indexId = params.indexId();
      break;
    }

    case RequestParams::TIndexGetKeyParams: {
      const IndexGetKeyParams& params = aParams.get_IndexGetKeyParams();
      objectStoreId = params.objectStoreId();
      indexId = params.indexId();
      break;
    }

    case RequestParams::TIndexGetAllParams: {
      const IndexGetAllParams& params = aParams.get_IndexGetAllParams();
      objectStoreId = params.objectStoreId();
      indexId = params.indexId();
      break;
    }

    case RequestParams::TIndexGetAllKeysParams: {
      const IndexGetAllKeysParams& params = aParams.get_IndexGetAllKeysParams();
      objectStoreId = params.objectStoreId();
      indexId = params.indexId();
      break;
    }

    case RequestParams::TIndexCountParams: {
      const IndexCountParams& params = aParams.get_IndexCountParams();
      objectStoreId = params.objectStoreId();
      indexId = params.indexId();
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  const nsRefPtr<FullObjectStoreMetadata> objectStoreMetadata =
    aTransaction->GetMetadataForObjectStoreId(objectStoreId);
  MOZ_ASSERT(objectStoreMetadata);

  nsRefPtr<FullIndexMetadata> indexMetadata =
    aTransaction->GetMetadataForIndexId(objectStoreMetadata, indexId);
  MOZ_ASSERT(indexMetadata);

  return indexMetadata.forget();
}

IndexGetRequestOp::IndexGetRequestOp(TransactionBase* aTransaction,
                                     const RequestParams& aParams,
                                     bool aGetAll)
  : IndexRequestOpBase(aTransaction, aParams)
  , mFileManager(aTransaction->GetDatabase()->GetFileManager())
  , mOptionalKeyRange(aGetAll ?
                        aParams.get_IndexGetAllParams().optionalKeyRange() :
                        OptionalKeyRange(aParams.get_IndexGetParams()
                                                .keyRange()))
  , mBackgroundParent(aTransaction->GetBackgroundParent())
  , mLimit(aGetAll ? aParams.get_IndexGetAllParams().limit() : 1)
  , mGetAll(aGetAll)
{
  MOZ_ASSERT(aParams.type() == RequestParams::TIndexGetParams ||
             aParams.type() == RequestParams::TIndexGetAllParams);
  MOZ_ASSERT(mFileManager);
  MOZ_ASSERT_IF(!aGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
  MOZ_ASSERT(mBackgroundParent);
}

nsresult
IndexGetRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT_IF(!mGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
  MOZ_ASSERT_IF(!mGetAll, mLimit == 1);

  PROFILER_LABEL("IndexedDB",
                 "IndexGetRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  nsCString indexTable;
  if (mMetadata->mCommonMetadata.unique()) {
    indexTable.AssignLiteral("unique_index_data ");
  }
  else {
    indexTable.AssignLiteral("index_data ");
  }

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                NS_LITERAL_CSTRING("value"),
                                keyRangeClause);
  }

  nsCString limitClause;
  if (mLimit) {
    limitClause.AssignLiteral(" LIMIT ");
    limitClause.AppendInt(mLimit);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT file_ids, data "
                       "FROM object_data "
                       "INNER JOIN ") +
    indexTable +
    NS_LITERAL_CSTRING("AS index_table "
                       "ON object_data.object_store_id = "
                         "index_table.object_store_id "
                       "AND object_data.key = "
                         "index_table.object_data_key "
                       "WHERE index_id = :index_id") +
    keyRangeClause +
    limitClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("index_id"),
                             mMetadata->mCommonMetadata.id());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  while(NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    StructuredCloneReadInfo* cloneInfo = mResponse.AppendElement(fallible);
    if (NS_WARN_IF(!cloneInfo)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = GetStructuredCloneReadInfoFromStatement(stmt, 1, 0, mFileManager,
                                                 cloneInfo);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT_IF(!mGetAll, mResponse.Length() <= 1);

  return NS_OK;
}

void
IndexGetRequestOp::GetResponse(RequestResponse& aResponse)
{
  MOZ_ASSERT_IF(!mGetAll, mResponse.Length() <= 1);

  if (mGetAll) {
    aResponse = IndexGetAllResponse();

    if (!mResponse.IsEmpty()) {
      FallibleTArray<SerializedStructuredCloneReadInfo> fallibleCloneInfos;
      if (NS_WARN_IF(!fallibleCloneInfos.SetLength(mResponse.Length(),
                                                   fallible))) {
        aResponse = NS_ERROR_OUT_OF_MEMORY;
        return;
      }

      for (uint32_t count = mResponse.Length(), index = 0;
           index < count;
           index++) {
        StructuredCloneReadInfo& info = mResponse[index];

        SerializedStructuredCloneReadInfo& serializedInfo =
          fallibleCloneInfos[index];

        info.mData.SwapElements(serializedInfo.data());

        FallibleTArray<PBlobParent*> blobs;
        FallibleTArray<intptr_t> fileInfos;
        nsresult rv = ConvertBlobsToActors(mBackgroundParent,
                                           mFileManager,
                                           info.mFiles,
                                           blobs,
                                           fileInfos);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          aResponse = rv;
          return;
        }

        MOZ_ASSERT(serializedInfo.blobsParent().IsEmpty());
        MOZ_ASSERT(serializedInfo.fileInfos().IsEmpty());

        serializedInfo.blobsParent().SwapElements(blobs);
        serializedInfo.fileInfos().SwapElements(fileInfos);
      }

      nsTArray<SerializedStructuredCloneReadInfo>& cloneInfos =
        aResponse.get_IndexGetAllResponse().cloneInfos();

      fallibleCloneInfos.SwapElements(cloneInfos);
    }

    return;
  }

  aResponse = IndexGetResponse();

  if (!mResponse.IsEmpty()) {
    StructuredCloneReadInfo& info = mResponse[0];

    SerializedStructuredCloneReadInfo& serializedInfo =
      aResponse.get_IndexGetResponse().cloneInfo();

    info.mData.SwapElements(serializedInfo.data());

    FallibleTArray<PBlobParent*> blobs;
    FallibleTArray<intptr_t> fileInfos;
    nsresult rv =
      ConvertBlobsToActors(mBackgroundParent,
                           mFileManager,
                           info.mFiles,
                           blobs,
                           fileInfos);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResponse = rv;
      return;
    }

    MOZ_ASSERT(serializedInfo.blobsParent().IsEmpty());
    MOZ_ASSERT(serializedInfo.fileInfos().IsEmpty());

    serializedInfo.blobsParent().SwapElements(blobs);
    serializedInfo.fileInfos().SwapElements(fileInfos);
  }
}

IndexGetKeyRequestOp::IndexGetKeyRequestOp(TransactionBase* aTransaction,
                                           const RequestParams& aParams,
                                           bool aGetAll)
  : IndexRequestOpBase(aTransaction, aParams)
  , mOptionalKeyRange(aGetAll ?
                        aParams.get_IndexGetAllKeysParams().optionalKeyRange() :
                        OptionalKeyRange(aParams.get_IndexGetKeyParams()
                                                .keyRange()))
  , mLimit(aGetAll ? aParams.get_IndexGetAllKeysParams().limit() : 1)
  , mGetAll(aGetAll)
{
  MOZ_ASSERT(aParams.type() == RequestParams::TIndexGetKeyParams ||
             aParams.type() == RequestParams::TIndexGetAllKeysParams);
  MOZ_ASSERT_IF(!aGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
}

nsresult
IndexGetKeyRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT_IF(!mGetAll,
                mOptionalKeyRange.type() ==
                  OptionalKeyRange::TSerializedKeyRange);
  MOZ_ASSERT_IF(!mGetAll, mLimit == 1);

  PROFILER_LABEL("IndexedDB",
                 "IndexGetKeyRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  nsCString indexTable;
  if (mMetadata->mCommonMetadata.unique()) {
    indexTable.AssignLiteral("unique_index_data ");
  }
  else {
    indexTable.AssignLiteral("index_data ");
  }

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                NS_LITERAL_CSTRING("value"),
                                keyRangeClause);
  }

  nsCString limitClause;
  if (mLimit) {
    limitClause.AssignLiteral(" LIMIT ");
    limitClause.AppendInt(mLimit);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT object_data_key "
                       "FROM ") +
    indexTable +
    NS_LITERAL_CSTRING("WHERE index_id = :index_id") +
    keyRangeClause +
    limitClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("index_id"),
                             mMetadata->mCommonMetadata.id());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  while(NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    Key* key = mResponse.AppendElement(fallible);
    if (NS_WARN_IF(!key)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = key->SetFromStatement(stmt, 0);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT_IF(!mGetAll, mResponse.Length() <= 1);

  return NS_OK;
}

void
IndexGetKeyRequestOp::GetResponse(RequestResponse& aResponse)
{
  MOZ_ASSERT_IF(!mGetAll, mResponse.Length() <= 1);

  if (mGetAll) {
    aResponse = IndexGetAllKeysResponse();

    if (!mResponse.IsEmpty()) {
      mResponse.SwapElements(aResponse.get_IndexGetAllKeysResponse().keys());
    }

    return;
  }

  aResponse = IndexGetKeyResponse();

  if (!mResponse.IsEmpty()) {
    aResponse.get_IndexGetKeyResponse().key() = Move(mResponse[0]);
  }
}

nsresult
IndexCountRequestOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();

  PROFILER_LABEL("IndexedDB",
                 "IndexCountRequestOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool hasKeyRange =
    mParams.optionalKeyRange().type() == OptionalKeyRange::TSerializedKeyRange;

  nsCString indexTable;
  if (mMetadata->mCommonMetadata.unique()) {
    indexTable.AssignLiteral("unique_index_data ");
  }
  else {
    indexTable.AssignLiteral("index_data ");
  }

  nsAutoCString keyRangeClause;
  if (hasKeyRange) {
    GetBindingClauseForKeyRange(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      NS_LITERAL_CSTRING("value"),
      keyRangeClause);
  }

  nsCString query =
    NS_LITERAL_CSTRING("SELECT count(*) "
                       "FROM ") +
    indexTable +
    NS_LITERAL_CSTRING("WHERE index_id = :index_id") +
    keyRangeClause;

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("index_id"),
                             mMetadata->mCommonMetadata.id());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasKeyRange) {
    rv = BindKeyRangeToStatement(
      mParams.optionalKeyRange().get_SerializedKeyRange(),
      stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(!hasResult)) {
    MOZ_ASSERT(false, "This should never be possible!");
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  int64_t count = stmt->AsInt64(0);
  if (NS_WARN_IF(count < 0)) {
    MOZ_ASSERT(false, "This should never be possible!");
    IDB_REPORT_INTERNAL_ERR();
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  mResponse.count() = count;

  return NS_OK;
}

bool
Cursor::
CursorOpBase::SendFailureResult(nsresult aResultCode)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aResultCode));
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mCurrentlyRunningOp == this);
  MOZ_ASSERT(!mResponseSent);

  if (!IsActorDestroyed()) {
    mResponse = ClampResultCode(aResultCode);

    mCursor->SendResponseInternal(mResponse, mFiles);
  }

  mResponseSent = true;
  return false;
}

void
Cursor::
CursorOpBase::Cleanup()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT_IF(!IsActorDestroyed(), mResponseSent);

  mCursor = nullptr;

#ifdef DEBUG
  
  
  
  NoteActorDestroyed();
#endif

  TransactionDatabaseOperationBase::Cleanup();
}

nsresult
Cursor::
CursorOpBase::PopulateResponseFromStatement(
    DatabaseConnection::CachedStatement& aStmt)
{
  Transaction()->AssertIsOnConnectionThread();
  MOZ_ASSERT(mResponse.type() == CursorResponse::T__None);
  MOZ_ASSERT(mFiles.IsEmpty());

  nsresult rv = mCursor->mKey.SetFromStatement(aStmt, 0);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  switch (mCursor->mType) {
    case OpenCursorParams::TObjectStoreOpenCursorParams: {
      StructuredCloneReadInfo cloneInfo;
      rv = GetStructuredCloneReadInfoFromStatement(aStmt,
                                                   2,
                                                   1,
                                                   mCursor->mFileManager,
                                                   &cloneInfo);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mResponse = ObjectStoreCursorResponse();

      auto& response = mResponse.get_ObjectStoreCursorResponse();
      response.cloneInfo().data().SwapElements(cloneInfo.mData);
      response.key() = mCursor->mKey;

      mFiles.SwapElements(cloneInfo.mFiles);
      break;
    }

    case OpenCursorParams::TObjectStoreOpenKeyCursorParams: {
      mResponse = ObjectStoreKeyCursorResponse(mCursor->mKey);
      break;
    }

    case OpenCursorParams::TIndexOpenCursorParams: {
      rv = mCursor->mObjectKey.SetFromStatement(aStmt, 1);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      StructuredCloneReadInfo cloneInfo;
      rv = GetStructuredCloneReadInfoFromStatement(aStmt,
                                                   3,
                                                   2,
                                                   mCursor->mFileManager,
                                                   &cloneInfo);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mResponse = IndexCursorResponse();

      auto& response = mResponse.get_IndexCursorResponse();
      response.cloneInfo().data().SwapElements(cloneInfo.mData);
      response.key() = mCursor->mKey;
      response.objectKey() = mCursor->mObjectKey;

      mFiles.SwapElements(cloneInfo.mFiles);
      break;
    }

    case OpenCursorParams::TIndexOpenKeyCursorParams: {
      rv = mCursor->mObjectKey.SetFromStatement(aStmt, 1);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mResponse = IndexKeyCursorResponse(mCursor->mKey, mCursor->mObjectKey);
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  return NS_OK;
}

void
Cursor::
OpenOp::GetRangeKeyInfo(bool aLowerBound, Key* aKey, bool* aOpen)
{
  AssertIsOnConnectionThread();
  MOZ_ASSERT(aKey);
  MOZ_ASSERT(aKey->IsUnset());
  MOZ_ASSERT(aOpen);

  if (mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange) {
    const SerializedKeyRange& range =
      mOptionalKeyRange.get_SerializedKeyRange();
    if (range.isOnly()) {
      *aKey = range.lower();
      *aOpen = false;
    } else {
      *aKey = aLowerBound ? range.lower() : range.upper();
      *aOpen = aLowerBound ? range.lowerOpen() : range.upperOpen();
    }
  } else {
    *aOpen = false;
  }
}

nsresult
Cursor::
OpenOp::DoObjectStoreDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mType == OpenCursorParams::TObjectStoreOpenCursorParams);
  MOZ_ASSERT(mCursor->mObjectStoreId);
  MOZ_ASSERT(mCursor->mFileManager);

  PROFILER_LABEL("IndexedDB",
                 "Cursor::OpenOp::DoObjectStoreDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool usingKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  NS_NAMED_LITERAL_CSTRING(keyString, "key");
  NS_NAMED_LITERAL_CSTRING(id, "id");
  NS_NAMED_LITERAL_CSTRING(openLimit, " LIMIT ");

  nsCString queryStart =
    NS_LITERAL_CSTRING("SELECT ") +
    keyString +
    NS_LITERAL_CSTRING(", file_ids, data "
                       "FROM object_data "
                       "WHERE object_store_id = :") +
    id;

  nsAutoCString keyRangeClause;
  if (usingKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                keyString,
                                keyRangeClause);
  }

  nsAutoCString directionClause = NS_LITERAL_CSTRING(" ORDER BY ") + keyString;
  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE:
      directionClause.AppendLiteral(" ASC");
      break;

    case IDBCursor::PREV:
    case IDBCursor::PREV_UNIQUE:
      directionClause.AppendLiteral(" DESC");
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  
  
  nsCString firstQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit +
    NS_LITERAL_CSTRING("1");

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(firstQuery, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(id, mCursor->mObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (usingKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!hasResult) {
    mResponse = void_t();
    return NS_OK;
  }

  rv = PopulateResponseFromStatement(stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  keyRangeClause.Truncate();
  nsAutoCString continueToKeyRangeClause;

  NS_NAMED_LITERAL_CSTRING(currentKey, "current_key");
  NS_NAMED_LITERAL_CSTRING(rangeKey, "range_key");

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      AppendConditionClause(keyString, currentKey, false, false,
                            keyRangeClause);
      AppendConditionClause(keyString, currentKey, false, true,
                            continueToKeyRangeClause);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(keyString, rangeKey, true, !open, keyRangeClause);
        AppendConditionClause(keyString, rangeKey, true, !open,
                              continueToKeyRangeClause);
        mCursor->mRangeKey = upper;
      }
      break;
    }

    case IDBCursor::PREV:
    case IDBCursor::PREV_UNIQUE: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      AppendConditionClause(keyString, currentKey, true, false, keyRangeClause);
      AppendConditionClause(keyString, currentKey, true, true,
                           continueToKeyRangeClause);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(keyString, rangeKey, false, !open,
                              keyRangeClause);
        AppendConditionClause(keyString, rangeKey, false, !open,
                              continueToKeyRangeClause);
        mCursor->mRangeKey = lower;
      }
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  mCursor->mContinueQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit;

  mCursor->mContinueToQuery =
    queryStart +
    continueToKeyRangeClause +
    directionClause +
    openLimit;

  return NS_OK;
}

nsresult
Cursor::
OpenOp::DoObjectStoreKeyDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mType ==
               OpenCursorParams::TObjectStoreOpenKeyCursorParams);
  MOZ_ASSERT(mCursor->mObjectStoreId);

  PROFILER_LABEL("IndexedDB",
                 "Cursor::OpenOp::DoObjectStoreKeyDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool usingKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  NS_NAMED_LITERAL_CSTRING(keyString, "key");
  NS_NAMED_LITERAL_CSTRING(id, "id");
  NS_NAMED_LITERAL_CSTRING(openLimit, " LIMIT ");

  nsCString queryStart =
    NS_LITERAL_CSTRING("SELECT ") +
    keyString +
    NS_LITERAL_CSTRING(" FROM object_data "
                       "WHERE object_store_id = :") +
    id;

  nsAutoCString keyRangeClause;
  if (usingKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                keyString,
                                keyRangeClause);
  }

  nsAutoCString directionClause = NS_LITERAL_CSTRING(" ORDER BY ") + keyString;
  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE:
      directionClause.AppendLiteral(" ASC");
      break;

    case IDBCursor::PREV:
    case IDBCursor::PREV_UNIQUE:
      directionClause.AppendLiteral(" DESC");
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  
  
  nsCString firstQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit +
    NS_LITERAL_CSTRING("1");

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(firstQuery, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(id, mCursor->mObjectStoreId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (usingKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!hasResult) {
    mResponse = void_t();
    return NS_OK;
  }

  rv = PopulateResponseFromStatement(stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  keyRangeClause.Truncate();
  nsAutoCString continueToKeyRangeClause;

  NS_NAMED_LITERAL_CSTRING(currentKey, "current_key");
  NS_NAMED_LITERAL_CSTRING(rangeKey, "range_key");

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      AppendConditionClause(keyString, currentKey, false, false,
                            keyRangeClause);
      AppendConditionClause(keyString, currentKey, false, true,
                            continueToKeyRangeClause);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(keyString, rangeKey, true, !open, keyRangeClause);
        AppendConditionClause(keyString, rangeKey, true, !open,
                              continueToKeyRangeClause);
        mCursor->mRangeKey = upper;
      }
      break;
    }

    case IDBCursor::PREV:
    case IDBCursor::PREV_UNIQUE: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      AppendConditionClause(keyString, currentKey, true, false, keyRangeClause);
      AppendConditionClause(keyString, currentKey, true, true,
                            continueToKeyRangeClause);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(keyString, rangeKey, false, !open,
                              keyRangeClause);
        AppendConditionClause(keyString, rangeKey, false, !open,
                              continueToKeyRangeClause);
        mCursor->mRangeKey = lower;
      }
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  mCursor->mContinueQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit;
  mCursor->mContinueToQuery =
    queryStart +
    continueToKeyRangeClause +
    directionClause +
    openLimit;

  return NS_OK;
}

nsresult
Cursor::
OpenOp::DoIndexDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mType == OpenCursorParams::TIndexOpenCursorParams);
  MOZ_ASSERT(mCursor->mObjectStoreId);
  MOZ_ASSERT(mCursor->mIndexId);

  PROFILER_LABEL("IndexedDB",
                 "Cursor::OpenOp::DoIndexDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool usingKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  nsCString indexTable = mCursor->mUniqueIndex ?
    NS_LITERAL_CSTRING("unique_index_data") :
    NS_LITERAL_CSTRING("index_data");

  NS_NAMED_LITERAL_CSTRING(value, "index_table.value");
  NS_NAMED_LITERAL_CSTRING(id, "id");
  NS_NAMED_LITERAL_CSTRING(openLimit, " LIMIT ");

  nsAutoCString keyRangeClause;
  if (usingKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                value,
                                keyRangeClause);
  }

  nsAutoCString directionClause =
    NS_LITERAL_CSTRING(" ORDER BY ") +
    value;

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE:
      directionClause.AppendLiteral(" ASC, index_table.object_data_key ASC");
      break;

    case IDBCursor::PREV:
      directionClause.AppendLiteral(" DESC, index_table.object_data_key DESC");
      break;

    case IDBCursor::PREV_UNIQUE:
      directionClause.AppendLiteral(" DESC, index_table.object_data_key ASC");
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  nsAutoCString queryStart =
    NS_LITERAL_CSTRING("SELECT index_table.value, "
                              "index_table.object_data_key, "
                              "object_data.file_ids, "
                              "object_data.data "
                       "FROM ") +
    indexTable +
    NS_LITERAL_CSTRING(" AS index_table "
                       "JOIN object_data "
                       "ON index_table.object_store_id = "
                         "object_data.object_store_id "
                       "AND index_table.object_data_key = "
                         "object_data.key "
                       "WHERE index_table.index_id = :") +
    id;

  
  
  nsCString firstQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit +
    NS_LITERAL_CSTRING("1");

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(firstQuery, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(id, mCursor->mIndexId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (usingKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!hasResult) {
    mResponse = void_t();
    return NS_OK;
  }

  rv = PopulateResponseFromStatement(stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  NS_NAMED_LITERAL_CSTRING(rangeKey, "range_key");

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(value, rangeKey, true, !open, queryStart);
        mCursor->mRangeKey = upper;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value >= :current_key "
                            "AND ( index_table.value > :current_key OR "
                                  "index_table.object_data_key > :object_key ) "
                          ) +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value >= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::NEXT_UNIQUE: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(value, rangeKey, true, !open, queryStart);
        mCursor->mRangeKey = upper;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value > :current_key") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value >= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::PREV: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(value, rangeKey, false, !open, queryStart);
        mCursor->mRangeKey = lower;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value <= :current_key "
                            "AND ( index_table.value < :current_key OR "
                                  "index_table.object_data_key < :object_key ) "
                          ) +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value <= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::PREV_UNIQUE: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(value, rangeKey, false, !open, queryStart);
        mCursor->mRangeKey = lower;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value < :current_key") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND index_table.value <= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  return NS_OK;
}

nsresult
Cursor::
OpenOp::DoIndexKeyDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mType == OpenCursorParams::TIndexOpenKeyCursorParams);
  MOZ_ASSERT(mCursor->mObjectStoreId);
  MOZ_ASSERT(mCursor->mIndexId);

  PROFILER_LABEL("IndexedDB",
                 "Cursor::OpenOp::DoIndexKeyDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  const bool usingKeyRange =
    mOptionalKeyRange.type() == OptionalKeyRange::TSerializedKeyRange;

  nsCString table = mCursor->mUniqueIndex ?
    NS_LITERAL_CSTRING("unique_index_data") :
    NS_LITERAL_CSTRING("index_data");

  NS_NAMED_LITERAL_CSTRING(value, "value");
  NS_NAMED_LITERAL_CSTRING(id, "id");
  NS_NAMED_LITERAL_CSTRING(openLimit, " LIMIT ");

  nsAutoCString keyRangeClause;
  if (usingKeyRange) {
    GetBindingClauseForKeyRange(mOptionalKeyRange.get_SerializedKeyRange(),
                                value,
                                keyRangeClause);
  }

  nsAutoCString directionClause =
    NS_LITERAL_CSTRING(" ORDER BY ") +
    value;

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT:
    case IDBCursor::NEXT_UNIQUE:
      directionClause.AppendLiteral(" ASC, object_data_key ASC");
      break;

    case IDBCursor::PREV:
      directionClause.AppendLiteral(" DESC, object_data_key DESC");
      break;

    case IDBCursor::PREV_UNIQUE:
      directionClause.AppendLiteral(" DESC, object_data_key ASC");
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  nsAutoCString queryStart =
    NS_LITERAL_CSTRING("SELECT value, object_data_key "
                       "FROM ") +
    table +
    NS_LITERAL_CSTRING(" WHERE index_id = :") +
    id;

  
  
  nsCString firstQuery =
    queryStart +
    keyRangeClause +
    directionClause +
    openLimit +
    NS_LITERAL_CSTRING("1");

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(firstQuery, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = stmt->BindInt64ByName(id, mCursor->mIndexId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (usingKeyRange) {
    rv = BindKeyRangeToStatement(mOptionalKeyRange.get_SerializedKeyRange(),
                                 stmt);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!hasResult) {
    mResponse = void_t();
    return NS_OK;
  }

  rv = PopulateResponseFromStatement(stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  NS_NAMED_LITERAL_CSTRING(rangeKey, "range_key");

  switch (mCursor->mDirection) {
    case IDBCursor::NEXT: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(value, rangeKey, true, !open, queryStart);
        mCursor->mRangeKey = upper;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value >= :current_key "
                            "AND ( value > :current_key OR "
                                  "object_data_key > :object_key )") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value >= :current_key ") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::NEXT_UNIQUE: {
      Key upper;
      bool open;
      GetRangeKeyInfo(false, &upper, &open);
      if (usingKeyRange && !upper.IsUnset()) {
        AppendConditionClause(value, rangeKey, true, !open, queryStart);
        mCursor->mRangeKey = upper;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value > :current_key") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value >= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::PREV: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(value, rangeKey, false, !open, queryStart);
        mCursor->mRangeKey = lower;
      }

      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value <= :current_key "
                            "AND ( value < :current_key OR "
                                  "object_data_key < :object_key )") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value <= :current_key ") +
        directionClause +
        openLimit;
      break;
    }

    case IDBCursor::PREV_UNIQUE: {
      Key lower;
      bool open;
      GetRangeKeyInfo(true, &lower, &open);
      if (usingKeyRange && !lower.IsUnset()) {
        AppendConditionClause(value, rangeKey, false, !open, queryStart);
        mCursor->mRangeKey = lower;
      }
      mCursor->mContinueQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value < :current_key") +
        directionClause +
        openLimit;
      mCursor->mContinueToQuery =
        queryStart +
        NS_LITERAL_CSTRING(" AND value <= :current_key") +
        directionClause +
        openLimit;
      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  return NS_OK;
}

nsresult
Cursor::
OpenOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mContinueQuery.IsEmpty());
  MOZ_ASSERT(mCursor->mContinueToQuery.IsEmpty());
  MOZ_ASSERT(mCursor->mKey.IsUnset());
  MOZ_ASSERT(mCursor->mRangeKey.IsUnset());

  PROFILER_LABEL("IndexedDB",
                 "Cursor::OpenOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  nsresult rv;

  switch (mCursor->mType) {
    case OpenCursorParams::TObjectStoreOpenCursorParams:
      rv = DoObjectStoreDatabaseWork(aConnection);
      break;

    case OpenCursorParams::TObjectStoreOpenKeyCursorParams:
      rv = DoObjectStoreKeyDatabaseWork(aConnection);
      break;

    case OpenCursorParams::TIndexOpenCursorParams:
      rv = DoIndexDatabaseWork(aConnection);
      break;

    case OpenCursorParams::TIndexOpenKeyCursorParams:
      rv = DoIndexKeyDatabaseWork(aConnection);
      break;

    default:
      MOZ_CRASH("Should never get here!");
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
Cursor::
OpenOp::SendSuccessResult()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mCurrentlyRunningOp == this);
  MOZ_ASSERT(mResponse.type() != CursorResponse::T__None);
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mKey.IsUnset());
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mRangeKey.IsUnset());
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mObjectKey.IsUnset());

  if (IsActorDestroyed()) {
    return NS_ERROR_DOM_INDEXEDDB_ABORT_ERR;
  }

  mCursor->SendResponseInternal(mResponse, mFiles);

  mResponseSent = true;
  return NS_OK;
}

nsresult
Cursor::
ContinueOp::DoDatabaseWork(DatabaseConnection* aConnection)
{
  MOZ_ASSERT(aConnection);
  aConnection->AssertIsOnConnectionThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mObjectStoreId);
  MOZ_ASSERT(!mCursor->mContinueQuery.IsEmpty());
  MOZ_ASSERT(!mCursor->mContinueToQuery.IsEmpty());
  MOZ_ASSERT(!mCursor->mKey.IsUnset());

  const bool isIndex =
    mCursor->mType == OpenCursorParams::TIndexOpenCursorParams ||
    mCursor->mType == OpenCursorParams::TIndexOpenKeyCursorParams;

  MOZ_ASSERT_IF(isIndex, mCursor->mIndexId);
  MOZ_ASSERT_IF(isIndex, !mCursor->mObjectKey.IsUnset());

  PROFILER_LABEL("IndexedDB",
                 "Cursor::ContinueOp::DoDatabaseWork",
                 js::ProfileEntry::Category::STORAGE);

  
  
  
  
  
  

  
  
  nsCString query;
  nsAutoCString countString;

  bool hasContinueKey = false;
  uint32_t advanceCount;

  if (mParams.type() == CursorRequestParams::TContinueParams) {
    
    advanceCount = 1;
    countString.AppendLiteral("1");

    if (mParams.get_ContinueParams().key().IsUnset()) {
      query = mCursor->mContinueQuery + countString;
      hasContinueKey = false;
    } else {
      query = mCursor->mContinueToQuery + countString;
      hasContinueKey = true;
    }
  } else {
    advanceCount = mParams.get_AdvanceParams().count();
    MOZ_ASSERT(advanceCount > 0);
    countString.AppendInt(advanceCount);

    query = mCursor->mContinueQuery + countString;
    hasContinueKey = false;
  }

  NS_NAMED_LITERAL_CSTRING(currentKeyName, "current_key");
  NS_NAMED_LITERAL_CSTRING(rangeKeyName, "range_key");
  NS_NAMED_LITERAL_CSTRING(objectKeyName, "object_key");

  const Key& currentKey =
    hasContinueKey ? mParams.get_ContinueParams().key() : mCursor->mKey;

  const bool usingRangeKey = !mCursor->mRangeKey.IsUnset();

  DatabaseConnection::CachedStatement stmt;
  nsresult rv = aConnection->GetCachedStatement(query, &stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  int64_t id = isIndex ? mCursor->mIndexId : mCursor->mObjectStoreId;

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), id);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = currentKey.BindToStatement(stmt, currentKeyName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (usingRangeKey) {
    rv = mCursor->mRangeKey.BindToStatement(stmt, rangeKeyName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  
  if (isIndex &&
      !hasContinueKey &&
      (mCursor->mDirection == IDBCursor::NEXT ||
       mCursor->mDirection == IDBCursor::PREV)) {
    rv = mCursor->mObjectKey.BindToStatement(stmt, objectKeyName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  bool hasResult;
  for (uint32_t index = 0; index < advanceCount; index++) {
    rv = stmt->ExecuteStep(&hasResult);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!hasResult) {
      mCursor->mKey.Unset();
      mCursor->mRangeKey.Unset();
      mCursor->mObjectKey.Unset();
      mResponse = void_t();
      return NS_OK;
    }
  }

  rv = PopulateResponseFromStatement(stmt);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
Cursor::
ContinueOp::SendSuccessResult()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mCursor);
  MOZ_ASSERT(mCursor->mCurrentlyRunningOp == this);
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mKey.IsUnset());
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mRangeKey.IsUnset());
  MOZ_ASSERT_IF(mResponse.type() == CursorResponse::Tvoid_t,
                mCursor->mObjectKey.IsUnset());

  if (IsActorDestroyed()) {
    return NS_ERROR_DOM_INDEXEDDB_ABORT_ERR;
  }

  mCursor->SendResponseInternal(mResponse, mFiles);

  mResponseSent = true;
  return NS_OK;
}

void
PermissionRequestHelper::OnPromptComplete(PermissionValue aPermissionValue)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mActorDestroyed) {
    unused <<
      PIndexedDBPermissionRequestParent::Send__delete__(this, aPermissionValue);
  }
}

void
PermissionRequestHelper::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mActorDestroyed);

  mActorDestroyed = true;
}

#ifdef DEBUG

NS_IMPL_ISUPPORTS(DEBUGThreadSlower, nsIThreadObserver)

NS_IMETHODIMP
DEBUGThreadSlower::OnDispatchedEvent(nsIThreadInternal* )
{
  MOZ_CRASH("Should never be called!");
}

NS_IMETHODIMP
DEBUGThreadSlower::OnProcessNextEvent(nsIThreadInternal* ,
                                      bool ,
                                      uint32_t )
{
  return NS_OK;
}

NS_IMETHODIMP
DEBUGThreadSlower::AfterProcessNextEvent(nsIThreadInternal* ,
                                         uint32_t ,
                                         bool )
{
  MOZ_ASSERT(kDEBUGThreadSleepMS);

  MOZ_ALWAYS_TRUE(PR_Sleep(PR_MillisecondsToInterval(kDEBUGThreadSleepMS)) ==
                    PR_SUCCESS);
  return NS_OK;
}

#endif 

} 
} 
} 

#undef IDB_MOBILE
#undef IDB_DEBUG_LOG
#undef ASSERT_UNLESS_FUZZING
#undef DISABLE_ASSERTS_FOR_FUZZING
