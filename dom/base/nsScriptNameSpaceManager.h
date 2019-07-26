



















#ifndef nsScriptNameSpaceManager_h__
#define nsScriptNameSpaceManager_h__

#include "mozilla/MemoryReporting.h"
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

  
  union {
    mozilla::dom::DefineInterface mDefineDOMInterface; 
    mozilla::dom::ConstructNavigatorProperty mConstructNavigatorProperty; 
  };
  
  mozilla::dom::ConstructorEnabled* mConstructorEnabled;
};


class nsIScriptContext;
class nsICategoryManager;
class nsIMemoryReporter;
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

  
  
  
  
  const nsGlobalNameStruct* LookupNavigatorName(const nsAString& aName);

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
    mozilla::dom::DefineInterface aDefineDOMInterface,
    mozilla::dom::ConstructorEnabled* aConstructorEnabled);

  void RegisterNavigatorDOMConstructor(const nsAFlatString& aName,
    mozilla::dom::ConstructNavigatorProperty aNavConstructor,
    mozilla::dom::ConstructorEnabled* aConstructorEnabled);

  typedef PLDHashOperator
  (* NameEnumerator)(const nsAString& aGlobalName, void* aClosure);

  void EnumerateGlobalNames(NameEnumerator aEnumerator,
                            void* aClosure);
  void EnumerateNavigatorNames(NameEnumerator aEnumerator,
                               void* aClosure);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

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

  nsCOMPtr<nsIMemoryReporter> mReporter;
};

#endif 
