





































#ifndef TRANSFRMX_TXNAMESPACEMAP_H
#define TRANSFRMX_TXNAMESPACEMAP_H

#include "nsIAtom.h"
#include "nsCOMArray.h"

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
    PRInt32 lookupNamespace(nsIAtom* aPrefix);
    PRInt32 lookupNamespace(const nsAString& aPrefix);
    PRInt32 lookupNamespaceWithDefault(const nsAString& aPrefix);

private:
    nsAutoRefCnt mRefCnt;
    nsCOMArray<nsIAtom> mPrefixes;
    nsVoidArray mNamespaces;
};

#endif 
