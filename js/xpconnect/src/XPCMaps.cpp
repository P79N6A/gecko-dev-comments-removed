









































#include "xpcprivate.h"

#include "jshash.h"







static JSDHashNumber
HashIIDPtrKey(JSDHashTable *table, const void *key)
{
    return *((JSHashNumber*)key);
}

static JSBool
MatchIIDPtrKey(JSDHashTable *table,
               const JSDHashEntryHdr *entry,
               const void *key)
{
    return ((const nsID*)key)->
                Equals(*((const nsID*)((JSDHashEntryStub*)entry)->key));
}

static JSDHashNumber
HashNativeKey(JSDHashTable *table, const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    JSDHashNumber h = 0;

    XPCNativeSet*       Set;
    XPCNativeInterface* Addition;
    PRUint16            Position;

    if (Key->IsAKey()) {
        Set      = Key->GetBaseSet();
        Addition = Key->GetAddition();
        Position = Key->GetPosition();
    } else {
        Set      = (XPCNativeSet*) Key;
        Addition = nsnull;
        Position = 0;
    }

    if (!Set) {
        NS_ASSERTION(Addition, "bad key");
        
        
        h = (JSHashNumber) NS_PTR_TO_INT32(Addition) >> 2;
    } else {
        XPCNativeInterface** Current = Set->GetInterfaceArray();
        PRUint16 count = Set->GetInterfaceCount();
        if (Addition) {
            count++;
            for (PRUint16 i = 0; i < count; i++) {
                if (i == Position)
                    h ^= (JSHashNumber) NS_PTR_TO_INT32(Addition) >> 2;
                else
                    h ^= (JSHashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
            }
        } else {
            for (PRUint16 i = 0; i < count; i++)
                h ^= (JSHashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
        }
    }

    return h;
}





JSObject2WrappedJSMap*
JSObject2WrappedJSMap::newMap(int size)
{
    JSObject2WrappedJSMap* map = new JSObject2WrappedJSMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

JSObject2WrappedJSMap::JSObject2WrappedJSMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(Entry), size);
}

JSObject2WrappedJSMap::~JSObject2WrappedJSMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
JSObject2WrappedJSMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
JSObject2WrappedJSMap::SizeOfEntryExcludingThis(JSDHashEntryHdr *hdr,
                                                JSMallocSizeOfFun mallocSizeOf, void *)
{
    return mallocSizeOf(((JSObject2WrappedJSMap::Entry*)hdr)->value);
}





Native2WrappedNativeMap*
Native2WrappedNativeMap::newMap(int size)
{
    Native2WrappedNativeMap* map = new Native2WrappedNativeMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

Native2WrappedNativeMap::Native2WrappedNativeMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(Entry), size);
}

Native2WrappedNativeMap::~Native2WrappedNativeMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
Native2WrappedNativeMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
Native2WrappedNativeMap::SizeOfEntryExcludingThis(JSDHashEntryHdr *hdr,
                                                  JSMallocSizeOfFun mallocSizeOf, void *)
{
    return mallocSizeOf(((Native2WrappedNativeMap::Entry*)hdr)->value);
}




struct JSDHashTableOps IID2WrappedJSClassMap::Entry::sOps =
{
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashIIDPtrKey,
    MatchIIDPtrKey,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub
};


IID2WrappedJSClassMap*
IID2WrappedJSClassMap::newMap(int size)
{
    IID2WrappedJSClassMap* map = new IID2WrappedJSClassMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2WrappedJSClassMap::IID2WrappedJSClassMap(int size)
{
    mTable = JS_NewDHashTable(&Entry::sOps, nsnull, sizeof(Entry), size);
}

IID2WrappedJSClassMap::~IID2WrappedJSClassMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}





struct JSDHashTableOps IID2NativeInterfaceMap::Entry::sOps =
{
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashIIDPtrKey,
    MatchIIDPtrKey,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub
};


IID2NativeInterfaceMap*
IID2NativeInterfaceMap::newMap(int size)
{
    IID2NativeInterfaceMap* map = new IID2NativeInterfaceMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2NativeInterfaceMap::IID2NativeInterfaceMap(int size)
{
    mTable = JS_NewDHashTable(&Entry::sOps, nsnull, sizeof(Entry), size);
}

IID2NativeInterfaceMap::~IID2NativeInterfaceMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
IID2NativeInterfaceMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
IID2NativeInterfaceMap::SizeOfEntryExcludingThis(JSDHashEntryHdr *hdr,
                                                 JSMallocSizeOfFun mallocSizeOf, void *)
{
    XPCNativeInterface *iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    return iface->SizeOfIncludingThis(mallocSizeOf);
}





ClassInfo2NativeSetMap*
ClassInfo2NativeSetMap::newMap(int size)
{
    ClassInfo2NativeSetMap* map = new ClassInfo2NativeSetMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

ClassInfo2NativeSetMap::ClassInfo2NativeSetMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(Entry), size);
}

ClassInfo2NativeSetMap::~ClassInfo2NativeSetMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
ClassInfo2NativeSetMap::ShallowSizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, NULL, mallocSizeOf) : 0;
    return n;
}





ClassInfo2WrappedNativeProtoMap*
ClassInfo2WrappedNativeProtoMap::newMap(int size)
{
    ClassInfo2WrappedNativeProtoMap* map = new ClassInfo2WrappedNativeProtoMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

ClassInfo2WrappedNativeProtoMap::ClassInfo2WrappedNativeProtoMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(Entry), size);
}

ClassInfo2WrappedNativeProtoMap::~ClassInfo2WrappedNativeProtoMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
ClassInfo2WrappedNativeProtoMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
ClassInfo2WrappedNativeProtoMap::SizeOfEntryExcludingThis(JSDHashEntryHdr *hdr,
                                                          JSMallocSizeOfFun mallocSizeOf, void *)
{
    return mallocSizeOf(((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value);
}




JSBool
NativeSetMap::Entry::Match(JSDHashTable *table,
                           const JSDHashEntryHdr *entry,
                           const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    
    if (!Key->IsAKey()) {
        XPCNativeSet* Set1 = (XPCNativeSet*) key;
        XPCNativeSet* Set2 = ((Entry*)entry)->key_value;

        if (Set1 == Set2)
            return true;

        PRUint16 count = Set1->GetInterfaceCount();
        if (count != Set2->GetInterfaceCount())
            return false;

        XPCNativeInterface** Current1 = Set1->GetInterfaceArray();
        XPCNativeInterface** Current2 = Set2->GetInterfaceArray();
        for (PRUint16 i = 0; i < count; i++) {
            if (*(Current1++) != *(Current2++))
                return false;
        }

        return true;
    }

    XPCNativeSet*       SetInTable = ((Entry*)entry)->key_value;
    XPCNativeSet*       Set        = Key->GetBaseSet();
    XPCNativeInterface* Addition   = Key->GetAddition();

    if (!Set) {
        
        
        
        
        
        
        
        

        return ((SetInTable->GetInterfaceCount() == 1 &&
                 SetInTable->GetInterfaceAt(0) == Addition) ||
                (SetInTable->GetInterfaceCount() == 2 &&
                 SetInTable->GetInterfaceAt(1) == Addition));
    }

    if (!Addition && Set == SetInTable)
        return true;

    PRUint16 count = Set->GetInterfaceCount() + (Addition ? 1 : 0);
    if (count != SetInTable->GetInterfaceCount())
        return false;

    PRUint16 Position = Key->GetPosition();
    XPCNativeInterface** CurrentInTable = SetInTable->GetInterfaceArray();
    XPCNativeInterface** Current = Set->GetInterfaceArray();
    for (PRUint16 i = 0; i < count; i++) {
        if (Addition && i == Position) {
            if (Addition != *(CurrentInTable++))
                return false;
        } else {
            if (*(Current++) != *(CurrentInTable++))
                return false;
        }
    }

    return true;
}

struct JSDHashTableOps NativeSetMap::Entry::sOps =
{
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashNativeKey,
    Match,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub
};


NativeSetMap*
NativeSetMap::newMap(int size)
{
    NativeSetMap* map = new NativeSetMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

NativeSetMap::NativeSetMap(int size)
{
    mTable = JS_NewDHashTable(&Entry::sOps, nsnull, sizeof(Entry), size);
}

NativeSetMap::~NativeSetMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

size_t
NativeSetMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? JS_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
NativeSetMap::SizeOfEntryExcludingThis(JSDHashEntryHdr *hdr, JSMallocSizeOfFun mallocSizeOf, void *)
{
    XPCNativeSet *set = ((NativeSetMap::Entry*)hdr)->key_value;
    return set->SizeOfIncludingThis(mallocSizeOf);
}




JSBool
IID2ThisTranslatorMap::Entry::Match(JSDHashTable *table,
                                    const JSDHashEntryHdr *entry,
                                    const void *key)
{
    return ((const nsID*)key)->Equals(((Entry*)entry)->key);
}

void
IID2ThisTranslatorMap::Entry::Clear(JSDHashTable *table, JSDHashEntryHdr *entry)
{
    NS_IF_RELEASE(((Entry*)entry)->value);
    memset(entry, 0, table->entrySize);
}

struct JSDHashTableOps IID2ThisTranslatorMap::Entry::sOps =
{
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashIIDPtrKey,
    Match,
    JS_DHashMoveEntryStub,
    Clear,
    JS_DHashFinalizeStub
};


IID2ThisTranslatorMap*
IID2ThisTranslatorMap::newMap(int size)
{
    IID2ThisTranslatorMap* map = new IID2ThisTranslatorMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2ThisTranslatorMap::IID2ThisTranslatorMap(int size)
{
    mTable = JS_NewDHashTable(&Entry::sOps, nsnull, sizeof(Entry), size);
}

IID2ThisTranslatorMap::~IID2ThisTranslatorMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}



JSDHashNumber
XPCNativeScriptableSharedMap::Entry::Hash(JSDHashTable *table, const void *key)
{
    JSDHashNumber h;
    const unsigned char *s;

    XPCNativeScriptableShared* obj =
        (XPCNativeScriptableShared*) key;

    
    
    

    h = (JSDHashNumber) obj->GetFlags();
    for (s = (const unsigned char*) obj->GetJSClass()->name; *s != '\0'; s++)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;
    return h;
}

JSBool
XPCNativeScriptableSharedMap::Entry::Match(JSDHashTable *table,
                                           const JSDHashEntryHdr *entry,
                                           const void *key)
{
    XPCNativeScriptableShared* obj1 =
        ((XPCNativeScriptableSharedMap::Entry*) entry)->key;

    XPCNativeScriptableShared* obj2 =
        (XPCNativeScriptableShared*) key;

    

    if (obj1->GetFlags() != obj2->GetFlags() ||
        obj1->GetInterfacesBitmap() != obj2->GetInterfacesBitmap())
        return false;

    const char* name1 = obj1->GetJSClass()->name;
    const char* name2 = obj2->GetJSClass()->name;

    if (!name1 || !name2)
        return name1 == name2;

    return 0 == strcmp(name1, name2);
}

struct JSDHashTableOps XPCNativeScriptableSharedMap::Entry::sOps =
{
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    Hash,
    Match,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub
};


XPCNativeScriptableSharedMap*
XPCNativeScriptableSharedMap::newMap(int size)
{
    XPCNativeScriptableSharedMap* map =
        new XPCNativeScriptableSharedMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

XPCNativeScriptableSharedMap::XPCNativeScriptableSharedMap(int size)
{
    mTable = JS_NewDHashTable(&Entry::sOps, nsnull, sizeof(Entry), size);
}

XPCNativeScriptableSharedMap::~XPCNativeScriptableSharedMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

JSBool
XPCNativeScriptableSharedMap::GetNewOrUsed(uint32_t flags,
                                           char* name,
                                           bool isGlobal,
                                           PRUint32 interfacesBitmap,
                                           XPCNativeScriptableInfo* si)
{
    NS_PRECONDITION(name,"bad param");
    NS_PRECONDITION(si,"bad param");

    XPCNativeScriptableShared key(flags, name, interfacesBitmap);
    Entry* entry = (Entry*)
        JS_DHashTableOperate(mTable, &key, JS_DHASH_ADD);
    if (!entry)
        return false;

    XPCNativeScriptableShared* shared = entry->key;

    if (!shared) {
        entry->key = shared =
            new XPCNativeScriptableShared(flags, key.TransferNameOwnership(),
                                          interfacesBitmap);
        if (!shared)
            return false;
        shared->PopulateJSClass(isGlobal);
    }
    si->SetScriptableShared(shared);
    return true;
}





XPCWrappedNativeProtoMap*
XPCWrappedNativeProtoMap::newMap(int size)
{
    XPCWrappedNativeProtoMap* map = new XPCWrappedNativeProtoMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

XPCWrappedNativeProtoMap::XPCWrappedNativeProtoMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(JSDHashEntryStub), size);
}

XPCWrappedNativeProtoMap::~XPCWrappedNativeProtoMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}





XPCNativeWrapperMap*
XPCNativeWrapperMap::newMap(int size)
{
    XPCNativeWrapperMap* map = new XPCNativeWrapperMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

XPCNativeWrapperMap::XPCNativeWrapperMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(JSDHashEntryStub), size);
}

XPCNativeWrapperMap::~XPCNativeWrapperMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}




struct JSDHashTableOps
WrappedNative2WrapperMap::sOps = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    JS_DHashVoidPtrKeyStub,
    JS_DHashMatchEntryStub,
    MoveLink,
    ClearLink,
    JS_DHashFinalizeStub,
    nsnull
};


void
WrappedNative2WrapperMap::ClearLink(JSDHashTable* table,
                                    JSDHashEntryHdr* entry)
{
    Entry* e = static_cast<Entry*>(entry);
    e->key = nsnull;
    PR_REMOVE_LINK(&e->value);
    memset(e, 0, sizeof(*e));
}


void
WrappedNative2WrapperMap::MoveLink(JSDHashTable* table,
                                   const JSDHashEntryHdr* from,
                                   JSDHashEntryHdr* to)
{
    const Entry* oldEntry = static_cast<const Entry*>(from);
    Entry* newEntry = static_cast<Entry*>(to);

    newEntry->key = oldEntry->key;

    
    if (PR_CLIST_IS_EMPTY(&oldEntry->value)) {
        PR_INIT_CLIST(&newEntry->value);
        newEntry->value.obj = oldEntry->value.obj;
    } else {
        newEntry->value = oldEntry->value;
        newEntry->value.next->prev = &newEntry->value;
        newEntry->value.prev->next = &newEntry->value;
    }
}


WrappedNative2WrapperMap*
WrappedNative2WrapperMap::newMap(int size)
{
    WrappedNative2WrapperMap* map = new WrappedNative2WrapperMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

WrappedNative2WrapperMap::WrappedNative2WrapperMap(int size)
{
    mTable = JS_NewDHashTable(&sOps, nsnull, sizeof(Entry), size);
}

WrappedNative2WrapperMap::~WrappedNative2WrapperMap()
{
    if (mTable)
        JS_DHashTableDestroy(mTable);
}

JSObject*
WrappedNative2WrapperMap::Add(WrappedNative2WrapperMap* head,
                              JSObject* wrappedObject,
                              JSObject* wrapper)
{
    NS_PRECONDITION(wrappedObject,"bad param");
    Entry* entry = (Entry*)
        JS_DHashTableOperate(mTable, wrappedObject, JS_DHASH_ADD);
    if (!entry)
        return nsnull;
    NS_ASSERTION(!entry->key || this == head, "dangling pointer?");
    entry->key = wrappedObject;
    Link* l = &entry->value;

    NS_ASSERTION(!l->obj, "Uh, how'd this happen?");

    if (!l->next) {
        
        
        PR_INIT_CLIST(l);
    }

    l->obj = wrapper;

    if (this != head) {
        Link* headLink = head->FindLink(wrappedObject);
        if (!headLink) {
            Entry* dummy = (Entry*)
                JS_DHashTableOperate(head->mTable, wrappedObject, JS_DHASH_ADD);
            dummy->key = wrappedObject;
            headLink = &dummy->value;
            PR_INIT_CLIST(headLink);
            headLink->obj = nsnull;
        }

        PR_INSERT_BEFORE(l, headLink);
    }

    return wrapper;
}

bool
WrappedNative2WrapperMap::AddLink(JSObject* wrappedObject, Link* oldLink)
{
    Entry* entry = (Entry*)
        JS_DHashTableOperate(mTable, wrappedObject, JS_DHASH_ADD);
    if (!entry)
        return false;
    NS_ASSERTION(!entry->key, "Eh? What's happening?");
    entry->key = wrappedObject;
    Link* newLink = &entry->value;

    PR_INSERT_LINK(newLink, oldLink);
    PR_REMOVE_AND_INIT_LINK(oldLink);
    newLink->obj = oldLink->obj;

    return true;
}




struct JSDHashTableOps
JSObject2JSObjectMap::sOps = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    JS_DHashVoidPtrKeyStub,
    JS_DHashMatchEntryStub,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    nsnull
};


