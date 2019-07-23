





































#include "txExpandedNameMap.h"
#include "txCore.h"

class txMapItemComparator
{
  public:
    PRBool Equals(const txExpandedNameMap_base::MapItem& aItem,
                  const txExpandedName& aKey) const {
      return aItem.mNamespaceID == aKey.mNamespaceID &&
             aItem.mLocalName == aKey.mLocalName;
    }
};








nsresult txExpandedNameMap_base::addItem(const txExpandedName& aKey,
                                         void* aValue)
{
    PRUint32 pos = mItems.IndexOf(aKey, 0, txMapItemComparator());
    if (pos != mItems.NoIndex) {
        return NS_ERROR_XSLT_ALREADY_SET;
    }

    MapItem* item = mItems.AppendElement();
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    item->mNamespaceID = aKey.mNamespaceID;
    item->mLocalName = aKey.mLocalName;
    item->mValue = aValue;

    return NS_OK;
}








nsresult txExpandedNameMap_base::setItem(const txExpandedName& aKey,
                                         void* aValue,
                                         void** aOldValue)
{
    *aOldValue = nsnull;
    PRUint32 pos = mItems.IndexOf(aKey, 0, txMapItemComparator());
    if (pos != mItems.NoIndex) {
        *aOldValue = mItems[pos].mValue;
        mItems[pos].mValue = aValue;
        
        return NS_OK;
    }

    MapItem* item = mItems.AppendElement();
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    item->mNamespaceID = aKey.mNamespaceID;
    item->mLocalName = aKey.mLocalName;
    item->mValue = aValue;

    return NS_OK;
}






void* txExpandedNameMap_base::getItem(const txExpandedName& aKey) const
{
    PRUint32 pos = mItems.IndexOf(aKey, 0, txMapItemComparator());
    if (pos != mItems.NoIndex) {
        return mItems[pos].mValue;
    }

    return nsnull;
}







void* txExpandedNameMap_base::removeItem(const txExpandedName& aKey)
{
    void* value = nsnull;
    PRUint32 pos = mItems.IndexOf(aKey, 0, txMapItemComparator());
    if (pos != mItems.NoIndex) {
        value = mItems[pos].mValue;
        mItems.RemoveElementAt(pos);
    }

    return value;
}
