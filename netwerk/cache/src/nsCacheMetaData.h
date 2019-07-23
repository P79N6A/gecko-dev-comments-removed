







































#ifndef _nsCacheMetaData_h_
#define _nsCacheMetaData_h_

#include "nspr.h"
#include "pldhash.h"
#include "nscore.h"
#include "nsIAtom.h"

class nsICacheMetaDataVisitor;

class nsCacheMetaData {
public:
    nsCacheMetaData();
    ~nsCacheMetaData()  { Clear(); }

    void                  Clear();
    PRBool                IsEmpty() { return (mData == nsnull); }

    const char *          GetElement(const char * key);

    nsresult              SetElement(const char * key,
                                     const char * value);

    PRUint32              Size(void) { return mMetaSize; }

    nsresult              FlattenMetaData(char * buffer, PRUint32 bufSize);

    nsresult              UnflattenMetaData(const char * buffer, PRUint32 bufSize);

    nsresult              VisitElements(nsICacheMetaDataVisitor * visitor);

private:

    struct MetaElement
    {
        struct MetaElement * mNext;
        nsCOMPtr<nsIAtom>    mKey;
        char                 mValue[1]; 

        
        void *operator new(size_t size,
                           const char *value,
                           PRUint32 valueSize) CPP_THROW_NEW;
    };

    MetaElement * mData;
    PRUint32      mMetaSize;
};

#endif 
