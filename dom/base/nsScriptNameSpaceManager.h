



















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
    eTypeNewDOMBinding,
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
    int32_t mDOMClassInfoID; 
    nsIID mIID; 
    nsExternalDOMClassInfoData* mData; 
    ConstructorAlias* mAlias; 
    nsCID mCID; 
  };

  
  mozilla::dom::binding::DefineInterface mDefineDOMInterface;

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

  
  
  
  
  
  const nsGlobalNameStruct* LookupName(const nsAString& aName,
                                       const PRUnichar **aClassName = nullptr)
  {
    return LookupNameInternal(aName, aClassName);
  }

  
  
  
  
  nsresult LookupNavigatorName(const nsAString& aName,
                               const nsGlobalNameStruct **aNameStruct);

  nsresult RegisterClassName(const char *aClassName,
                             int32_t aDOMClassInfoID,
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
                             uint32_t aScriptableFlags,
                             bool aHasClassInterface,
                             const nsCID *aConstructorCID);

  nsGlobalNameStruct* GetConstructorProto(const nsGlobalNameStruct* aStruct);

  void RegisterDefineDOMInterface(const nsAFlatString& aName,
    mozilla::dom::binding::DefineInterface aDefineDOMInterface);

private:
  
  
  
  
  nsGlobalNameStruct *AddToHash(PLDHashTable *aTable, const nsAString *aKey,
                                const PRUnichar **aClassName = nullptr);
  nsGlobalNameStruct *AddToHash(PLDHashTable *aTable, const char *aKey,
                                const PRUnichar **aClassName = nullptr)
  {
    NS_ConvertASCIItoUTF16 key(aKey);
    return AddToHash(aTable, &key, aClassName);
  }

  nsresult FillHash(nsICategoryManager *aCategoryManager,
                    const char *aCategory);
  nsresult FillHashWithDOMInterfaces();
  nsresult RegisterInterface(const char* aIfName,
                             const nsIID *aIfIID,
                             bool* aFoundOld);

  








  nsresult AddCategoryEntryToHash(nsICategoryManager* aCategoryManager,
                                  const char* aCategory,
                                  nsISupports* aEntry);

  nsGlobalNameStruct* LookupNameInternal(const nsAString& aName,
                                         const PRUnichar **aClassName = nullptr);

  PLDHashTable mGlobalNames;
  PLDHashTable mNavigatorNames;

  bool mIsInitialized;
};

#endif 
