









































#include "xpcprivate.h"







static JSDHashNumber JS_DLL_CALLBACK
HashIIDPtrKey(JSDHashTable *table, const void *key)
{
    return *((JSHashNumber*)key);
}

static JSBool JS_DLL_CALLBACK
MatchIIDPtrKey(JSDHashTable *table,
            const JSDHashEntryHdr *entry,
            const void *key)
{
    return ((const nsID*)key)->
                Equals(*((const nsID*)((JSDHashEntryStub*)entry)->key));
}

static JSDHashNumber JS_DLL_CALLBACK
HashNativeKey(JSDHashTable *table, const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    JSDHashNumber h = 0;

    XPCNativeSet*       Set;
    XPCNativeInterface* Addition;
    PRUint16            Position;

    if(Key->IsAKey())
    {
        Set      = Key->GetBaseSet();
        Addition = Key->GetAddition();
        Position = Key->GetPosition();
    }
    else
    {
        Set      = (XPCNativeSet*) Key;
        Addition = nsnull;
        Position = 0;
    }

    if(!Set)
    {
        NS_ASSERTION(Addition, "bad key");
        
        
        h = (JSHashNumber) NS_PTR_TO_INT32(Addition) >> 2;
    }
    else
    {
        XPCNativeInterface** Current = Set->GetInterfaceArray();
        PRUint16 count = Set->GetInterfaceCount();
        if(Addition)
        {
            count++;
            for(PRUint16 i = 0; i < count; i++)
            {
                if(i == Position)
                    h ^= (JSHashNumber) NS_PTR_TO_INT32(Addition) >> 2;
                else
                    h ^= (JSHashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
            }
        }
        else
        {
            for(PRUint16 i = 0; i < count; i++)
                h ^= (JSHashNumber) NS_PTR_TO_INT32(*(Current++)) >> 2;
        }
    }

    return h;
}





JSContext2XPCContextMap*
JSContext2XPCContextMap::newMap(int size)
{
    JSContext2XPCContextMap* map = new JSContext2XPCContextMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}


JSContext2XPCContextMap::JSContext2XPCContextMap(int size)
{
    mTable = JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                              sizeof(Entry), size);
}

JSContext2XPCContextMap::~JSContext2XPCContextMap()
{
    if(mTable)
        JS_DHashTableDestroy(mTable);
}





JSObject2WrappedJSMap*
JSObject2WrappedJSMap::newMap(int size)
{
    JSObject2WrappedJSMap* map = new JSObject2WrappedJSMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}





Native2WrappedNativeMap*
Native2WrappedNativeMap::newMap(int size)
{
    Native2WrappedNativeMap* map = new Native2WrappedNativeMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
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
    if(map && map->mTable)
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
    if(mTable)
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
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}





ClassInfo2NativeSetMap*
ClassInfo2NativeSetMap::newMap(int size)
{
    ClassInfo2NativeSetMap* map = new ClassInfo2NativeSetMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}





ClassInfo2WrappedNativeProtoMap*
ClassInfo2WrappedNativeProtoMap::newMap(int size)
{
    ClassInfo2WrappedNativeProtoMap* map = new ClassInfo2WrappedNativeProtoMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}




JSBool JS_DLL_CALLBACK
NativeSetMap::Entry::Match(JSDHashTable *table,
                           const JSDHashEntryHdr *entry,
                           const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    
    if(!Key->IsAKey())
    {
        XPCNativeSet* Set1 = (XPCNativeSet*) key;
        XPCNativeSet* Set2 = ((Entry*)entry)->key_value;

        if(Set1 == Set2)
            return JS_TRUE;

        PRUint16 count = Set1->GetInterfaceCount();
        if(count != Set2->GetInterfaceCount())
            return JS_FALSE;

        XPCNativeInterface** Current1 = Set1->GetInterfaceArray();
        XPCNativeInterface** Current2 = Set2->GetInterfaceArray();
        for(PRUint16 i = 0; i < count; i++)
        {
            if(*(Current1++) != *(Current2++))
                return JS_FALSE;
        }

        return JS_TRUE;
    }

    XPCNativeSet*       SetInTable = ((Entry*)entry)->key_value;
    XPCNativeSet*       Set        = Key->GetBaseSet();
    XPCNativeInterface* Addition   = Key->GetAddition();

    if(!Set)
    {
        
        
        
        
        
        
        
        

        return ((SetInTable->GetInterfaceCount() == 1 &&
                 SetInTable->GetInterfaceAt(0) == Addition) ||
                (SetInTable->GetInterfaceCount() == 2 &&
                 SetInTable->GetInterfaceAt(1) == Addition));
    }

    if(!Addition && Set == SetInTable)
        return JS_TRUE;

    PRUint16 count = Set->GetInterfaceCount() + (Addition ? 1 : 0);
    if(count != SetInTable->GetInterfaceCount())
        return JS_FALSE;

    PRUint16 Position = Key->GetPosition();
    XPCNativeInterface** CurrentInTable = SetInTable->GetInterfaceArray();
    XPCNativeInterface** Current = Set->GetInterfaceArray();
    for(PRUint16 i = 0; i < count; i++)
    {
        if(Addition && i == Position)
        {
            if(Addition != *(CurrentInTable++))
                return JS_FALSE;
        }
        else
        {
            if(*(Current++) != *(CurrentInTable++))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
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
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}




JSBool JS_DLL_CALLBACK
IID2ThisTranslatorMap::Entry::Match(JSDHashTable *table,
                                    const JSDHashEntryHdr *entry,
                                    const void *key)
{
    return ((const nsID*)key)->Equals(((Entry*)entry)->key);
}

void JS_DLL_CALLBACK
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
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}



JSDHashNumber JS_DLL_CALLBACK
XPCNativeScriptableSharedMap::Entry::Hash(JSDHashTable *table, const void *key)
{
    JSDHashNumber h;
    const unsigned char *s;

    XPCNativeScriptableShared* obj =
        (XPCNativeScriptableShared*) key;

    

    h = (JSDHashNumber) obj->GetFlags();
    for (s = (const unsigned char*) obj->GetJSClass()->name; *s != '\0'; s++)
        h = (h >> (JS_DHASH_BITS - 4)) ^ (h << 4) ^ *s;
    return h;
}

JSBool JS_DLL_CALLBACK
XPCNativeScriptableSharedMap::Entry::Match(JSDHashTable *table,
                                         const JSDHashEntryHdr *entry,
                                         const void *key)
{
    XPCNativeScriptableShared* obj1 =
        ((XPCNativeScriptableSharedMap::Entry*) entry)->key;

    XPCNativeScriptableShared* obj2 =
        (XPCNativeScriptableShared*) key;

    

    if(obj1->GetFlags() != obj2->GetFlags())
        return JS_FALSE;

    const char* name1 = obj1->GetJSClass()->name;
    const char* name2 = obj2->GetJSClass()->name;

    if(!name1 || !name2)
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
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}

JSBool
XPCNativeScriptableSharedMap::GetNewOrUsed(JSUint32 flags,
                                           char* name,
                                           JSBool isGlobal,
                                           XPCNativeScriptableInfo* si)
{
    NS_PRECONDITION(name,"bad param");
    NS_PRECONDITION(si,"bad param");

    XPCNativeScriptableShared key(flags, name);

    Entry* entry = (Entry*)
        JS_DHashTableOperate(mTable, &key, JS_DHASH_ADD);
    if(!entry)
        return JS_FALSE;

    XPCNativeScriptableShared* shared = entry->key;

    if(!shared)
    {
        entry->key = shared =
            new XPCNativeScriptableShared(flags, key.TransferNameOwnership());
        if(!shared)
            return JS_FALSE;
        shared->PopulateJSClass(isGlobal);
    }
    si->SetScriptableShared(shared);
    return JS_TRUE;
}





XPCWrappedNativeProtoMap*
XPCWrappedNativeProtoMap::newMap(int size)
{
    XPCWrappedNativeProtoMap* map = new XPCWrappedNativeProtoMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}





XPCNativeWrapperMap*
XPCNativeWrapperMap::newMap(int size)
{
    XPCNativeWrapperMap* map = new XPCNativeWrapperMap(size);
    if(map && map->mTable)
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
    if(mTable)
        JS_DHashTableDestroy(mTable);
}


