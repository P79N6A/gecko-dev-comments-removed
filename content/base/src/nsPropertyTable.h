





















#ifndef nsPropertyTable_h_
#define nsPropertyTable_h_

#include "mozilla/MemoryReporting.h"
#include "nscore.h"

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

  
  
  
  MOZ_IMPLICIT nsPropertyOwner(const nsINode* aObject) : mObject(aObject) {}
  MOZ_IMPLICIT nsPropertyOwner(const nsIFrame* aObject) : mObject(aObject) {}

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

  
















  nsresult SetProperty(nsPropertyOwner     aObject,
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

  



  nsresult DeleteProperty(nsPropertyOwner aObject,
                                      nsIAtom    *aPropertyName);

  




  void* UnsetProperty(nsPropertyOwner aObject,
                      nsIAtom    *aPropertyName,
                      nsresult   *aStatus = nullptr)
  {
    return GetPropertyInternal(aObject, aPropertyName, true, aStatus);
  }

  



  void DeleteAllPropertiesFor(nsPropertyOwner aObject);

  






  nsresult
    TransferOrDeleteAllPropertiesFor(nsPropertyOwner aObject,
                                     nsPropertyTable *aOtherTable);

  




  void Enumerate(nsPropertyOwner aObject,
                             NSPropertyFunc aCallback, void *aData);

  




  void EnumerateAll(NSPropertyFunc aCallback, void *aData);

  



  void DeleteAllProperties();

  nsPropertyTable() : mPropertyList(nullptr) {}  
  ~nsPropertyTable() {
    DeleteAllProperties();
  }

  




  static void SupportsDtorFunc(void *aObject, nsIAtom *aPropertyName,
                               void *aPropertyValue, void *aData);

  class PropertyList;

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

 private:
  void DestroyPropertyList();
  PropertyList* GetPropertyListFor(nsIAtom *aPropertyName) const;
  void* GetPropertyInternal(nsPropertyOwner aObject,
                                        nsIAtom    *aPropertyName,
                                        bool        aRemove,
                                        nsresult   *aStatus);
  nsresult SetPropertyInternal(nsPropertyOwner     aObject,
                                           nsIAtom            *aPropertyName,
                                           void               *aPropertyValue,
                                           NSPropertyDtorFunc  aDtor,
                                           void               *aDtorData,
                                           bool                aTransfer,
                                           void              **aOldValue);

  PropertyList *mPropertyList;
};
#endif
