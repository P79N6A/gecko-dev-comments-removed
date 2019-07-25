



















































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
    eTypeExternalConstructor,
    eTypeStaticNameSet,
    eTypeDynamicNameSet,
    eTypeClassConstructor,
    eTypeClassProto,
    eTypeExternalClassInfoCreator,
    eTypeExternalClassInfo,
    eTypeExternalConstructorAlias
  } mType;

  PRBool mChromeOnly;
  PRBool mDisabled;

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

  nsresult RegisterClassName(const char *aClassName,
                             PRInt32 aDOMClassInfoID,
                             PRBool aPrivileged,
                             PRBool aDisabled,
                             const PRUnichar **aResult);

  nsresult RegisterClassProto(const char *aClassName,
                              const nsIID *aConstructorProtoIID,
                              PRBool *aFoundOld);

  nsresult RegisterExternalInterfaces(PRBool aAsProto);

  nsresult RegisterExternalClassName(const char *aClassName,
                                     nsCID& aCID);

  
  
  nsresult RegisterDOMCIData(const char *aName,
                             nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                             const nsIID *aProtoChainInterface,
                             const nsIID **aInterfaces,
                             PRUint32 aScriptableFlags,
                             PRBool aHasClassInterface,
                             const nsCID *aConstructorCID);

  nsGlobalNameStruct* GetConstructorProto(const nsGlobalNameStruct* aStruct);

protected:
  
  
  
  
  nsGlobalNameStruct *AddToHash(const char *aKey,
                                const PRUnichar **aClassName = nsnull);

  nsresult FillHash(nsICategoryManager *aCategoryManager,
                    const char *aCategory);
  nsresult FillHashWithDOMInterfaces();
  nsresult RegisterInterface(const char* aIfName,
                             const nsIID *aIfIID,
                             PRBool* aFoundOld);

  








  nsresult AddCategoryEntryToHash(nsICategoryManager* aCategoryManager,
                                  const char* aCategory,
                                  nsISupports* aEntry);

  
  
  PLDHashTable mGlobalNames;

  PRPackedBool mIsInitialized;
};

#endif 
