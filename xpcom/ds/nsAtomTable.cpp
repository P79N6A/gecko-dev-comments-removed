





#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/unused.h"

#include "nsAtomTable.h"
#include "nsStaticAtom.h"
#include "nsString.h"
#include "nsCRT.h"
#include "pldhash.h"
#include "prenv.h"
#include "nsThreadUtils.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "nsUnicharUtils.h"

using namespace mozilla;

#if defined(__clang__)
#  pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#elif MOZ_IS_GCC
#  if MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
#    pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#  endif
#endif










static PLDHashTable gAtomTable;

class StaticAtomEntry : public PLDHashEntryHdr
{
public:
  typedef const nsAString& KeyType;
  typedef const nsAString* KeyTypePointer;

  explicit StaticAtomEntry(KeyTypePointer aKey) {}
  StaticAtomEntry(const StaticAtomEntry& aOther) : mAtom(aOther.mAtom) {}
  ~StaticAtomEntry() {}

  bool KeyEquals(KeyTypePointer aKey) const
  {
    
    return mAtom->Equals(*aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return HashString(*aKey);
  }

  enum { ALLOW_MEMMOVE = true };

  nsIAtom* mAtom;
};





typedef nsTHashtable<StaticAtomEntry> StaticAtomTable;
static StaticAtomTable* gStaticAtomTable = nullptr;




static bool gStaticAtomTableSealed = false;








class AtomImpl : public nsIAtom
{
public:
  AtomImpl(const nsAString& aString, uint32_t aHash);

  
  
  AtomImpl(nsStringBuffer* aData, uint32_t aLength, uint32_t aHash);

protected:
  
  
  AtomImpl()
  {
    
    
    NS_ASSERTION((mLength + 1) * sizeof(char16_t) <=
                 nsStringBuffer::FromData(mString)->StorageSize() &&
                 mString[mLength] == 0,
                 "Not initialized atom");
  }

  
  
  ~AtomImpl();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIATOM

  enum { REFCNT_PERMANENT_SENTINEL = UINT32_MAX };

  virtual bool IsPermanent();

  
  bool IsPermanentInDestructor()
  {
    return mRefCnt == REFCNT_PERMANENT_SENTINEL;
  }

  
  nsrefcnt GetRefCount() { return mRefCnt; }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf);
};





class PermanentAtomImpl MOZ_FINAL : public AtomImpl
{
public:
  PermanentAtomImpl(const nsAString& aString, PLDHashNumber aKeyHash)
    : AtomImpl(aString, aKeyHash)
  {
  }
  PermanentAtomImpl(nsStringBuffer* aData, uint32_t aLength,
                    PLDHashNumber aKeyHash)
    : AtomImpl(aData, aLength, aKeyHash)
  {
  }
  PermanentAtomImpl() {}

  ~PermanentAtomImpl();
  NS_IMETHOD_(MozExternalRefCountType) AddRef();
  NS_IMETHOD_(MozExternalRefCountType) Release();

  virtual bool IsPermanent();

  
  

  void* operator new(size_t aSize, AtomImpl* aAtom) CPP_THROW_NEW;
  void* operator new(size_t aSize) CPP_THROW_NEW
  {
    return ::operator new(aSize);
  }
};



struct AtomTableEntry : public PLDHashEntryHdr
{
  AtomImpl* mAtom;
};

struct AtomTableKey
{
  AtomTableKey(const char16_t* aUTF16String, uint32_t aLength, uint32_t aHash)
    : mUTF16String(aUTF16String)
    , mUTF8String(nullptr)
    , mLength(aLength)
    , mHash(aHash)
  {
    MOZ_ASSERT(mHash == HashString(mUTF16String, mLength));
  }

  AtomTableKey(const char* aUTF8String, uint32_t aLength, uint32_t aHash)
    : mUTF16String(nullptr)
    , mUTF8String(aUTF8String)
    , mLength(aLength)
    , mHash(aHash)
  {
    mozilla::DebugOnly<bool> err;
    MOZ_ASSERT(aHash == HashUTF8AsUTF16(mUTF8String, mLength, &err));
  }

  AtomTableKey(const char16_t* aUTF16String, uint32_t aLength,
               uint32_t* aHashOut)
    : mUTF16String(aUTF16String)
    , mUTF8String(nullptr)
    , mLength(aLength)
  {
    mHash = HashString(mUTF16String, mLength);
    *aHashOut = mHash;
  }

  AtomTableKey(const char* aUTF8String, uint32_t aLength, uint32_t* aHashOut)
    : mUTF16String(nullptr)
    , mUTF8String(aUTF8String)
    , mLength(aLength)
  {
    bool err;
    mHash = HashUTF8AsUTF16(mUTF8String, mLength, &err);
    if (err) {
      mUTF8String = nullptr;
      mLength = 0;
      mHash = 0;
    }
    *aHashOut = mHash;
  }

  const char16_t* mUTF16String;
  const char* mUTF8String;
  uint32_t mLength;
  uint32_t mHash;
};

static PLDHashNumber
AtomTableGetHash(PLDHashTable* aTable, const void* aKey)
{
  const AtomTableKey* k = static_cast<const AtomTableKey*>(aKey);
  return k->mHash;
}

static bool
AtomTableMatchKey(PLDHashTable* aTable, const PLDHashEntryHdr* aEntry,
                  const void* aKey)
{
  const AtomTableEntry* he = static_cast<const AtomTableEntry*>(aEntry);
  const AtomTableKey* k = static_cast<const AtomTableKey*>(aKey);

  if (k->mUTF8String) {
    return
      CompareUTF8toUTF16(nsDependentCSubstring(k->mUTF8String,
                                               k->mUTF8String + k->mLength),
                         nsDependentAtomString(he->mAtom)) == 0;
  }

  uint32_t length = he->mAtom->GetLength();
  if (length != k->mLength) {
    return false;
  }

  return memcmp(he->mAtom->GetUTF16String(),
                k->mUTF16String, length * sizeof(char16_t)) == 0;
}

static void
AtomTableClearEntry(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  
  
  
  
  
  

  AtomImpl* atom = static_cast<AtomTableEntry*>(aEntry)->mAtom;
  if (atom->IsPermanent()) {
    
    
    delete static_cast<PermanentAtomImpl*>(atom);
  }
}

static bool
AtomTableInitEntry(PLDHashTable* aTable, PLDHashEntryHdr* aEntry,
                   const void* aKey)
{
  static_cast<AtomTableEntry*>(aEntry)->mAtom = nullptr;

  return true;
}


static const PLDHashTableOps AtomTableOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  AtomTableGetHash,
  AtomTableMatchKey,
  PL_DHashMoveEntryStub,
  AtomTableClearEntry,
  PL_DHashFinalizeStub,
  AtomTableInitEntry
};


#ifdef DEBUG
static PLDHashOperator
DumpAtomLeaks(PLDHashTable* aTable, PLDHashEntryHdr* aEntryHdr,
              uint32_t aIndex, void* aArg)
{
  AtomTableEntry* entry = static_cast<AtomTableEntry*>(aEntryHdr);
  AtomImpl* atom = entry->mAtom;
  if (!atom->IsPermanent()) {
    ++*static_cast<uint32_t*>(aArg);
    nsAutoCString str;
    atom->ToUTF8String(str);
    fputs(str.get(), stdout);
    fputs("\n", stdout);
  }
  return PL_DHASH_NEXT;
}
#endif

static inline
void
PromoteToPermanent(AtomImpl* aAtom)
{
#ifdef NS_BUILD_REFCNT_LOGGING
  {
    nsrefcnt refcount = aAtom->GetRefCount();
    do {
      NS_LOG_RELEASE(aAtom, --refcount, "AtomImpl");
    } while (refcount);
  }
#endif
  aAtom = new (aAtom) PermanentAtomImpl();
}

void
NS_PurgeAtomTable()
{
  delete gStaticAtomTable;

  if (gAtomTable.ops) {
#ifdef DEBUG
    const char* dumpAtomLeaks = PR_GetEnv("MOZ_DUMP_ATOM_LEAKS");
    if (dumpAtomLeaks && *dumpAtomLeaks) {
      uint32_t leaked = 0;
      printf("*** %d atoms still exist (including permanent):\n",
             gAtomTable.entryCount);
      PL_DHashTableEnumerate(&gAtomTable, DumpAtomLeaks, &leaked);
      printf("*** %u non-permanent atoms leaked\n", leaked);
    }
#endif
    PL_DHashTableFinish(&gAtomTable);
    gAtomTable.entryCount = 0;
    gAtomTable.ops = nullptr;
  }
}

AtomImpl::AtomImpl(const nsAString& aString, uint32_t aHash)
{
  mLength = aString.Length();
  nsRefPtr<nsStringBuffer> buf = nsStringBuffer::FromString(aString);
  if (buf) {
    mString = static_cast<char16_t*>(buf->Data());
  } else {
    buf = nsStringBuffer::Alloc((mLength + 1) * sizeof(char16_t));
    mString = static_cast<char16_t*>(buf->Data());
    CopyUnicodeTo(aString, 0, mString, mLength);
    mString[mLength] = char16_t(0);
  }

  mHash = aHash;
  MOZ_ASSERT(mHash == HashString(mString, mLength));

  NS_ASSERTION(mString[mLength] == char16_t(0), "null terminated");
  NS_ASSERTION(buf && buf->StorageSize() >= (mLength + 1) * sizeof(char16_t),
               "enough storage");
  NS_ASSERTION(Equals(aString), "correct data");

  
  mozilla::unused << buf.forget();
}

AtomImpl::AtomImpl(nsStringBuffer* aStringBuffer, uint32_t aLength,
                   uint32_t aHash)
{
  mLength = aLength;
  mString = static_cast<char16_t*>(aStringBuffer->Data());
  
  
  aStringBuffer->AddRef();

  mHash = aHash;
  MOZ_ASSERT(mHash == HashString(mString, mLength));

  NS_ASSERTION(mString[mLength] == char16_t(0), "null terminated");
  NS_ASSERTION(aStringBuffer &&
               aStringBuffer->StorageSize() == (mLength + 1) * sizeof(char16_t),
               "correct storage");
}

AtomImpl::~AtomImpl()
{
  NS_PRECONDITION(gAtomTable.ops, "uninitialized atom hashtable");
  
  
  
  if (!IsPermanentInDestructor()) {
    AtomTableKey key(mString, mLength, mHash);
    PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_REMOVE);
    if (gAtomTable.entryCount == 0) {
      PL_DHashTableFinish(&gAtomTable);
      NS_ASSERTION(gAtomTable.entryCount == 0,
                   "PL_DHashTableFinish changed the entry count");
    }
  }

  nsStringBuffer::FromData(mString)->Release();
}

NS_IMPL_ISUPPORTS(AtomImpl, nsIAtom)

PermanentAtomImpl::~PermanentAtomImpl()
{
  
  mRefCnt = REFCNT_PERMANENT_SENTINEL;
}

NS_IMETHODIMP_(MozExternalRefCountType)
PermanentAtomImpl::AddRef()
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  return 2;
}

NS_IMETHODIMP_(MozExternalRefCountType)
PermanentAtomImpl::Release()
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  return 1;
}

 bool
AtomImpl::IsPermanent()
{
  return false;
}

 bool
PermanentAtomImpl::IsPermanent()
{
  return true;
}

void*
PermanentAtomImpl::operator new(size_t aSize, AtomImpl* aAtom) CPP_THROW_NEW
{
  MOZ_ASSERT(!aAtom->IsPermanent(),
             "converting atom that's already permanent");

  
  return aAtom;
}

NS_IMETHODIMP
AtomImpl::ScriptableToString(nsAString& aBuf)
{
  nsStringBuffer::FromData(mString)->ToString(mLength, aBuf);
  return NS_OK;
}

NS_IMETHODIMP
AtomImpl::ToUTF8String(nsACString& aBuf)
{
  CopyUTF16toUTF8(nsDependentString(mString, mLength), aBuf);
  return NS_OK;
}

NS_IMETHODIMP_(bool)
AtomImpl::EqualsUTF8(const nsACString& aString)
{
  return CompareUTF8toUTF16(aString,
                            nsDependentString(mString, mLength)) == 0;
}

NS_IMETHODIMP
AtomImpl::ScriptableEquals(const nsAString& aString, bool* aResult)
{
  *aResult = aString.Equals(nsDependentString(mString, mLength));
  return NS_OK;
}

NS_IMETHODIMP_(bool)
AtomImpl::IsStaticAtom()
{
  return IsPermanent();
}

size_t
AtomImpl::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
{
  size_t n = aMallocSizeOf(this);

  
  
  
  
  if (!IsStaticAtom()) {
    n += nsStringBuffer::FromData(mString)->SizeOfIncludingThisIfUnshared(
           aMallocSizeOf);
  }
  return n;
}



static size_t
SizeOfAtomTableEntryExcludingThis(PLDHashEntryHdr* aHdr,
                                  MallocSizeOf aMallocSizeOf,
                                  void* aArg)
{
  AtomTableEntry* entry = static_cast<AtomTableEntry*>(aHdr);
  return entry->mAtom->SizeOfIncludingThis(aMallocSizeOf);
}

void
NS_SizeOfAtomTablesIncludingThis(MallocSizeOf aMallocSizeOf,
                                 size_t* aMain, size_t* aStatic)
{
  *aMain = gAtomTable.ops
         ? PL_DHashTableSizeOfExcludingThis(&gAtomTable,
                                            SizeOfAtomTableEntryExcludingThis,
                                            aMallocSizeOf)
         : 0;

  
  
  *aStatic = gStaticAtomTable
           ? gStaticAtomTable->SizeOfIncludingThis(nullptr, aMallocSizeOf)
           : 0;
}

#define ATOM_HASHTABLE_INITIAL_SIZE  4096

static inline void
EnsureTableExists()
{
  if (!gAtomTable.ops) {
    PL_DHashTableInit(&gAtomTable, &AtomTableOps, 0,
                      sizeof(AtomTableEntry), ATOM_HASHTABLE_INITIAL_SIZE);
  }
}

static inline AtomTableEntry*
GetAtomHashEntry(const char* aString, uint32_t aLength, uint32_t* aHashOut)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  EnsureTableExists();
  AtomTableKey key(aString, aLength, aHashOut);
  AtomTableEntry* e = static_cast<AtomTableEntry*>(
    PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_ADD));
  if (!e) {
    NS_ABORT_OOM(gAtomTable.entryCount * gAtomTable.entrySize);
  }
  return e;
}

static inline AtomTableEntry*
GetAtomHashEntry(const char16_t* aString, uint32_t aLength, uint32_t* aHashOut)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  EnsureTableExists();
  AtomTableKey key(aString, aLength, aHashOut);
  AtomTableEntry* e = static_cast<AtomTableEntry*>(
    PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_ADD));
  if (!e) {
    NS_ABORT_OOM(gAtomTable.entryCount * gAtomTable.entrySize);
  }
  return e;
}

class CheckStaticAtomSizes
{
  CheckStaticAtomSizes()
  {
    static_assert((sizeof(nsFakeStringBuffer<1>().mRefCnt) ==
                   sizeof(nsStringBuffer().mRefCount)) &&
                  (sizeof(nsFakeStringBuffer<1>().mSize) ==
                   sizeof(nsStringBuffer().mStorageSize)) &&
                  (offsetof(nsFakeStringBuffer<1>, mRefCnt) ==
                   offsetof(nsStringBuffer, mRefCount)) &&
                  (offsetof(nsFakeStringBuffer<1>, mSize) ==
                   offsetof(nsStringBuffer, mStorageSize)) &&
                  (offsetof(nsFakeStringBuffer<1>, mStringData) ==
                   sizeof(nsStringBuffer)),
                  "mocked-up strings' representations should be compatible");
  }
};

nsresult
RegisterStaticAtoms(const nsStaticAtom* aAtoms, uint32_t aAtomCount)
{
  if (!gStaticAtomTable && !gStaticAtomTableSealed) {
    gStaticAtomTable = new StaticAtomTable();
  }

  for (uint32_t i = 0; i < aAtomCount; ++i) {
    NS_ASSERTION(nsCRT::IsAscii((char16_t*)aAtoms[i].mStringBuffer->Data()),
                 "Static atoms must be ASCII!");

    uint32_t stringLen =
      aAtoms[i].mStringBuffer->StorageSize() / sizeof(char16_t) - 1;

    uint32_t hash;
    AtomTableEntry* he =
      GetAtomHashEntry((char16_t*)aAtoms[i].mStringBuffer->Data(),
                       stringLen, &hash);

    AtomImpl* atom = he->mAtom;
    if (atom) {
      if (!atom->IsPermanent()) {
        
        
        PromoteToPermanent(atom);
      }
    } else {
      atom = new PermanentAtomImpl(aAtoms[i].mStringBuffer, stringLen, hash);
      he->mAtom = atom;
    }
    *aAtoms[i].mAtom = atom;

    if (!gStaticAtomTableSealed) {
      StaticAtomEntry* entry =
        gStaticAtomTable->PutEntry(nsDependentAtomString(atom));
      entry->mAtom = atom;
    }
  }
  return NS_OK;
}

already_AddRefed<nsIAtom>
NS_NewAtom(const char* aUTF8String)
{
  return NS_NewAtom(nsDependentCString(aUTF8String));
}

already_AddRefed<nsIAtom>
NS_NewAtom(const nsACString& aUTF8String)
{
  uint32_t hash;
  AtomTableEntry* he = GetAtomHashEntry(aUTF8String.Data(),
                                        aUTF8String.Length(),
                                        &hash);

  if (he->mAtom) {
    nsCOMPtr<nsIAtom> atom = he->mAtom;

    return atom.forget();
  }

  
  
  
  nsString str;
  CopyUTF8toUTF16(aUTF8String, str);
  nsRefPtr<AtomImpl> atom = new AtomImpl(str, hash);

  he->mAtom = atom;

  return atom.forget();
}

already_AddRefed<nsIAtom>
NS_NewAtom(const char16_t* aUTF16String)
{
  return NS_NewAtom(nsDependentString(aUTF16String));
}

already_AddRefed<nsIAtom>
NS_NewAtom(const nsAString& aUTF16String)
{
  uint32_t hash;
  AtomTableEntry* he = GetAtomHashEntry(aUTF16String.Data(),
                                        aUTF16String.Length(),
                                        &hash);

  if (he->mAtom) {
    nsCOMPtr<nsIAtom> atom = he->mAtom;

    return atom.forget();
  }

  nsRefPtr<AtomImpl> atom = new AtomImpl(aUTF16String, hash);
  he->mAtom = atom;

  return atom.forget();
}

nsIAtom*
NS_NewPermanentAtom(const nsAString& aUTF16String)
{
  uint32_t hash;
  AtomTableEntry* he = GetAtomHashEntry(aUTF16String.Data(),
                                        aUTF16String.Length(),
                                        &hash);

  AtomImpl* atom = he->mAtom;
  if (atom) {
    if (!atom->IsPermanent()) {
      PromoteToPermanent(atom);
    }
  } else {
    atom = new PermanentAtomImpl(aUTF16String, hash);
    he->mAtom = atom;
  }

  
  return atom;
}

nsrefcnt
NS_GetNumberOfAtoms(void)
{
  return gAtomTable.entryCount;
}

nsIAtom*
NS_GetStaticAtom(const nsAString& aUTF16String)
{
  NS_PRECONDITION(gStaticAtomTable, "Static atom table not created yet.");
  NS_PRECONDITION(gStaticAtomTableSealed, "Static atom table not sealed yet.");
  StaticAtomEntry* entry = gStaticAtomTable->GetEntry(aUTF16String);
  return entry ? entry->mAtom : nullptr;
}

void
NS_SealStaticAtomTable()
{
  gStaticAtomTableSealed = true;
}
