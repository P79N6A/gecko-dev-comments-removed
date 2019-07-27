




#ifndef txKey_h__
#define txKey_h__

#include "nsTHashtable.h"
#include "txNodeSet.h"
#include "txList.h"
#include "txXSLTPatterns.h"
#include "txXMLUtils.h"

class txPattern;
class Expr;
class txExecutionState;

class txKeyValueHashKey
{
public:
    txKeyValueHashKey(const txExpandedName& aKeyName,
                      int32_t aRootIdentifier,
                      const nsAString& aKeyValue)
        : mKeyName(aKeyName),
          mKeyValue(aKeyValue),
          mRootIdentifier(aRootIdentifier)
    {
    }

    txExpandedName mKeyName;
    nsString mKeyValue;
    int32_t mRootIdentifier;
};

struct txKeyValueHashEntry : public PLDHashEntryHdr
{
public:
    typedef const txKeyValueHashKey& KeyType;
    typedef const txKeyValueHashKey* KeyTypePointer;

    txKeyValueHashEntry(KeyTypePointer aKey)
        : mKey(*aKey),
          mNodeSet(new txNodeSet(nullptr)) { }

    txKeyValueHashEntry(const txKeyValueHashEntry& entry)
        : mKey(entry.mKey),
          mNodeSet(entry.mNodeSet) { }

    bool KeyEquals(KeyTypePointer aKey) const;

    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

    static PLDHashNumber HashKey(KeyTypePointer aKey);

    enum { ALLOW_MEMMOVE = true };
    
    txKeyValueHashKey mKey;
    nsRefPtr<txNodeSet> mNodeSet;
};

typedef nsTHashtable<txKeyValueHashEntry> txKeyValueHash;

class txIndexedKeyHashKey
{
public:
    txIndexedKeyHashKey(txExpandedName aKeyName,
                        int32_t aRootIdentifier)
        : mKeyName(aKeyName),
          mRootIdentifier(aRootIdentifier)
    {
    }

    txExpandedName mKeyName;
    int32_t mRootIdentifier;
};

struct txIndexedKeyHashEntry : public PLDHashEntryHdr
{
public:
    typedef const txIndexedKeyHashKey& KeyType;
    typedef const txIndexedKeyHashKey* KeyTypePointer;

    txIndexedKeyHashEntry(KeyTypePointer aKey)
        : mKey(*aKey),
          mIndexed(false) { }

    txIndexedKeyHashEntry(const txIndexedKeyHashEntry& entry)
        : mKey(entry.mKey),
          mIndexed(entry.mIndexed) { }

    bool KeyEquals(KeyTypePointer aKey) const;

    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

    static PLDHashNumber HashKey(KeyTypePointer aKey);

    enum { ALLOW_MEMMOVE = true };

    txIndexedKeyHashKey mKey;
    bool mIndexed;
};

typedef nsTHashtable<txIndexedKeyHashEntry> txIndexedKeyHash;





class txXSLKey {
    
public:
    txXSLKey(const txExpandedName& aName) : mName(aName)
    {
    }
    
    





    bool addKey(nsAutoPtr<txPattern>&& aMatch, nsAutoPtr<Expr>&& aUse);

    





    nsresult indexSubtreeRoot(const txXPathNode& aRoot,
                              txKeyValueHash& aKeyValueHash,
                              txExecutionState& aEs);

private:
    







    nsresult indexTree(const txXPathNode& aNode, txKeyValueHashKey& aKey,
                       txKeyValueHash& aKeyValueHash, txExecutionState& aEs);

    







    nsresult testNode(const txXPathNode& aNode, txKeyValueHashKey& aKey,
                      txKeyValueHash& aKeyValueHash, txExecutionState& aEs);

    


    struct Key {
        nsAutoPtr<txPattern> matchPattern;
        nsAutoPtr<Expr> useExpr;
    };

    


    nsTArray<Key> mKeys;
    
    


    txExpandedName mName;
};


class txKeyHash
{
public:
    txKeyHash(const txOwningExpandedNameMap<txXSLKey>& aKeys)
        : mKeyValues(4)
        , mIndexedKeys(1)
        , mKeys(aKeys)
    {
    }
    
    nsresult init();

    nsresult getKeyNodes(const txExpandedName& aKeyName,
                         const txXPathNode& aRoot,
                         const nsAString& aKeyValue,
                         bool aIndexIfNotFound,
                         txExecutionState& aEs,
                         txNodeSet** aResult);

private:
    
    txKeyValueHash mKeyValues;

    
    txIndexedKeyHash mIndexedKeys;
    
    
    const txOwningExpandedNameMap<txXSLKey>& mKeys;
    
    
    nsRefPtr<txNodeSet> mEmptyNodeSet;
};


#endif 
