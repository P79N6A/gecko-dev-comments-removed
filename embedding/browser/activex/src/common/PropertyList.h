




































#ifndef PROPERTYLIST_H
#define PROPERTYLIST_H




class PropertyList
{
    struct Property {
        BSTR    bstrName;
        VARIANT vValue;
    } *mProperties;
    unsigned long mListSize;
    unsigned long mMaxListSize;

    bool EnsureMoreSpace()
    {
        
        const unsigned long kGrowBy = 10;
        if (!mProperties)
        {
            mProperties = (Property *) malloc(sizeof(Property) * kGrowBy);
            if (!mProperties)
                return false;
            mMaxListSize = kGrowBy;
        }
        else if (mListSize == mMaxListSize)
        {
            Property *pNewProperties;
            pNewProperties = (Property *) realloc(mProperties, sizeof(Property) * (mMaxListSize + kGrowBy));
            if (!pNewProperties)
                return false;
            mProperties = pNewProperties;
            mMaxListSize += kGrowBy;
        }
        return true;
    }

public:
    PropertyList() :
      mProperties(NULL),
      mListSize(0),
      mMaxListSize(0)
    {
    }
    ~PropertyList()
    {
    }
    void Clear()
    {
        if (mProperties)
        {
            for (unsigned long i = 0; i < mListSize; i++)
            {
                SysFreeString(mProperties[i].bstrName); 
                VariantClear(&mProperties[i].vValue);
            }
            free(mProperties);
            mProperties = NULL;
        }
        mListSize = 0;
        mMaxListSize = 0;
    }
    unsigned long GetSize() const
    {
        return mListSize;
    }
    const BSTR GetNameOf(unsigned long nIndex) const
    {
        if (nIndex > mListSize)
        {
            return NULL;
        }
        return mProperties[nIndex].bstrName;
    }
    const VARIANT *GetValueOf(unsigned long nIndex) const
    {
        if (nIndex > mListSize)
        {
            return NULL;
        }
        return &mProperties[nIndex].vValue;
    }
    bool AddOrReplaceNamedProperty(const BSTR bstrName, const VARIANT &vValue)
    {
        if (!bstrName)
            return false;
        for (unsigned long i = 0; i < GetSize(); i++)
        {
            
            if (wcsicmp(mProperties[i].bstrName, bstrName) == 0)
            {
                return SUCCEEDED(
                    VariantCopy(&mProperties[i].vValue, const_cast<VARIANT *>(&vValue)));
            }
        }
        return AddNamedProperty(bstrName, vValue);
    }
    bool AddNamedProperty(const BSTR bstrName, const VARIANT &vValue)
    {
        if (!bstrName || !EnsureMoreSpace())
            return false;
        Property *pProp = &mProperties[mListSize];
        pProp->bstrName = ::SysAllocString(bstrName);
        if (!pProp->bstrName)
        {
            return false;
        }
        VariantInit(&pProp->vValue);
        if (FAILED(VariantCopy(&pProp->vValue, const_cast<VARIANT *>(&vValue))))
        {
            SysFreeString(pProp->bstrName);
            return false;
        }
        mListSize++;
        return true;
    }
};

#endif