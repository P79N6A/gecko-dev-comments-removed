



















































#ifndef nsScriptNameSpaceManager_h__
#define nsScriptNameSpaceManager_h__

#include "nsIScriptNameSpaceManager.h"
#include "nsString.h"
#include "nsID.h"
#include "pldhash.h"
#include "nsDOMClassInfo.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"


struct nsGlobalNameStruct
{
  struct ConstructorAlias
  {
    nsCID mCID;
    nsString mProtoName;
    nsGlobalNameStruct* mProto;    
  };

  enum nametype {
    eTypeNotInitialized,
    eTypeInterface,
    eTypeProperty,
    eTypeNavigatorProperty,
    eTypeExternalConstructor,
    eTypeStaticNameSet,
    eTypeDynamicNameSet,
    eTypeClassConstructor,
    eTypeClassProto,
    eTypeExternalClassInfoCreator,
    eTypeExternalClassInfo,
    eTypeExternalConstructorAlias
  } mType;

  bool mChromeOnly;
  bool mDisabled;

  union {
    PRInt32 mDOMClassInfoID; 
    nsIID mIID; 
    nsExternalDOMClassInfoData* mData; 
    ConstructorAlias* mAlias; 
    nsCID mCID; 
  };

private:

  
};


class nsIScriptContext;
class nsICategoryManager;
class GlobalNameMapEntry;


class nsScriptNameSpaceManager : public nsIObserver,
                                 public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsScriptNameSpaceManager();
  virtual ~nsScriptNameSpaceManager();

  nsresult Init();
  nsresult InitForContext(nsIScriptContext *aContext);

  
  
  
  
  
  nsresult LookupName(const nsAString& aName,
                      const nsGlobalNameStruct **aNameStruct,
                      const PRUnichar **aClassName = nsnull);
  
  
  
  
  nsresult LookupNavigatorName(const nsAString& aName,
                               const nsGlobalNameStruct **aNameStruct);

  nsresult RegisterClassName(const char *aClassName,
                             PRInt32 aDOMClassInfoID,
                             bool aPrivileged,
                             bool aDisabled,
                             const PRUnichar **aResult);

  nsresult RegisterClassProto(const char *aClassName,
                              const nsIID *aConstructorProtoIID,
                              bool *aFoundOld);

  nsresult RegisterExternalInterfaces(bool aAsProto);

  nsresult RegisterExternalClassName(const char *aClassName,
                                     nsCID& aCID);

  
  
  nsresult RegisterDOMCIData(const char *aName,
                             nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                             const nsIID *aProtoChainInterface,
                             const nsIID **aInterfaces,
                             PRUint32 aScriptableFlags,
                             bool aHasClassInterface,
                             const nsCID *aConstructorCID);

  nsGlobalNameStruct* GetConstructorProto(const nsGlobalNameStruct* aStruct);

protected:
  
  
  
  
  nsGlobalNameStruct *AddToHash(PLDHashTable *aTable, const char *aKey,
                                const PRUnichar **aClassName = nsnull);

  nsresult FillHash(nsICategoryManager *aCategoryManager,
                    const char *aCategory);
  nsresult FillHashWithDOMInterfaces();
  nsresult RegisterInterface(const char* aIfName,
                             const nsIID *aIfIID,
                             bool* aFoundOld);

  








  nsresult AddCategoryEntryToHash(nsICategoryManager* aCategoryManager,
                                  const char* aCategory,
                                  nsISupports* aEntry);

  PLDHashTable mGlobalNames;
  PLDHashTable mNavigatorNames;

  bool mIsInitialized;
};

#endif 
