






































#include "nsHttp.h"
#include "nsAutoLock.h"
#include "pldhash.h"
#include "nsCRT.h"

#if defined(PR_LOGGING)
PRLogModuleInfo *gHttpLog = nsnull;
#endif


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
static struct HttpHeapAtom *sHeapAtoms = nsnull;
static PRLock              *sLock = nsnull;

HttpHeapAtom *
NewHeapAtom(const char *value) {
    int len = strlen(value);

    HttpHeapAtom *a =
        NS_REINTERPRET_CAST(HttpHeapAtom *, malloc(sizeof(*a) + len));
    if (!a)
        return nsnull;
    memcpy(a->value, value, len + 1);

    
    a->next = sHeapAtoms;
    sHeapAtoms = a;

    return a;
}


PR_STATIC_CALLBACK(PLDHashNumber)
StringHash(PLDHashTable *table, const void *key)
{
    PLDHashNumber h = 0;
    for (const char *s = NS_REINTERPRET_CAST(const char*, key); *s; ++s)
        h = (h >> 28) ^ (h << 4) ^ nsCRT::ToLower(*s);
    return h;
}

PR_STATIC_CALLBACK(PRBool)
StringCompare(PLDHashTable *table, const PLDHashEntryHdr *entry,
              const void *testKey)
{
    const void *entryKey =
            NS_REINTERPRET_CAST(const PLDHashEntryStub *, entry)->key;

    return PL_strcasecmp(NS_REINTERPRET_CAST(const char *, entryKey),
                         NS_REINTERPRET_CAST(const char *, testKey)) == 0;
}

static const PLDHashTableOps ops = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    StringHash,
    StringCompare,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nsnull
};


nsresult
nsHttp::CreateAtomTable()
{
    NS_ASSERTION(!sAtomTable.ops, "atom table already initialized");

    if (!sLock) {
        sLock = PR_NewLock();
        if (!sLock)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    if (!PL_DHashTableInit(&sAtomTable, &ops, nsnull, sizeof(PLDHashEntryStub),
                           NUM_HTTP_ATOMS + 10)) {
        sAtomTable.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    const char *const atoms[] = {
#define HTTP_ATOM(_name, _value) nsHttp::_name._val,
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
        nsnull
    };

    for (int i = 0; atoms[i]; ++i) {
        PLDHashEntryStub *stub = NS_REINTERPRET_CAST(PLDHashEntryStub *,
                PL_DHashTableOperate(&sAtomTable, atoms[i], PL_DHASH_ADD));
        if (!stub)
            return NS_ERROR_OUT_OF_MEMORY;
        
        NS_ASSERTION(!stub->key, "duplicate static atom");
        stub->key = atoms[i];
    }

    return NS_OK;
}

void
nsHttp::DestroyAtomTable()
{
    if (sAtomTable.ops) {
        PL_DHashTableFinish(&sAtomTable);
        sAtomTable.ops = nsnull;
    }

    while (sHeapAtoms) {
        HttpHeapAtom *next = sHeapAtoms->next;
        free(sHeapAtoms);
        sHeapAtoms = next;
    }

    if (sLock) {
        PR_DestroyLock(sLock);
        sLock = nsnull;
    }
}


nsHttpAtom
nsHttp::ResolveAtom(const char *str)
{
    nsHttpAtom atom = { nsnull };

    if (!str || !sAtomTable.ops)
        return atom;

    nsAutoLock lock(sLock);

    PLDHashEntryStub *stub = NS_REINTERPRET_CAST(PLDHashEntryStub *,
            PL_DHashTableOperate(&sAtomTable, str, PL_DHASH_ADD));
    if (!stub)
        return atom;  

    if (stub->key) {
        atom._val = NS_REINTERPRET_CAST(const char *, stub->key);
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
PRBool
nsHttp::IsValidToken(const char *start, const char *end)
{
    if (start == end)
        return PR_FALSE;

    for (; start != end; ++start) {
        const unsigned char idx = *start;
        if (idx > 127 || !kValidTokenMap[idx])
            return PR_FALSE;
    }

    return PR_TRUE;
}

const char *
nsHttp::FindToken(const char *input, const char *token, const char *seps)
{
    if (!input)
        return nsnull;

    int inputLen = strlen(input);
    int tokenLen = strlen(token);

    if (inputLen < tokenLen)
        return nsnull;

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

    return nsnull;
}

PRBool
nsHttp::ParseInt64(const char *input, const char **next, PRInt64 *r)
{
    const char *start = input;
    *r = 0;
    while (*input >= '0' && *input <= '9') {
        PRInt64 next = 10 * (*r) + (*input - '0');
        if (next < *r) 
            return PR_FALSE;
        *r = next;
        ++input;
    }
    if (input == start) 
        return PR_FALSE;
    if (next)
        *next = input;
    return PR_TRUE;
}
