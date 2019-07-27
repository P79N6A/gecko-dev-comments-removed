





#include "nsCacheMetaData.h"
#include "nsICacheEntryDescriptor.h"

const char *
nsCacheMetaData::GetElement(const char * key)
{
    const char * data = mBuffer;
    const char * limit = mBuffer + mMetaSize;

    while (data < limit) {
        
        const char * value = data + strlen(data) + 1;
        MOZ_ASSERT(value < limit, "Cache Metadata corrupted");
        if (strcmp(data, key) == 0)
            return value;

        
        data = value + strlen(value) + 1;
    }
    MOZ_ASSERT(data == limit, "Metadata corrupted");
    return nullptr;
}


nsresult
nsCacheMetaData::SetElement(const char * key,
                            const char * value)
{
    const uint32_t keySize = strlen(key) + 1;
    char * pos = (char *)GetElement(key);

    if (!value) {
        
        if (pos) {
            uint32_t oldValueSize = strlen(pos) + 1;
            uint32_t offset = pos - mBuffer;
            uint32_t remainder = mMetaSize - (offset + oldValueSize);

            memmove(pos - keySize, pos + oldValueSize, remainder);
            mMetaSize -= keySize + oldValueSize;
        }
        return NS_OK;
    }

    const uint32_t valueSize = strlen(value) + 1;
    uint32_t newSize = mMetaSize + valueSize;
    if (pos) {
        const uint32_t oldValueSize = strlen(pos) + 1;
        const uint32_t offset = pos - mBuffer;
        const uint32_t remainder = mMetaSize - (offset + oldValueSize);

        
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
nsCacheMetaData::FlattenMetaData(char * buffer, uint32_t bufSize)
{
    if (mMetaSize > bufSize) {
        NS_ERROR("buffer size too small for meta data.");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(buffer, mBuffer, mMetaSize);
    return NS_OK;
}

nsresult
nsCacheMetaData::UnflattenMetaData(const char * data, uint32_t size)
{
    if (data && size) {
        
        if (data[size-1] != '\0') {
            NS_ERROR("Cache MetaData is not null terminated");
            return NS_ERROR_ILLEGAL_VALUE;
        }
        
        
        bool odd = false;
        for (uint32_t i = 0; i < size; i++) {
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
        MOZ_ASSERT(data < limit, "Metadata corrupted");
        bool keepGoing;
        nsresult rv = visitor->VisitMetaDataElement(key, data, &keepGoing);
        if (NS_FAILED(rv) || !keepGoing)
            return NS_OK;

        
        data += strlen(data) + 1;
    }
    MOZ_ASSERT(data == limit, "Metadata corrupted");
    return NS_OK;
}

nsresult
nsCacheMetaData::EnsureBuffer(uint32_t bufSize)
{
    if (mBufferSize < bufSize) {
        char * buf = (char *)moz_realloc(mBuffer, bufSize);
        if (!buf) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mBuffer = buf;
        mBufferSize = bufSize;
    }
    return NS_OK;
}        
