






#include "HttpLog.h"

#include "nsHttp.h"
#include "pldhash.h"
#include "mozilla/Mutex.h"
#include "mozilla/HashFunctions.h"
#include "nsCRT.h"

#if defined(PR_LOGGING)
PRLogModuleInfo *gHttpLog = nullptr;
#endif

namespace mozilla {
namespace net {


#define HTTP_ATOM(_name, _value) nsHttpAtom nsHttp::_name = { _value };
#include "nsHttpAtomList.h"
#undef HTTP_ATOM


#define HTTP_ATOM(_name, _value) Unused_ ## _name,
enum {
#include "nsHttpAtomList.h"
    NUM_HTTP_ATOMS
};
#undef HTTP_ATOM





struct HttpHeapAtom {
    struct HttpHeapAtom *next;
    char                 value[1];
};

static struct PLDHashTable  sAtomTable = {0};
static struct HttpHeapAtom *sHeapAtoms = nullptr;
static Mutex               *sLock = nullptr;

HttpHeapAtom *
NewHeapAtom(const char *value) {
    int len = strlen(value);

    HttpHeapAtom *a =
        reinterpret_cast<HttpHeapAtom *>(malloc(sizeof(*a) + len));
    if (!a)
        return nullptr;
    memcpy(a->value, value, len + 1);

    
    a->next = sHeapAtoms;
    sHeapAtoms = a;

    return a;
}


static PLDHashNumber
StringHash(PLDHashTable *table, const void *key)
{
    PLDHashNumber h = 0;
    for (const char *s = reinterpret_cast<const char*>(key); *s; ++s)
        h = AddToHash(h, nsCRT::ToLower(*s));
    return h;
}

static bool
StringCompare(PLDHashTable *table, const PLDHashEntryHdr *entry,
              const void *testKey)
{
    const void *entryKey =
            reinterpret_cast<const PLDHashEntryStub *>(entry)->key;

    return PL_strcasecmp(reinterpret_cast<const char *>(entryKey),
                         reinterpret_cast<const char *>(testKey)) == 0;
}

static const PLDHashTableOps ops = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    StringHash,
    StringCompare,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};


nsresult
nsHttp::CreateAtomTable()
{
    MOZ_ASSERT(!sAtomTable.ops, "atom table already initialized");

    if (!sLock) {
        sLock = new Mutex("nsHttp.sLock");
    }

    
    
    
    if (!PL_DHashTableInit(&sAtomTable, &ops, nullptr,
                           sizeof(PLDHashEntryStub),
                           fallible_t(), NUM_HTTP_ATOMS + 10)) {
        sAtomTable.ops = nullptr;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    const char *const atoms[] = {
#define HTTP_ATOM(_name, _value) nsHttp::_name._val,
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
        nullptr
    };

    for (int i = 0; atoms[i]; ++i) {
        PLDHashEntryStub *stub = reinterpret_cast<PLDHashEntryStub *>
                                                 (PL_DHashTableOperate(&sAtomTable, atoms[i], PL_DHASH_ADD));
        if (!stub)
            return NS_ERROR_OUT_OF_MEMORY;

        MOZ_ASSERT(!stub->key, "duplicate static atom");
        stub->key = atoms[i];
    }

    return NS_OK;
}

void
nsHttp::DestroyAtomTable()
{
    if (sAtomTable.ops) {
        PL_DHashTableFinish(&sAtomTable);
        sAtomTable.ops = nullptr;
    }

    while (sHeapAtoms) {
        HttpHeapAtom *next = sHeapAtoms->next;
        free(sHeapAtoms);
        sHeapAtoms = next;
    }

    if (sLock) {
        delete sLock;
        sLock = nullptr;
    }
}

Mutex *
nsHttp::GetLock()
{
    return sLock;
}


nsHttpAtom
nsHttp::ResolveAtom(const char *str)
{
    nsHttpAtom atom = { nullptr };

    if (!str || !sAtomTable.ops)
        return atom;

    MutexAutoLock lock(*sLock);

    PLDHashEntryStub *stub = reinterpret_cast<PLDHashEntryStub *>
                                             (PL_DHashTableOperate(&sAtomTable, str, PL_DHASH_ADD));
    if (!stub)
        return atom;  

    if (stub->key) {
        atom._val = reinterpret_cast<const char *>(stub->key);
        return atom;
    }

    
    
    HttpHeapAtom *heapAtom = NewHeapAtom(str);
    if (!heapAtom)
        return atom;  

    stub->key = atom._val = heapAtom->value;
    return atom;
}















static const char kValidTokenMap[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 

    0, 1, 0, 1, 1, 1, 1, 1, 
    0, 0, 1, 1, 0, 1, 1, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 0, 0, 0, 0, 0, 0, 

    0, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 0, 0, 0, 1, 1, 

    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 0, 1, 0, 1, 0  
};
bool
nsHttp::IsValidToken(const char *start, const char *end)
{
    if (start == end)
        return false;

    for (; start != end; ++start) {
        const unsigned char idx = *start;
        if (idx > 127 || !kValidTokenMap[idx])
            return false;
    }

    return true;
}


bool
nsHttp::IsReasonableHeaderValue(const nsACString &s)
{
  
  
  
  
  
  const nsACString::char_type* end = s.EndReading();
  for (const nsACString::char_type* i = s.BeginReading(); i != end; ++i) {
    if (*i == '\r' || *i == '\n' || *i == '\0') {
      return false;
    }
  }
  return true;
}

const char *
nsHttp::FindToken(const char *input, const char *token, const char *seps)
{
    if (!input)
        return nullptr;

    int inputLen = strlen(input);
    int tokenLen = strlen(token);

    if (inputLen < tokenLen)
        return nullptr;

    const char *inputTop = input;
    const char *inputEnd = input + inputLen - tokenLen;
    for (; input <= inputEnd; ++input) {
        if (PL_strncasecmp(input, token, tokenLen) == 0) {
            if (input > inputTop && !strchr(seps, *(input - 1)))
                continue;
            if (input < inputEnd && !strchr(seps, *(input + tokenLen)))
                continue;
            return input;
        }
    }

    return nullptr;
}

bool
nsHttp::ParseInt64(const char *input, const char **next, int64_t *r)
{
    const char *start = input;
    *r = 0;
    while (*input >= '0' && *input <= '9') {
        int64_t next = 10 * (*r) + (*input - '0');
        if (next < *r) 
            return false;
        *r = next;
        ++input;
    }
    if (input == start) 
        return false;
    if (next)
        *next = input;
    return true;
}

bool
nsHttp::IsPermanentRedirect(uint32_t httpStatus)
{
  return httpStatus == 301 || httpStatus == 308;
}


template<typename T> void
localEnsureBuffer(nsAutoArrayPtr<T> &buf, uint32_t newSize,
             uint32_t preserve, uint32_t &objSize)
{
  if (objSize >= newSize)
    return;

  
  
  

  objSize = (newSize + 2048 + 4095) & ~4095;

  static_assert(sizeof(T) == 1, "sizeof(T) must be 1");
  nsAutoArrayPtr<T> tmp(new T[objSize]);
  if (preserve) {
    memcpy(tmp, buf, preserve);
  }
  buf = tmp;
}

void EnsureBuffer(nsAutoArrayPtr<char> &buf, uint32_t newSize,
                  uint32_t preserve, uint32_t &objSize)
{
    localEnsureBuffer<char> (buf, newSize, preserve, objSize);
}

void EnsureBuffer(nsAutoArrayPtr<uint8_t> &buf, uint32_t newSize,
                  uint32_t preserve, uint32_t &objSize)
{
    localEnsureBuffer<uint8_t> (buf, newSize, preserve, objSize);
}

} 
} 
