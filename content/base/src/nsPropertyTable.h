





















































#ifndef nsPropertyTable_h_
#define nsPropertyTable_h_

#include "nscore.h"

class nsIAtom;
typedef unsigned long PtrBits;

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



#define DOM_USER_DATA         1
#define DOM_USER_DATA_HANDLER 2

class nsPropertyTable
{
 public:
  



  void* GetProperty(nsPropertyOwner aObject,
                    nsIAtom    *aPropertyName,
                    nsresult   *aResult = nsnull)
  {
    return GetPropertyInternal(aObject, 0, aPropertyName, PR_FALSE, aResult);
  }

  void* GetProperty(nsPropertyOwner aObject,
                    PRUint16    aCategory,
                    nsIAtom    *aPropertyName,
                    nsresult   *aResult = nsnull)
  {
    return GetPropertyInternal(aObject, aCategory, aPropertyName, PR_FALSE,
                               aResult);
  }

  
















  NS_HIDDEN_(nsresult) SetProperty(nsPropertyOwner     aObject,
                                   nsIAtom            *aPropertyName,
                                   void               *aPropertyValue,
                                   NSPropertyDtorFunc  aDtor,
                                   void               *aDtorData,
                                   PRBool              aTransfer = PR_FALSE,
                                   void              **aOldValue = nsnull)
  {
    return SetPropertyInternal(aObject, 0, aPropertyName, aPropertyValue,
                               aDtor, aDtorData, aTransfer, aOldValue);
  }

  
















  NS_HIDDEN_(nsresult) SetProperty(nsPropertyOwner     aObject,
                                   PRUint16            aCategory,
                                   nsIAtom            *aPropertyName,
                                   void               *aPropertyValue,
                                   NSPropertyDtorFunc  aDtor,
                                   void               *aDtorData,
                                   PRBool              aTransfer = PR_FALSE,
                                   void              **aOldValue = nsnull)
  {
    return SetPropertyInternal(aObject, aCategory, aPropertyName,
                               aPropertyValue, aDtor, aDtorData, aTransfer,
                               aOldValue);
  }

  



  NS_HIDDEN_(nsresult) DeleteProperty(nsPropertyOwner aObject,
                                      nsIAtom    *aPropertyName)
  {
    return DeleteProperty(aObject, 0, aPropertyName);
  }

  



  NS_HIDDEN_(nsresult) DeleteProperty(nsPropertyOwner aObject,
                                      PRUint16    aCategory,
                                      nsIAtom    *aPropertyName);

  




  void* UnsetProperty(nsPropertyOwner aObject,
                      nsIAtom    *aPropertyName,
                      nsresult   *aStatus = nsnull)
  {
    return GetPropertyInternal(aObject, 0, aPropertyName, PR_TRUE, aStatus);
  }

  




  void* UnsetProperty(nsPropertyOwner aObject,
                      PRUint16    aCategory,
                      nsIAtom    *aPropertyName,
                      nsresult   *aStatus = nsnull)
  {
    return GetPropertyInternal(aObject, aCategory, aPropertyName, PR_TRUE,
                               aStatus);
  }

  



  NS_HIDDEN_(void) DeleteAllPropertiesFor(nsPropertyOwner aObject);

  






  NS_HIDDEN_(nsresult)
    TransferOrDeleteAllPropertiesFor(nsPropertyOwner aObject,
                                     nsPropertyTable *aOtherTable);

  




  NS_HIDDEN_(void) Enumerate(nsPropertyOwner aObject, PRUint16 aCategory,
                             NSPropertyFunc aCallback, void *aData);

  



  NS_HIDDEN_(void) DeleteAllProperties();
  
  ~nsPropertyTable() {
    DeleteAllProperties();
  }

  




  static void SupportsDtorFunc(void *aObject, nsIAtom *aPropertyName,
                               void *aPropertyValue, void *aData);

  class PropertyList;

 private:
  NS_HIDDEN_(void) DestroyPropertyList();
  NS_HIDDEN_(PropertyList*) GetPropertyListFor(PRUint16 aCategory,
                                               nsIAtom *aPropertyName) const;
  NS_HIDDEN_(void*) GetPropertyInternal(nsPropertyOwner aObject,
                                        PRUint16    aCategory,
                                        nsIAtom    *aPropertyName,
                                        PRBool      aRemove,
                                        nsresult   *aStatus);
  NS_HIDDEN_(nsresult) SetPropertyInternal(nsPropertyOwner     aObject,
                                           PRUint16            aCategory,
                                           nsIAtom            *aPropertyName,
                                           void               *aPropertyValue,
                                           NSPropertyDtorFunc  aDtor,
                                           void               *aDtorData,
                                           PRBool              aTransfer,
                                           void              **aOldValue);

  PropertyList *mPropertyList;
};
#endif
