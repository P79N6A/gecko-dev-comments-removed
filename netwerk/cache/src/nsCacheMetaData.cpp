







































#include "nsCacheMetaData.h"
#include "nsICacheEntryDescriptor.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "plstr.h"

nsCacheMetaData::nsCacheMetaData()
    : mData(nsnull), mMetaSize(0)
{
}

void
nsCacheMetaData::Clear()
{
    mMetaSize = 0;
    MetaElement * elem;
    while (mData) {
        elem = mData->mNext;
        delete mData;
        mData = elem;
    }
}

const char *
nsCacheMetaData::GetElement(const char * key)
{
    
    

    nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(key);

    MetaElement * elem = mData;
    while (elem) {
        if (elem->mKey == keyAtom)
            return elem->mValue;
        elem = elem->mNext;
    }
    return nsnull;
}


nsresult
nsCacheMetaData::SetElement(const char * key,
                            const char * value)
{
    nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(key);
    if (!keyAtom)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 keySize = strlen(key);
    PRUint32 valueSize = value ? strlen(value) : 0;

    
    MetaElement * elem = mData, * last = nsnull;
    while (elem) {
        if (elem->mKey == keyAtom) {
            
            PRUint32 oldValueLen = strlen(elem->mValue);
            if (valueSize == oldValueLen) {
                
                memcpy(elem->mValue, value, valueSize);
                return NS_OK;
            }
            
            if (last)
                last->mNext = elem->mNext;
            else
                mData = elem->mNext;
            
            mMetaSize -= 2 + keySize + oldValueLen;
            delete elem;
            break;
        }
        last = elem;
        elem = elem->mNext;
    }

    
    if (value) {
        elem = new (value, valueSize) MetaElement;
        if (!elem)
            return NS_ERROR_OUT_OF_MEMORY;
        elem->mKey = keyAtom;

        
        if (last) {
            elem->mNext = last->mNext;
            last->mNext = elem;
        }
        else {
            elem->mNext = mData;
            mData = elem;
        }

        
        mMetaSize += 2 + keySize + valueSize;
    }

    return NS_OK;
}

nsresult
nsCacheMetaData::FlattenMetaData(char * buffer, PRUint32 bufSize)
{
    const char *key;

    if (mMetaSize > bufSize) {
        NS_ERROR("buffer size too small for meta data.");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    MetaElement * elem = mData;
    while (elem) {
        elem->mKey->GetUTF8String(&key);

        PRUint32 keySize = 1 + strlen(key);
        memcpy(buffer, key, keySize);
        buffer += keySize;

        PRUint32 valSize = 1 + strlen(elem->mValue);
        memcpy(buffer, elem->mValue, valSize);
        buffer += valSize;

        elem = elem->mNext;
    }
    return NS_OK;
}

nsresult
nsCacheMetaData::UnflattenMetaData(const char * data, PRUint32 size)
{
    if (size == 0) return NS_OK;

    const char* limit = data + size;
    MetaElement * last = nsnull;

    while (data < limit) {
        const char* key = data;
        PRUint32 keySize = strlen(key);
        data += 1 + keySize;
        if (data < limit) {
            nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(key);
            if (!keyAtom)
                return NS_ERROR_OUT_OF_MEMORY;

            PRUint32 valueSize = strlen(data);
            MetaElement *elem = new (data, valueSize) MetaElement;
            if (!elem)
                 return NS_ERROR_OUT_OF_MEMORY;
            elem->mKey = keyAtom;

            
            if (last) {
                elem->mNext = last->mNext;
                last->mNext = elem;
            }
            else {
                elem->mNext = mData;
                mData = elem;
            }

            last = elem;
            data += 1 + valueSize;

            
            mMetaSize += 2 + keySize + valueSize;
        }
    }
    return NS_OK;
}

nsresult
nsCacheMetaData::VisitElements(nsICacheMetaDataVisitor * visitor)
{
    const char *key;

    MetaElement * elem = mData;
    while (elem) {
        elem->mKey->GetUTF8String(&key);

        PRBool keepGoing;
        nsresult rv = visitor->VisitMetaDataElement(key, elem->mValue, &keepGoing);

        if (NS_FAILED(rv) || !keepGoing)
            break;

        elem = elem->mNext;
    }

    return NS_OK;
}

void *
nsCacheMetaData::MetaElement::operator new(size_t size,
                                           const char *value,
                                           PRUint32 valueSize) CPP_THROW_NEW
{
    size += valueSize;

    MetaElement *elem = (MetaElement *) ::operator new(size);
    if (!elem)
        return nsnull;

    memcpy(elem->mValue, value, valueSize);
    elem->mValue[valueSize] = 0;

    return elem;
}
