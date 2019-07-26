




#ifndef TRANSFRMX_TXNAMESPACEMAP_H
#define TRANSFRMX_TXNAMESPACEMAP_H

#include "nsIAtom.h"
#include "nsCOMArray.h"
#include "nsTArray.h"

class txNamespaceMap
{
public:
    txNamespaceMap();
    txNamespaceMap(const txNamespaceMap& aOther);

    nsrefcnt AddRef()
    {
        return ++mRefCnt;
    }
    nsrefcnt Release()
    {
        if (--mRefCnt == 0) {
            mRefCnt = 1; 
            delete this;
            return 0;
        }
        return mRefCnt;
    }

    nsresult mapNamespace(nsIAtom* aPrefix, const nsAString& aNamespaceURI);
    int32_t lookupNamespace(nsIAtom* aPrefix);
    int32_t lookupNamespaceWithDefault(const nsAString& aPrefix);

private:
    nsAutoRefCnt mRefCnt;
    nsCOMArray<nsIAtom> mPrefixes;
    nsTArray<int32_t> mNamespaces;
};

#endif 
