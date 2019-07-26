







#include "mozilla/MemoryReporting.h"
#include "xpcprivate.h"

#include "js/HashTable.h"







static PLDHashNumber
HashIIDPtrKey(PLDHashTable *table, const void *key)
{
    return *((js::HashNumber*)key);
}

static bool
MatchIIDPtrKey(PLDHashTable *table,
               const PLDHashEntryHdr *entry,
               const void *key)
{
    return ((const nsID*)key)->
                Equals(*((const nsID*)((PLDHashEntryStub*)entry)->key));
}

static PLDHashNumber
HashNativeKey(PLDHashTable *table, const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    PLDHashNumber h = 0;

    XPCNativeSet*       Set;
    XPCNativeInterface* Addition;
    uint16_t            Position;

    if (Key->IsAKey()) {
        Set      = Key->GetBaseSet();
        Addition = Key->GetAddition();
        Position = Key->GetPosition();
    } else {
        Set      = (XPCNativeSet*) Key;
        Addition = nullptr;
        Position = 0;
    }

    if (!Set) {
        NS_ASSERTION(Addition, "bad key");
        
        
        h = (js::HashNumber) NS_PTR_TO_INT32(Addition) >> 2;
    } else {
        XPCNativeInterface** Current = Set->GetInterfaceArray();
        uint16_t count = Set->GetInterfaceCount();
        if (Addition) {
            count++;
            for (uint16_t i = 0; i < count; i++) {
                if (i == Position)
                    h ^= (js::HashNumber) NS_PTR_TO_INT32(Addition) >> 2;
                else
                    h ^= (js::HashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
            }
        } else {
            for (uint16_t i = 0; i < count; i++)
                h ^= (js::HashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
        }
    }

    return h;
}




void
JSObject2WrappedJSMap::FindDyingJSObjects(nsTArray<nsXPCWrappedJS*>* dying)
{
    for (Map::Range r = mTable.all(); !r.empty(); r.popFront()) {
        nsXPCWrappedJS* wrapper = r.front().value;
        NS_ASSERTION(wrapper, "found a null JS wrapper!");

        
        while (wrapper) {
            if (wrapper->IsSubjectToFinalization() && wrapper->IsObjectAboutToBeFinalized())
                dying->AppendElement(wrapper);
            wrapper = wrapper->GetNextWrapper();
        }
    }
}

void
JSObject2WrappedJSMap::ShutdownMarker(JSRuntime* rt)
{
    for (Map::Range r = mTable.all(); !r.empty(); r.popFront()) {
        nsXPCWrappedJS* wrapper = r.front().value;
        NS_ASSERTION(wrapper, "found a null JS wrapper!");
        NS_ASSERTION(wrapper->IsValid(), "found an invalid JS wrapper!");
        wrapper->SystemIsBeingShutDown(rt);
    }
}





Native2WrappedNativeMap*
Native2WrappedNativeMap::newMap(int size)
{
    Native2WrappedNativeMap* map = new Native2WrappedNativeMap(size);
    if (map && map->mTable)
        return map;
    
    
    
    
    NS_RUNTIMEABORT("Ran out of memory.");
    return nullptr;
}

Native2WrappedNativeMap::Native2WrappedNativeMap(int size)
{
    mTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                              sizeof(Entry), size);
}

Native2WrappedNativeMap::~Native2WrappedNativeMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

size_t
Native2WrappedNativeMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? PL_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
Native2WrappedNativeMap::SizeOfEntryExcludingThis(PLDHashEntryHdr *hdr,
                                                  mozilla::MallocSizeOf mallocSizeOf, void *)
{
    return mallocSizeOf(((Native2WrappedNativeMap::Entry*)hdr)->value);
}




struct PLDHashTableOps IID2WrappedJSClassMap::Entry::sOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIIDPtrKey,
    MatchIIDPtrKey,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
};


IID2WrappedJSClassMap*
IID2WrappedJSClassMap::newMap(int size)
{
    IID2WrappedJSClassMap* map = new IID2WrappedJSClassMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nullptr;
}

IID2WrappedJSClassMap::IID2WrappedJSClassMap(int size)
{
    mTable = PL_NewDHashTable(&Entry::sOps, nullptr, sizeof(Entry), size);
}

IID2WrappedJSClassMap::~IID2WrappedJSClassMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}





struct PLDHashTableOps IID2NativeInterfaceMap::Entry::sOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIIDPtrKey,
    MatchIIDPtrKey,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
};


IID2NativeInterfaceMap*
IID2NativeInterfaceMap::newMap(int size)
{
    IID2NativeInterfaceMap* map = new IID2NativeInterfaceMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nullptr;
}

IID2NativeInterfaceMap::IID2NativeInterfaceMap(int size)
{
    mTable = PL_NewDHashTable(&Entry::sOps, nullptr, sizeof(Entry), size);
}

IID2NativeInterfaceMap::~IID2NativeInterfaceMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

size_t
IID2NativeInterfaceMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? PL_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
IID2NativeInterfaceMap::SizeOfEntryExcludingThis(PLDHashEntryHdr *hdr,
                                                 mozilla::MallocSizeOf mallocSizeOf, void *)
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
    return nullptr;
}

ClassInfo2NativeSetMap::ClassInfo2NativeSetMap(int size)
{
    mTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                              sizeof(Entry), size);
}

ClassInfo2NativeSetMap::~ClassInfo2NativeSetMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

size_t
ClassInfo2NativeSetMap::ShallowSizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    
    n += mTable ? PL_DHashTableSizeOfIncludingThis(mTable, nullptr, mallocSizeOf) : 0;
    return n;
}





ClassInfo2WrappedNativeProtoMap*
ClassInfo2WrappedNativeProtoMap::newMap(int size)
{
    ClassInfo2WrappedNativeProtoMap* map = new ClassInfo2WrappedNativeProtoMap(size);
    if (map && map->mTable)
        return map;
    
    
    
    
    NS_RUNTIMEABORT("Ran out of memory.");
    return nullptr;
}

ClassInfo2WrappedNativeProtoMap::ClassInfo2WrappedNativeProtoMap(int size)
{
    mTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                              sizeof(Entry), size);
}

ClassInfo2WrappedNativeProtoMap::~ClassInfo2WrappedNativeProtoMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

size_t
ClassInfo2WrappedNativeProtoMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? PL_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
ClassInfo2WrappedNativeProtoMap::SizeOfEntryExcludingThis(PLDHashEntryHdr *hdr,
                                                          mozilla::MallocSizeOf mallocSizeOf, void *)
{
    return mallocSizeOf(((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value);
}




bool
NativeSetMap::Entry::Match(PLDHashTable *table,
                           const PLDHashEntryHdr *entry,
                           const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    
    if (!Key->IsAKey()) {
        XPCNativeSet* Set1 = (XPCNativeSet*) key;
        XPCNativeSet* Set2 = ((Entry*)entry)->key_value;

        if (Set1 == Set2)
            return true;

        uint16_t count = Set1->GetInterfaceCount();
        if (count != Set2->GetInterfaceCount())
            return false;

        XPCNativeInterface** Current1 = Set1->GetInterfaceArray();
        XPCNativeInterface** Current2 = Set2->GetInterfaceArray();
        for (uint16_t i = 0; i < count; i++) {
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

    uint16_t count = Set->GetInterfaceCount() + (Addition ? 1 : 0);
    if (count != SetInTable->GetInterfaceCount())
        return false;

    uint16_t Position = Key->GetPosition();
    XPCNativeInterface** CurrentInTable = SetInTable->GetInterfaceArray();
    XPCNativeInterface** Current = Set->GetInterfaceArray();
    for (uint16_t i = 0; i < count; i++) {
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

struct PLDHashTableOps NativeSetMap::Entry::sOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashNativeKey,
    Match,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
};


NativeSetMap*
NativeSetMap::newMap(int size)
{
    NativeSetMap* map = new NativeSetMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nullptr;
}

NativeSetMap::NativeSetMap(int size)
{
    mTable = PL_NewDHashTable(&Entry::sOps, nullptr, sizeof(Entry), size);
}

NativeSetMap::~NativeSetMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

size_t
NativeSetMap::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mTable ? PL_DHashTableSizeOfIncludingThis(mTable, SizeOfEntryExcludingThis, mallocSizeOf) : 0;
    return n;
}

 size_t
NativeSetMap::SizeOfEntryExcludingThis(PLDHashEntryHdr *hdr, mozilla::MallocSizeOf mallocSizeOf, void *)
{
    XPCNativeSet *set = ((NativeSetMap::Entry*)hdr)->key_value;
    return set->SizeOfIncludingThis(mallocSizeOf);
}




bool
IID2ThisTranslatorMap::Entry::Match(PLDHashTable *table,
                                    const PLDHashEntryHdr *entry,
                                    const void *key)
{
    return ((const nsID*)key)->Equals(((Entry*)entry)->key);
}

void
IID2ThisTranslatorMap::Entry::Clear(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    NS_IF_RELEASE(((Entry*)entry)->value);
    memset(entry, 0, table->entrySize);
}

struct PLDHashTableOps IID2ThisTranslatorMap::Entry::sOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashIIDPtrKey,
    Match,
    PL_DHashMoveEntryStub,
    Clear,
    PL_DHashFinalizeStub
};


IID2ThisTranslatorMap*
IID2ThisTranslatorMap::newMap(int size)
{
    IID2ThisTranslatorMap* map = new IID2ThisTranslatorMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nullptr;
}

IID2ThisTranslatorMap::IID2ThisTranslatorMap(int size)
{
    mTable = PL_NewDHashTable(&Entry::sOps, nullptr, sizeof(Entry), size);
}

IID2ThisTranslatorMap::~IID2ThisTranslatorMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}



PLDHashNumber
XPCNativeScriptableSharedMap::Entry::Hash(PLDHashTable *table, const void *key)
{
    PLDHashNumber h;
    const unsigned char *s;

    XPCNativeScriptableShared* obj =
        (XPCNativeScriptableShared*) key;

    
    
    

    h = (PLDHashNumber) obj->GetFlags();
    for (s = (const unsigned char*) obj->GetJSClass()->name; *s != '\0'; s++)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;
    return h;
}

bool
XPCNativeScriptableSharedMap::Entry::Match(PLDHashTable *table,
                                           const PLDHashEntryHdr *entry,
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

struct PLDHashTableOps XPCNativeScriptableSharedMap::Entry::sOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    Hash,
    Match,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
};


XPCNativeScriptableSharedMap*
XPCNativeScriptableSharedMap::newMap(int size)
{
    XPCNativeScriptableSharedMap* map =
        new XPCNativeScriptableSharedMap(size);
    if (map && map->mTable)
        return map;
    delete map;
    return nullptr;
}

XPCNativeScriptableSharedMap::XPCNativeScriptableSharedMap(int size)
{
    mTable = PL_NewDHashTable(&Entry::sOps, nullptr, sizeof(Entry), size);
}

XPCNativeScriptableSharedMap::~XPCNativeScriptableSharedMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}

JSBool
XPCNativeScriptableSharedMap::GetNewOrUsed(uint32_t flags,
                                           char* name,
                                           uint32_t interfacesBitmap,
                                           XPCNativeScriptableInfo* si)
{
    NS_PRECONDITION(name,"bad param");
    NS_PRECONDITION(si,"bad param");

    XPCNativeScriptableShared key(flags, name, interfacesBitmap);
    Entry* entry = (Entry*)
        PL_DHashTableOperate(mTable, &key, PL_DHASH_ADD);
    if (!entry)
        return false;

    XPCNativeScriptableShared* shared = entry->key;

    if (!shared) {
        entry->key = shared =
            new XPCNativeScriptableShared(flags, key.TransferNameOwnership(),
                                          interfacesBitmap);
        if (!shared)
            return false;
        shared->PopulateJSClass();
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
    return nullptr;
}

XPCWrappedNativeProtoMap::XPCWrappedNativeProtoMap(int size)
{
    mTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                              sizeof(PLDHashEntryStub), size);
}

XPCWrappedNativeProtoMap::~XPCWrappedNativeProtoMap()
{
    if (mTable)
        PL_DHashTableDestroy(mTable);
}


