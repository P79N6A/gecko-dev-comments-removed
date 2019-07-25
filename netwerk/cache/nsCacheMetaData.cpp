







































#include "nsCacheMetaData.h"
#include "nsICacheEntryDescriptor.h"
#include "prmem.h"

const char *
nsCacheMetaData::GetElement(const char * key)
{
    const char * data = mBuffer;
    const char * limit = mBuffer + mMetaSize;

    while (data < limit) {
        
        const char * value = data + strlen(data) + 1;
        NS_ABORT_IF_FALSE(value < limit, "Cache Metadata corrupted");
        if (strcmp(data, key) == 0)
            return value;

        
        data = value + strlen(value) + 1;
    }
    NS_ABORT_IF_FALSE(data == limit, "Metadata corrupted");
    return nsnull;
}


nsresult
nsCacheMetaData::SetElement(const char * key,
                            const char * value)
{
    const PRUint32 keySize = strlen(key) + 1;
    char * pos = (char *)GetElement(key);

    if (!value) {
        
        if (pos) {
            PRUint32 oldValueSize = strlen(pos) + 1;
            PRUint32 offset = pos - mBuffer;
            PRUint32 remainder = mMetaSize - (offset + oldValueSize);

            memmove(pos - keySize, pos + oldValueSize, remainder);
            mMetaSize -= keySize + oldValueSize;
        }
        return NS_OK;
    }

    const PRUint32 valueSize = strlen(value) + 1;
    PRUint32 newSize = mMetaSize + valueSize;
    if (pos) {
        const PRUint32 oldValueSize = strlen(pos) + 1;
        const PRUint32 offset = pos - mBuffer;
        const PRUint32 remainder = mMetaSize - (offset + oldValueSize);

        
        newSize -= oldValueSize;
        nsresult rv = EnsureBuffer(newSize);
        NS_ENSURE_SUCCESS(rv, rv);

        
        pos = mBuffer + offset;
        memmove(pos + valueSize, pos + oldValueSize, remainder);
    } else {
        
        newSize += keySize;
        nsresult rv = EnsureBuffer(newSize);
        NS_ENSURE_SUCCESS(rv, rv);

        
        pos = mBuffer + mMetaSize;
        memcpy(pos, key, keySize);
        pos += keySize;
    }

    
    memcpy(pos, value, valueSize);
    mMetaSize = newSize;

    return NS_OK;
}

nsresult
nsCacheMetaData::FlattenMetaData(char * buffer, PRUint32 bufSize)
{
    if (mMetaSize > bufSize) {
        NS_ERROR("buffer size too small for meta data.");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(buffer, mBuffer, mMetaSize);
    return NS_OK;
}

nsresult
nsCacheMetaData::UnflattenMetaData(const char * data, PRUint32 size)
{
    if (data && size) {
        
        if (data[size-1] != '\0') {
            NS_ERROR("Cache MetaData is not null terminated");
            return NS_ERROR_ILLEGAL_VALUE;
        }
        
        
        bool odd = false;
        for (int i = 0; i < size; i++) {
            if (data[i] == '\0') 
                odd = !odd;
        }
        if (odd) {
            NS_ERROR("Cache MetaData is malformed");
            return NS_ERROR_ILLEGAL_VALUE;
        }

        nsresult rv = EnsureBuffer(size);
        NS_ENSURE_SUCCESS(rv, rv);

        memcpy(mBuffer, data, size);
        mMetaSize = size;
    }
    return NS_OK;
}

nsresult
nsCacheMetaData::VisitElements(nsICacheMetaDataVisitor * visitor)
{
    const char * data = mBuffer;
    const char * limit = mBuffer + mMetaSize;

    while (data < limit) {
        const char * key = data;
        
        data += strlen(data) + 1;
        NS_ABORT_IF_FALSE(data < limit, "Metadata corrupted");
        bool keepGoing;
        nsresult rv = visitor->VisitMetaDataElement(key, data, &keepGoing);
        if (NS_FAILED(rv) || !keepGoing)
            break;

        
        data += strlen(data) + 1;
    }
    NS_ABORT_IF_FALSE(data == limit, "Metadata corrupted");
    return NS_OK;
}

nsresult
nsCacheMetaData::EnsureBuffer(PRUint32 bufSize)
{
    if (mBufferSize < bufSize) {
        char * buf = (char *)PR_REALLOC(mBuffer, bufSize);
        if (!buf) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mBuffer = buf;
        mBufferSize = bufSize;
    }
    return NS_OK;
}        
