





















#ifndef nsPropertyTable_h_
#define nsPropertyTable_h_

#include "nscore.h"
#include "prtypes.h"

class nsIAtom;

typedef void
(*NSPropertyFunc)(void           *aObject,
                  nsIAtom        *aPropertyName,
                  void           *aPropertyValue,
                  void           *aData);







typedef NSPropertyFunc NSPropertyDtorFunc;
class nsINode;
class nsIFrame;

class nsPropertyOwner
{
public:
  nsPropertyOwner(const nsPropertyOwner& aOther) : mObject(aOther.mObject) {}

  
  
  
  nsPropertyOwner(const nsINode* aObject) : mObject(aObject) {}
  nsPropertyOwner(const nsIFrame* aObject) : mObject(aObject) {}

  operator const void*() { return mObject; }
  const void* get() { return mObject; }

private:
  const void* mObject;
};

class nsPropertyTable
{
 public:
  



  void* GetProperty(nsPropertyOwner aObject,
                    nsIAtom    *aPropertyName,
                    nsresult   *aResult = nullptr)
  {
    return GetPropertyInternal(aObject, aPropertyName, false, aResult);
  }

  
















  NS_HIDDEN_(nsresult) SetProperty(nsPropertyOwner     aObject,
                                   nsIAtom            *aPropertyName,
                                   void               *aPropertyValue,
                                   NSPropertyDtorFunc  aDtor,
                                   void               *aDtorData,
                                   bool                aTransfer = false,
                                   void              **aOldValue = nullptr)
  {
    return SetPropertyInternal(aObject, aPropertyName, aPropertyValue,
                               aDtor, aDtorData, aTransfer, aOldValue);
  }

  



  NS_HIDDEN_(nsresult) DeleteProperty(nsPropertyOwner aObject,
                                      nsIAtom    *aPropertyName);

  




  void* UnsetProperty(nsPropertyOwner aObject,
                      nsIAtom    *aPropertyName,
                      nsresult   *aStatus = nullptr)
  {
    return GetPropertyInternal(aObject, aPropertyName, true, aStatus);
  }

  



  NS_HIDDEN_(void) DeleteAllPropertiesFor(nsPropertyOwner aObject);

  






  NS_HIDDEN_(nsresult)
    TransferOrDeleteAllPropertiesFor(nsPropertyOwner aObject,
                                     nsPropertyTable *aOtherTable);

  




  NS_HIDDEN_(void) Enumerate(nsPropertyOwner aObject,
                             NSPropertyFunc aCallback, void *aData);

  




  NS_HIDDEN_(void) EnumerateAll(NSPropertyFunc aCallback, void *aData);

  



  NS_HIDDEN_(void) DeleteAllProperties();

  nsPropertyTable() : mPropertyList(nullptr) {}  
  ~nsPropertyTable() {
    DeleteAllProperties();
  }

  




  static void SupportsDtorFunc(void *aObject, nsIAtom *aPropertyName,
                               void *aPropertyValue, void *aData);

  class PropertyList;

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

 private:
  NS_HIDDEN_(void) DestroyPropertyList();
  NS_HIDDEN_(PropertyList*) GetPropertyListFor(nsIAtom *aPropertyName) const;
  NS_HIDDEN_(void*) GetPropertyInternal(nsPropertyOwner aObject,
                                        nsIAtom    *aPropertyName,
                                        bool        aRemove,
                                        nsresult   *aStatus);
  NS_HIDDEN_(nsresult) SetPropertyInternal(nsPropertyOwner     aObject,
                                           nsIAtom            *aPropertyName,
                                           void               *aPropertyValue,
                                           NSPropertyDtorFunc  aDtor,
                                           void               *aDtorData,
                                           bool                aTransfer,
                                           void              **aOldValue);

  PropertyList *mPropertyList;
};
#endif
