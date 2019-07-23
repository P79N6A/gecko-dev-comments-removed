





















































#include "nsPropertyTable.h"
#include "pldhash.h"
#include "nsContentErrors.h"
#include "nsIAtom.h"

struct PropertyListMapEntry : public PLDHashEntryHdr {
  const void  *key;
  void        *value;
};



class nsPropertyTable::PropertyList {
public:
  PropertyList(PRUint16           aCategory,
               nsIAtom*           aName,
               NSPropertyDtorFunc aDtorFunc,
               void*              aDtorData,
               PRBool             aTransfer) NS_HIDDEN;
  ~PropertyList() NS_HIDDEN;

  
  
  NS_HIDDEN_(PRBool) DeletePropertyFor(nsPropertyOwner aObject);

  
  NS_HIDDEN_(void) Destroy();

  NS_HIDDEN_(PRBool) Equals(PRUint16 aCategory, nsIAtom *aPropertyName)
  {
    return mCategory == aCategory && mName == aPropertyName;
  }

  nsCOMPtr<nsIAtom>  mName;           
  PLDHashTable       mObjectValueMap; 
  NSPropertyDtorFunc mDtorFunc;       
  void*              mDtorData;       
  PRUint16           mCategory;       
  PRPackedBool       mTransfer;       
                                      
  
  PropertyList*      mNext;
};

void
nsPropertyTable::DeleteAllProperties()
{
  while (mPropertyList) {
    PropertyList* tmp = mPropertyList;

    mPropertyList = mPropertyList->mNext;
    tmp->Destroy();
    delete tmp;
  }
}

void
nsPropertyTable::DeleteAllPropertiesFor(nsPropertyOwner aObject)
{
  for (PropertyList* prop = mPropertyList; prop; prop = prop->mNext) {
    prop->DeletePropertyFor(aObject);
  }
}

nsresult
nsPropertyTable::TransferOrDeleteAllPropertiesFor(nsPropertyOwner aObject,
                                                  nsPropertyTable *aOtherTable)
{
  nsresult rv = NS_OK;
  for (PropertyList* prop = mPropertyList; prop; prop = prop->mNext) {
    if (prop->mTransfer) {
      PropertyListMapEntry *entry = NS_STATIC_CAST(PropertyListMapEntry*,
          PL_DHashTableOperate(&prop->mObjectValueMap, aObject,
                               PL_DHASH_LOOKUP));
      if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        rv = aOtherTable->SetProperty(aObject, prop->mCategory, prop->mName,
                                      entry->value, prop->mDtorFunc,
                                      prop->mDtorData, prop->mTransfer);
        if (NS_FAILED(rv)) {
          DeleteAllPropertiesFor(aObject);
          aOtherTable->DeleteAllPropertiesFor(aObject);

          break;
        }

        PL_DHashTableRawRemove(&prop->mObjectValueMap, entry);
      }
    }
    else {
      prop->DeletePropertyFor(aObject);
    }
  }

  return rv;
}

void
nsPropertyTable::Enumerate(nsPropertyOwner aObject, PRUint16 aCategory,
                           NSPropertyFunc aCallback, void *aData)
{
  PropertyList* prop;
  for (prop = mPropertyList; prop; prop = prop->mNext) {
    if (prop->mCategory == aCategory) {
      PropertyListMapEntry *entry = NS_STATIC_CAST(PropertyListMapEntry*,
          PL_DHashTableOperate(&prop->mObjectValueMap, aObject,
                               PL_DHASH_LOOKUP));
      if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        aCallback(NS_CONST_CAST(void*, aObject.get()), prop->mName, entry->value,
                  aData);
      }
    }
  }
}

void*
nsPropertyTable::GetPropertyInternal(nsPropertyOwner aObject,
                                     PRUint16    aCategory,
                                     nsIAtom    *aPropertyName,
                                     PRBool      aRemove,
                                     nsresult   *aResult)
{
  NS_PRECONDITION(aPropertyName && aObject, "unexpected null param");
  nsresult rv = NS_PROPTABLE_PROP_NOT_THERE;
  void *propValue = nsnull;

  PropertyList* propertyList = GetPropertyListFor(aCategory, aPropertyName);
  if (propertyList) {
    PropertyListMapEntry *entry = NS_STATIC_CAST(PropertyListMapEntry*,
        PL_DHashTableOperate(&propertyList->mObjectValueMap, aObject,
                             PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      propValue = entry->value;
      if (aRemove) {
        
        PL_DHashTableRawRemove(&propertyList->mObjectValueMap, entry);
      }
      rv = NS_OK;
    }
  }

  if (aResult)
    *aResult = rv;

  return propValue;
}

nsresult
nsPropertyTable::SetPropertyInternal(nsPropertyOwner     aObject,
                                     PRUint16            aCategory,
                                     nsIAtom            *aPropertyName,
                                     void               *aPropertyValue,
                                     NSPropertyDtorFunc  aPropDtorFunc,
                                     void               *aPropDtorData,
                                     PRBool              aTransfer,
                                     void              **aOldValue)
{
  NS_PRECONDITION(aPropertyName && aObject, "unexpected null param");

  PropertyList* propertyList = GetPropertyListFor(aCategory, aPropertyName);

  if (propertyList) {
    
    if (aPropDtorFunc != propertyList->mDtorFunc ||
        aPropDtorData != propertyList->mDtorData ||
        aTransfer != propertyList->mTransfer) {
      NS_WARNING("Destructor/data mismatch while setting property");
      return NS_ERROR_INVALID_ARG;
    }

  } else {
    propertyList = new PropertyList(aCategory, aPropertyName, aPropDtorFunc,
                                    aPropDtorData, aTransfer);
    if (!propertyList || !propertyList->mObjectValueMap.ops) {
      delete propertyList;
      return NS_ERROR_OUT_OF_MEMORY;
    }

    propertyList->mNext = mPropertyList;
    mPropertyList = propertyList;
  }

  
  
  nsresult result = NS_OK;
  PropertyListMapEntry *entry = NS_STATIC_CAST(PropertyListMapEntry*,
    PL_DHashTableOperate(&propertyList->mObjectValueMap, aObject, PL_DHASH_ADD));
  if (!entry)
    return NS_ERROR_OUT_OF_MEMORY;
  
  
  if (entry->key) {
    if (aOldValue)
      *aOldValue = entry->value;
    else if (propertyList->mDtorFunc)
      propertyList->mDtorFunc(NS_CONST_CAST(void*, entry->key), aPropertyName,
                              entry->value, propertyList->mDtorData);
    result = NS_PROPTABLE_PROP_OVERWRITTEN;
  }
  else if (aOldValue) {
    *aOldValue = nsnull;
  }
  entry->key = aObject;
  entry->value = aPropertyValue;

  return result;
}

nsresult
nsPropertyTable::DeleteProperty(nsPropertyOwner aObject,
                                PRUint16    aCategory,
                                nsIAtom    *aPropertyName)
{
  NS_PRECONDITION(aPropertyName && aObject, "unexpected null param");

  PropertyList* propertyList = GetPropertyListFor(aCategory, aPropertyName);
  if (propertyList) {
    if (propertyList->DeletePropertyFor(aObject))
      return NS_OK;
  }

  return NS_PROPTABLE_PROP_NOT_THERE;
}

nsPropertyTable::PropertyList*
nsPropertyTable::GetPropertyListFor(PRUint16 aCategory,
                                    nsIAtom* aPropertyName) const
{
  PropertyList* result;

  for (result = mPropertyList; result; result = result->mNext) {
    if (result->Equals(aCategory, aPropertyName)) {
      break;
    }
  }

  return result;
}


    
nsPropertyTable::PropertyList::PropertyList(PRUint16            aCategory,
                                            nsIAtom            *aName,
                                            NSPropertyDtorFunc  aDtorFunc,
                                            void               *aDtorData,
                                            PRBool              aTransfer)
  : mName(aName),
    mDtorFunc(aDtorFunc),
    mDtorData(aDtorData),
    mCategory(aCategory),
    mTransfer(aTransfer),
    mNext(nsnull)
{
  PL_DHashTableInit(&mObjectValueMap, PL_DHashGetStubOps(), this,
                    sizeof(PropertyListMapEntry), 16);
}

nsPropertyTable::PropertyList::~PropertyList()
{
  PL_DHashTableFinish(&mObjectValueMap);
}


PR_STATIC_CALLBACK(PLDHashOperator)
DestroyPropertyEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                          PRUint32 number, void *arg)
{
  nsPropertyTable::PropertyList *propList =
      NS_STATIC_CAST(nsPropertyTable::PropertyList*, table->data);
  PropertyListMapEntry* entry = NS_STATIC_CAST(PropertyListMapEntry*, hdr);

  propList->mDtorFunc(NS_CONST_CAST(void*, entry->key), propList->mName,
                      entry->value, propList->mDtorData);
  return PL_DHASH_NEXT;
}

void
nsPropertyTable::PropertyList::Destroy()
{
  
  if (mDtorFunc)
    PL_DHashTableEnumerate(&mObjectValueMap, DestroyPropertyEnumerator,
                           nsnull);
}

PRBool
nsPropertyTable::PropertyList::DeletePropertyFor(nsPropertyOwner aObject)
{
  PropertyListMapEntry *entry = NS_STATIC_CAST(PropertyListMapEntry*,
      PL_DHashTableOperate(&mObjectValueMap, aObject, PL_DHASH_LOOKUP));
  if (!PL_DHASH_ENTRY_IS_BUSY(entry))
    return PR_FALSE;

  void* value = entry->value;
  PL_DHashTableRawRemove(&mObjectValueMap, entry);

  if (mDtorFunc)
    mDtorFunc(NS_CONST_CAST(void*, aObject.get()), mName, value, mDtorData);

  return PR_TRUE;
}


void
nsPropertyTable::SupportsDtorFunc(void *aObject, nsIAtom *aPropertyName,
                                  void *aPropertyValue, void *aData)
{
  nsISupports *propertyValue = NS_STATIC_CAST(nsISupports*, aPropertyValue);
  NS_IF_RELEASE(propertyValue);
}
