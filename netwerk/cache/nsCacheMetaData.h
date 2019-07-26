





#ifndef _nsCacheMetaData_h_
#define _nsCacheMetaData_h_

#include "nspr.h"
#include "nscore.h"

class nsICacheMetaDataVisitor;

class nsCacheMetaData {
public:
    nsCacheMetaData() : mBuffer(nullptr), mBufferSize(0), mMetaSize(0) { }

    ~nsCacheMetaData() {
        mBufferSize = mMetaSize = 0;  
        PR_FREEIF(mBuffer);
    }

    const char *  GetElement(const char * key);

    nsresult      SetElement(const char * key, const char * value);

    uint32_t      Size(void) { return mMetaSize; }

    nsresult      FlattenMetaData(char * buffer, uint32_t bufSize);

    nsresult      UnflattenMetaData(const char * buffer, uint32_t bufSize);

    nsresult      VisitElements(nsICacheMetaDataVisitor * visitor);

private:
    nsresult      EnsureBuffer(uint32_t size);

    char *        mBuffer;
    uint32_t      mBufferSize;
    uint32_t      mMetaSize;
};

#endif 
