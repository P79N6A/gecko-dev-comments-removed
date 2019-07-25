




#include "txNamespaceMap.h"
#include "nsGkAtoms.h"
#include "txXPathNode.h"

txNamespaceMap::txNamespaceMap()
{
}

txNamespaceMap::txNamespaceMap(const txNamespaceMap& aOther)
    : mPrefixes(aOther.mPrefixes)
{
    mNamespaces = aOther.mNamespaces; 
}

nsresult
txNamespaceMap::mapNamespace(nsIAtom* aPrefix, const nsAString& aNamespaceURI)
{
    nsIAtom* prefix = aPrefix == nsGkAtoms::_empty ? nsnull : aPrefix;

    PRInt32 nsId;
    if (prefix && aNamespaceURI.IsEmpty()) {
        
        PRInt32 index = mPrefixes.IndexOf(prefix);
        if (index >= 0) {
            mPrefixes.RemoveObjectAt(index);
            mNamespaces.RemoveElementAt(index);
        }

        return NS_OK;
    }

    if (aNamespaceURI.IsEmpty()) {
        
        nsId = kNameSpaceID_None;
    }
    else {
        nsId = txNamespaceManager::getNamespaceID(aNamespaceURI);
        NS_ENSURE_FALSE(nsId == kNameSpaceID_Unknown, NS_ERROR_FAILURE);
    }

    
    PRInt32 index = mPrefixes.IndexOf(prefix);
    if (index >= 0) {
        mNamespaces.ElementAt(index) = nsId;

        return NS_OK;
    }

    
    if (!mPrefixes.AppendObject(prefix)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    if (mNamespaces.AppendElement(nsId) == nsnull) {
        mPrefixes.RemoveObjectAt(mPrefixes.Count() - 1);

        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

PRInt32
txNamespaceMap::lookupNamespace(nsIAtom* aPrefix)
{
    if (aPrefix == nsGkAtoms::xml) {
        return kNameSpaceID_XML;
    }

    nsIAtom* prefix = aPrefix == nsGkAtoms::_empty ? 0 : aPrefix;

    PRInt32 index = mPrefixes.IndexOf(prefix);
    if (index >= 0) {
        return mNamespaces.SafeElementAt(index, kNameSpaceID_Unknown);
    }

    if (!prefix) {
        return kNameSpaceID_None;
    }

    return kNameSpaceID_Unknown;
}

PRInt32
txNamespaceMap::lookupNamespaceWithDefault(const nsAString& aPrefix)
{
    nsCOMPtr<nsIAtom> prefix = do_GetAtom(aPrefix);
    if (prefix != nsGkAtoms::_poundDefault) {
        return lookupNamespace(prefix);
    }

    return lookupNamespace(nsnull);
}
