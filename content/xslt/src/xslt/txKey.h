





































#ifndef txKey_h__
#define txKey_h__

#include "nsDoubleHashtable.h"
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
                      PRInt32 aRootIdentifier,
                      const nsAString& aKeyValue)
        : mKeyName(aKeyName),
          mKeyValue(aKeyValue),
          mRootIdentifier(aRootIdentifier)
    {
    }

    txExpandedName mKeyName;
    nsString mKeyValue;
    PRInt32 mRootIdentifier;
};

struct txKeyValueHashEntry : public PLDHashEntryHdr
{
    txKeyValueHashEntry(const void* aKey)
        : mKey(*static_cast<const txKeyValueHashKey*>(aKey)),
          mNodeSet(new txNodeSet(nsnull))
    {
    }

    
    PRBool MatchEntry(const void* aKey) const;
    static PLDHashNumber HashKey(const void* aKey);
    
    txKeyValueHashKey mKey;
    nsRefPtr<txNodeSet> mNodeSet;
};

DECL_DHASH_WRAPPER(txKeyValueHash, txKeyValueHashEntry, txKeyValueHashKey&)

class txIndexedKeyHashKey
{
public:
    txIndexedKeyHashKey(txExpandedName aKeyName,
                        PRInt32 aRootIdentifier)
        : mKeyName(aKeyName),
          mRootIdentifier(aRootIdentifier)
    {
    }

    txExpandedName mKeyName;
    PRInt32 mRootIdentifier;
};

struct txIndexedKeyHashEntry : public PLDHashEntryHdr
{
    txIndexedKeyHashEntry(const void* aKey)
        : mKey(*static_cast<const txIndexedKeyHashKey*>(aKey)),
          mIndexed(PR_FALSE)
    {
    }

    
    PRBool MatchEntry(const void* aKey) const;
    static PLDHashNumber HashKey(const void* aKey);

    txIndexedKeyHashKey mKey;
    PRBool mIndexed;
};

DECL_DHASH_WRAPPER(txIndexedKeyHash, txIndexedKeyHashEntry,
                   txIndexedKeyHashKey&)





class txXSLKey {
    
public:
    txXSLKey(const txExpandedName& aName) : mName(aName)
    {
    }
    
    





    PRBool addKey(nsAutoPtr<txPattern> aMatch, nsAutoPtr<Expr> aUse);

    





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
        : mKeys(aKeys)
    {
    }
    
    nsresult init();

    nsresult getKeyNodes(const txExpandedName& aKeyName,
                         const txXPathNode& aRoot,
                         const nsAString& aKeyValue,
                         PRBool aIndexIfNotFound,
                         txExecutionState& aEs,
                         txNodeSet** aResult);

private:
    
    txKeyValueHash mKeyValues;

    
    txIndexedKeyHash mIndexedKeys;
    
    
    const txOwningExpandedNameMap<txXSLKey>& mKeys;
    
    
    nsRefPtr<txNodeSet> mEmptyNodeSet;
};


#endif 
