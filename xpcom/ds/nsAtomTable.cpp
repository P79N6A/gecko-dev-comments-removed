





































#include "nsAtomTable.h"
#include "nsStaticAtom.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUTF8Utils.h"
#include "nsCRT.h"
#include "pldhash.h"
#include "prenv.h"
#include "nsThreadUtils.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "plarena.h"

class nsStaticAtomWrapper;










static PLDHashTable gAtomTable;



static PLArenaPool* gStaticAtomArena = 0;





static nsDataHashtable<nsStringHashKey, nsIAtom*>* gStaticAtomTable = 0;




static PRBool gStaticAtomTableSealed = PR_FALSE;

class nsStaticAtomWrapper : public nsIAtom
{
public:
  nsStaticAtomWrapper(const nsStaticAtom* aAtom, PRUint32 aLength) :
    mStaticAtom(aAtom), mLength(aLength)
  {
    MOZ_COUNT_CTOR(nsStaticAtomWrapper);
  }
  ~nsStaticAtomWrapper() {   
    
    
    
    MOZ_COUNT_DTOR(nsStaticAtomWrapper);
  }

  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  NS_DECL_NSIATOM

  const nsStaticAtom* GetStaticAtom() const {
    return mStaticAtom;
  }

  PRUint32 getLength() const {
    return mLength;
  }

private:
  const nsStaticAtom* mStaticAtom;

  
  
  
  PRUint32 mLength;
};
























typedef PRUword PtrBits;

struct AtomTableEntry : public PLDHashEntryHdr {
  
  
  PtrBits mBits;
  PRUint32 mLength;

  inline AtomTableEntry(const char *aString, PRUint32 aLength)
    : mBits(PtrBits(aString)), mLength(aLength)
  {
    keyHash = 0;
  }

  inline AtomTableEntry(const PRUnichar *aString, PRUint32 aLength)
    : mBits(PtrBits(aString)), mLength(aLength)
  {
    keyHash = 1;
  }

  inline PRBool IsStaticAtom() const {
    NS_ASSERTION(keyHash > 1,
                 "IsStaticAtom() called on non-atom AtomTableEntry!");
    return (mBits & 0x1) != 0;
  }

  inline PRBool IsUTF8String() const {
    return keyHash == 0;
  }

  inline PRBool IsUTF16String() const {
    return keyHash == 1;
  }

  inline void SetAtomImpl(AtomImpl* aAtom) {
    NS_ASSERTION(keyHash > 1,
                 "SetAtomImpl() called on non-atom AtomTableEntry!");
    NS_ASSERTION(aAtom, "Setting null atom");
    mBits = PtrBits(aAtom);
    mLength = aAtom->mLength;
  }

  inline void SetStaticAtomWrapper(nsStaticAtomWrapper* aAtom) {
    NS_ASSERTION(keyHash > 1,
                 "SetStaticAtomWrapper() called on non-atom AtomTableEntry!");
    NS_ASSERTION(aAtom, "Setting null atom");
    NS_ASSERTION((PtrBits(aAtom) & ~0x1) == PtrBits(aAtom),
                 "Pointers must align or this is broken");

    mBits = PtrBits(aAtom) | 0x1;
    mLength = aAtom->getLength();
  }
  
  inline void ClearAtom() {
    mBits = nsnull;
  }

  inline PRBool HasValue() const {
    NS_ASSERTION(keyHash > 1,
                 "HasValue() called on non-atom AtomTableEntry!");
    return (mBits & ~0x1) != 0;
  }

  
  inline AtomImpl *GetAtomImpl() const {
    NS_ASSERTION(keyHash > 1,
                 "GetAtomImpl() called on non-atom AtomTableEntry!");
    NS_ASSERTION(!IsStaticAtom(), "This is a static atom, not an AtomImpl");
    return (AtomImpl*) (mBits & ~0x1);
  }

  inline nsStaticAtomWrapper *GetStaticAtomWrapper() const {
    NS_ASSERTION(keyHash > 1,
                 "GetStaticAtomWrapper() called on non-atom AtomTableEntry!");
    NS_ASSERTION(IsStaticAtom(), "This is an AtomImpl, not a static atom");
    return (nsStaticAtomWrapper*) (mBits & ~0x1);
  }

  inline const nsStaticAtom* GetStaticAtom() const {
    NS_ASSERTION(keyHash > 1,
                 "GetStaticAtom() called on non-atom AtomTableEntry!");
    return GetStaticAtomWrapper()->GetStaticAtom();
  }

  

  
  inline const char* getAtomString() const {
    NS_ASSERTION(keyHash > 1,
                 "getAtomString() called on non-atom AtomTableEntry!");

    return IsStaticAtom() ? GetStaticAtom()->mString : GetAtomImpl()->mString;
  }

  
  inline const char* getUTF8String() const {
    NS_ASSERTION(keyHash == 0,
                 "getUTF8String() called on non-UTF8 AtomTableEntry!");

    return (char *)mBits;
  }

  
  inline const PRUnichar* getUTF16String() const {
    NS_ASSERTION(keyHash == 1,
                 "getUTF16String() called on non-UTF16 AtomTableEntry!");

    return (PRUnichar *)mBits;
  }

  
  inline PRUint32 getLength() const {
    return mLength;
  }

  
  
  inline nsIAtom* GetAtom() const {
    NS_ASSERTION(keyHash > 1,
                 "GetAtom() called on non-atom AtomTableEntry!");

    nsIAtom* result;
    
    if (IsStaticAtom())
      result = GetStaticAtomWrapper();
    else {
      result = GetAtomImpl();
      NS_ADDREF(result);
    }
    
    return result;
  }
};

static PLDHashNumber
AtomTableGetHash(PLDHashTable *table, const void *key)
{
  const AtomTableEntry *e = static_cast<const AtomTableEntry*>(key);

  if (e->IsUTF16String()) {
    return nsCRT::HashCodeAsUTF8(e->getUTF16String(), e->getLength());;
  }

  NS_ASSERTION(e->IsUTF8String(),
               "AtomTableGetHash() called on non-string-key AtomTableEntry!");

  return nsCRT::HashCode(e->getUTF8String(), e->getLength());
}

static PRBool
AtomTableMatchKey(PLDHashTable *table, const PLDHashEntryHdr *entry,
                  const void *key)
{
  const AtomTableEntry *he = static_cast<const AtomTableEntry*>(entry);
  const AtomTableEntry *strKey = static_cast<const AtomTableEntry*>(key);

  const char *atomString = he->getAtomString();

  if (strKey->IsUTF16String()) {
    return
      CompareUTF8toUTF16(nsDependentCSubstring(atomString, atomString + he->getLength()),
                         nsDependentSubstring(strKey->getUTF16String(),
                                              strKey->getUTF16String() + strKey->getLength())) == 0;
  }

  PRUint32 length = he->getLength();
  if (length != strKey->getLength()) {
    return PR_FALSE;
  }

  const char *str;

  if (strKey->IsUTF8String()) {
    str = strKey->getUTF8String();
  } else {
    str = strKey->getAtomString();
  }

  return memcmp(atomString, str, length * sizeof(char)) == 0;
}

static void
AtomTableClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  AtomTableEntry *he = static_cast<AtomTableEntry*>(entry);
  
  if (!he->IsStaticAtom()) {
    AtomImpl *atom = he->GetAtomImpl();
    
    
    
    
    
    
    if (atom->IsPermanent()) {
      he->keyHash = 0;

      delete static_cast<PermanentAtomImpl*>(atom);
    }
  }
  else {
    he->GetStaticAtomWrapper()->~nsStaticAtomWrapper();
  }
  
  he->ClearAtom();
}

static PRBool
AtomTableInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                   const void *key)
{
  AtomTableEntry *he = static_cast<AtomTableEntry*>(entry);
  const AtomTableEntry *strKey = static_cast<const AtomTableEntry*>(key);

  he->mLength = strKey->getLength();

  return PR_TRUE;
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
DumpAtomLeaks(PLDHashTable *table, PLDHashEntryHdr *he,
              PRUint32 index, void *arg)
{
  AtomTableEntry *entry = static_cast<AtomTableEntry*>(he);
  
  if (entry->IsStaticAtom())
    return PL_DHASH_NEXT;
  
  AtomImpl* atom = entry->GetAtomImpl();
  if (!atom->IsPermanent()) {
    ++*static_cast<PRUint32*>(arg);
    const char *str;
    atom->GetUTF8String(&str);
    fputs(str, stdout);
    fputs("\n", stdout);
  }
  return PL_DHASH_NEXT;
}

#endif

static inline
void PromoteToPermanent(AtomImpl* aAtom)
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
    const char *dumpAtomLeaks = PR_GetEnv("MOZ_DUMP_ATOM_LEAKS");
    if (dumpAtomLeaks && *dumpAtomLeaks) {
      PRUint32 leaked = 0;
      printf("*** %d atoms still exist (including permanent):\n",
             gAtomTable.entryCount);
      PL_DHashTableEnumerate(&gAtomTable, DumpAtomLeaks, &leaked);
      printf("*** %u non-permanent atoms leaked\n", leaked);
    }
#endif
    PL_DHashTableFinish(&gAtomTable);
    gAtomTable.entryCount = 0;
    gAtomTable.ops = nsnull;

    if (gStaticAtomArena) {
      PL_FinishArenaPool(gStaticAtomArena);
      delete gStaticAtomArena;
      gStaticAtomArena = nsnull;
    }
  }
}

AtomImpl::AtomImpl()
{
}

AtomImpl::~AtomImpl()
{
  NS_PRECONDITION(gAtomTable.ops, "uninitialized atom hashtable");
  
  
  
  if (!IsPermanentInDestructor()) {
    AtomTableEntry key(mString, mLength);
    PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_REMOVE);
    if (gAtomTable.entryCount == 0) {
      PL_DHashTableFinish(&gAtomTable);
      NS_ASSERTION(gAtomTable.entryCount == 0,
                   "PL_DHashTableFinish changed the entry count");
    }
  }
}

NS_IMPL_ISUPPORTS1(AtomImpl, nsIAtom)

PermanentAtomImpl::PermanentAtomImpl()
  : AtomImpl()
{
}

PermanentAtomImpl::~PermanentAtomImpl()
{
  
  mRefCnt = REFCNT_PERMANENT_SENTINEL;
}

NS_IMETHODIMP_(nsrefcnt) PermanentAtomImpl::AddRef()
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  return 2;
}

NS_IMETHODIMP_(nsrefcnt) PermanentAtomImpl::Release()
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  return 1;
}

 PRBool
AtomImpl::IsPermanent()
{
  return PR_FALSE;
}

 PRBool
PermanentAtomImpl::IsPermanent()
{
  return PR_TRUE;
}

void* AtomImpl::operator new ( size_t size, const nsACString& aString ) CPP_THROW_NEW
{
    








  size += aString.Length() * sizeof(char);
  AtomImpl* ii = static_cast<AtomImpl*>(::operator new(size));
  NS_ENSURE_TRUE(ii, nsnull);

  char* toBegin = &ii->mString[0];
  nsACString::const_iterator fromBegin, fromEnd;
  *copy_string(aString.BeginReading(fromBegin), aString.EndReading(fromEnd), toBegin) = '\0';
  ii->mLength = aString.Length();
  return ii;
}

void* PermanentAtomImpl::operator new ( size_t size, AtomImpl* aAtom ) CPP_THROW_NEW {
  NS_ASSERTION(!aAtom->IsPermanent(),
               "converting atom that's already permanent");

  
  return aAtom;
}

NS_IMETHODIMP 
AtomImpl::ToString(nsAString& aBuf)
{
  CopyUTF8toUTF16(nsDependentCString(mString, mLength), aBuf);
  return NS_OK;
}

NS_IMETHODIMP
AtomImpl::ToUTF8String(nsACString& aBuf)
{
  aBuf.Assign(mString, mLength);
  return NS_OK;
}

NS_IMETHODIMP 
AtomImpl::GetUTF8String(const char **aResult)
{
  NS_PRECONDITION(aResult, "null out param");
  *aResult = mString;
  return NS_OK;
}

NS_IMETHODIMP
AtomImpl::EqualsUTF8(const nsACString& aString, PRBool* aResult)
{
  *aResult = aString.Equals(nsDependentCString(mString, mLength));
  return NS_OK;
}

NS_IMETHODIMP
AtomImpl::Equals(const nsAString& aString, PRBool* aResult)
{
  *aResult = CompareUTF8toUTF16(nsDependentCString(mString, mLength),
                                aString) == 0;
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
AtomImpl::IsStaticAtom()
{
  return PR_FALSE;
}





NS_IMETHODIMP_(nsrefcnt)
nsStaticAtomWrapper::AddRef()
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsStaticAtomWrapper::Release()
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  return 1;
}

NS_IMPL_QUERY_INTERFACE1(nsStaticAtomWrapper, nsIAtom)

NS_IMETHODIMP
nsStaticAtomWrapper::GetUTF8String(const char** aResult)
{
  *aResult = mStaticAtom->mString;
  return NS_OK;
}

NS_IMETHODIMP
nsStaticAtomWrapper::ToString(nsAString& aBuf)
{
  
  
  NS_ASSERTION(nsCRT::IsAscii(mStaticAtom->mString),
               "Data loss - atom should be ASCII");
  CopyASCIItoUTF16(nsDependentCString(mStaticAtom->mString, mLength), aBuf);
  return NS_OK;
}

NS_IMETHODIMP
nsStaticAtomWrapper::ToUTF8String(nsACString& aBuf)
{
  aBuf.Assign(mStaticAtom->mString);
  return NS_OK;
}

NS_IMETHODIMP
nsStaticAtomWrapper::EqualsUTF8(const nsACString& aString, PRBool* aResult)
{
  *aResult = aString.Equals(nsDependentCString(mStaticAtom->mString, mLength));
  return NS_OK;
}

NS_IMETHODIMP
nsStaticAtomWrapper::Equals(const nsAString& aString, PRBool* aResult)
{
  *aResult = CompareUTF8toUTF16(nsDependentCString(mStaticAtom->mString,
                                                   mLength),
                                aString) == 0;
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsStaticAtomWrapper::IsStaticAtom()
{
  return PR_TRUE;
}



static nsStaticAtomWrapper*
WrapStaticAtom(const nsStaticAtom* aAtom, PRUint32 aLength)
{
  if (!gStaticAtomArena) {
    gStaticAtomArena = new PLArenaPool;
    if (!gStaticAtomArena)
      return nsnull;
    
    PL_INIT_ARENA_POOL(gStaticAtomArena, "nsStaticAtomArena", 4096);
  }

  void* mem;
  PL_ARENA_ALLOCATE(mem, gStaticAtomArena, sizeof(nsStaticAtomWrapper));

  nsStaticAtomWrapper* wrapper =
    new (mem) nsStaticAtomWrapper(aAtom, aLength);

  return wrapper;
}

#define ATOM_HASHTABLE_INITIAL_SIZE  4096

static inline AtomTableEntry*
GetAtomHashEntry(const char* aString, PRUint32 aLength)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  if (!gAtomTable.ops &&
      !PL_DHashTableInit(&gAtomTable, &AtomTableOps, 0,
                         sizeof(AtomTableEntry), ATOM_HASHTABLE_INITIAL_SIZE)) {
    gAtomTable.ops = nsnull;
    return nsnull;
  }

  AtomTableEntry key(aString, aLength);
  return static_cast<AtomTableEntry*>
                    (PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_ADD));
}

static inline AtomTableEntry*
GetAtomHashEntry(const PRUnichar* aString, PRUint32 aLength)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  if (!gAtomTable.ops &&
      !PL_DHashTableInit(&gAtomTable, &AtomTableOps, 0,
                         sizeof(AtomTableEntry), ATOM_HASHTABLE_INITIAL_SIZE)) {
    gAtomTable.ops = nsnull;
    return nsnull;
  }

  AtomTableEntry key(aString, aLength);
  return static_cast<AtomTableEntry*>
                    (PL_DHashTableOperate(&gAtomTable, &key, PL_DHASH_ADD));
}

NS_COM nsresult
NS_RegisterStaticAtoms(const nsStaticAtom* aAtoms, PRUint32 aAtomCount)
{
  
  
  
  
  
  if (!gStaticAtomTable && !gStaticAtomTableSealed) {
    gStaticAtomTable = new nsDataHashtable<nsStringHashKey, nsIAtom*>();
    if (!gStaticAtomTable || !gStaticAtomTable->Init()) {
      delete gStaticAtomTable;
      gStaticAtomTable = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  
  for (PRUint32 i=0; i<aAtomCount; i++) {
    NS_ASSERTION(nsCRT::IsAscii(aAtoms[i].mString),
                 "Static atoms must be ASCII!");

    PRUint32 stringLen = strlen(aAtoms[i].mString);

    AtomTableEntry *he =
      GetAtomHashEntry(aAtoms[i].mString, stringLen);

    if (he->HasValue() && aAtoms[i].mAtom) {
      
      
      if (!he->IsStaticAtom() && !he->GetAtomImpl()->IsPermanent()) {
        
        
        
        PromoteToPermanent(he->GetAtomImpl());
      }
      
      
      
      if (aAtoms[i].mAtom)
        *aAtoms[i].mAtom = he->GetAtom();
    }
    else {
      nsStaticAtomWrapper* atom = WrapStaticAtom(&aAtoms[i], stringLen);
      NS_ASSERTION(atom, "Failed to wrap static atom");

      
      he->SetStaticAtomWrapper(atom);
      if (aAtoms[i].mAtom)
        *aAtoms[i].mAtom = atom;
        
      if (!gStaticAtomTableSealed) {
        nsAutoString key;
        atom->ToString(key);
        gStaticAtomTable->Put(key, atom);
      }
    }
  }
  return NS_OK;
}

NS_COM nsIAtom*
NS_NewAtom(const char* aUTF8String)
{
  return NS_NewAtom(nsDependentCString(aUTF8String));
}

NS_COM nsIAtom*
NS_NewAtom(const nsACString& aUTF8String)
{
  AtomTableEntry *he = GetAtomHashEntry(aUTF8String.Data(),
                                        aUTF8String.Length());

  if (!he) {
    return nsnull;
  }

  NS_ASSERTION(!he->IsUTF8String() && !he->IsUTF16String(),
               "Atom hash entry is string?  Should be atom!");

  if (he->HasValue())
    return he->GetAtom();

  AtomImpl* atom = new (aUTF8String) AtomImpl();
  he->SetAtomImpl(atom);
  if (!atom) {
    PL_DHashTableRawRemove(&gAtomTable, he);
    return nsnull;
  }

  NS_ADDREF(atom);
  return atom;
}

NS_COM nsIAtom*
NS_NewAtom(const PRUnichar* aUTF16String)
{
  return NS_NewAtom(nsDependentString(aUTF16String));
}

NS_COM nsIAtom*
NS_NewAtom(const nsAString& aUTF16String)
{
  AtomTableEntry *he = GetAtomHashEntry(aUTF16String.Data(),
                                        aUTF16String.Length());

  if (he->HasValue())
    return he->GetAtom();

  
  
  NS_ConvertUTF16toUTF8 str(aUTF16String);
  AtomImpl* atom = new (str) AtomImpl();
  he->SetAtomImpl(atom);
  if (!atom) {
    PL_DHashTableRawRemove(&gAtomTable, he);
    return nsnull;
  }

  NS_ADDREF(atom);
  return atom;
}

NS_COM nsIAtom*
NS_NewPermanentAtom(const char* aUTF8String)
{
  return NS_NewPermanentAtom(nsDependentCString(aUTF8String));
}

NS_COM nsIAtom*
NS_NewPermanentAtom(const nsACString& aUTF8String)
{
  AtomTableEntry *he = GetAtomHashEntry(aUTF8String.Data(),
                                        aUTF8String.Length());

  if (he->HasValue() && he->IsStaticAtom())
    return he->GetStaticAtomWrapper();
  
  
  
  AtomImpl* atom = he->GetAtomImpl();
  
  if (atom) {
    
    if (!atom->IsPermanent()) {
      PromoteToPermanent(atom);
    }
  } else {
    
    atom = new (aUTF8String) PermanentAtomImpl();
    he->SetAtomImpl(atom);
    if ( !atom ) {
      PL_DHashTableRawRemove(&gAtomTable, he);
      return nsnull;
    }
  }

  NS_ADDREF(atom);
  return atom;
}

NS_COM nsIAtom*
NS_NewPermanentAtom(const nsAString& aUTF16String)
{
  return NS_NewPermanentAtom(NS_ConvertUTF16toUTF8(aUTF16String));
}

NS_COM nsIAtom*
NS_NewPermanentAtom(const PRUnichar* aUTF16String)
{
  return NS_NewPermanentAtom(NS_ConvertUTF16toUTF8(aUTF16String));
}

NS_COM nsrefcnt
NS_GetNumberOfAtoms(void)
{
  return gAtomTable.entryCount;
}

NS_COM nsIAtom*
NS_GetStaticAtom(const nsAString& aUTF16String)
{
  NS_PRECONDITION(gStaticAtomTable, "Static atom table not created yet.");
  NS_PRECONDITION(gStaticAtomTableSealed, "Static atom table not sealed yet.");
  nsIAtom* atom;
  if (!gStaticAtomTable->Get(aUTF16String, &atom)) {
    atom = nsnull;
  }
  return atom;
}

NS_COM void
NS_SealStaticAtomTable()
{
  gStaticAtomTableSealed = PR_TRUE;
}
