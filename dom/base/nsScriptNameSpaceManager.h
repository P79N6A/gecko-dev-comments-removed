



















#ifndef nsScriptNameSpaceManager_h__
#define nsScriptNameSpaceManager_h__

#include "mozilla/MemoryReporting.h"
#include "nsIMemoryReporter.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsString.h"
#include "nsID.h"
#include "pldhash.h"
#include "nsDOMClassInfo.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "xpcpublic.h"


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
    eTypeClassConstructor,
    eTypeClassProto,
    eTypeExternalClassInfoCreator,
    eTypeExternalClassInfo,
    eTypeExternalConstructorAlias
  } mType;

  
  
  
  bool mChromeOnly : 1;
  bool mAllowXBL : 1;

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

class nsICategoryManager;

class nsScriptNameSpaceManager : public nsIObserver,
                                 public nsSupportsWeakReference,
                                 public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEMORYREPORTER

  nsScriptNameSpaceManager();

  nsresult Init();

  
  
  
  
  
  const nsGlobalNameStruct* LookupName(const nsAString& aName,
                                       const char16_t **aClassName = nullptr)
  {
    return LookupNameInternal(aName, aClassName);
  }

  
  
  
  
  const nsGlobalNameStruct* LookupNavigatorName(const nsAString& aName);

  nsresult RegisterClassName(const char *aClassName,
                             int32_t aDOMClassInfoID,
                             bool aPrivileged,
                             bool aXBLAllowed,
                             const char16_t **aResult);

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
  template<size_t N>
  void RegisterDefineDOMInterface(const char16_t (&aKey)[N],
    mozilla::dom::DefineInterface aDefineDOMInterface,
    mozilla::dom::ConstructorEnabled* aConstructorEnabled)
  {
    nsLiteralString key(aKey);
    return RegisterDefineDOMInterface(key, aDefineDOMInterface,
                                      aConstructorEnabled);
  }

  void RegisterNavigatorDOMConstructor(const nsAFlatString& aName,
    mozilla::dom::ConstructNavigatorProperty aNavConstructor,
    mozilla::dom::ConstructorEnabled* aConstructorEnabled);
  template<size_t N>
  void RegisterNavigatorDOMConstructor(const char16_t (&aKey)[N],
    mozilla::dom::ConstructNavigatorProperty aNavConstructor,
    mozilla::dom::ConstructorEnabled* aConstructorEnabled)
  {
    nsLiteralString key(aKey);
    return RegisterNavigatorDOMConstructor(key, aNavConstructor,
                                           aConstructorEnabled);
  }

  typedef PLDHashOperator
  (* NameEnumerator)(const nsAString& aGlobalName,
                     const nsGlobalNameStruct& aGlobalNameStruct,
                     void* aClosure);

  void EnumerateGlobalNames(NameEnumerator aEnumerator,
                            void* aClosure);
  void EnumerateNavigatorNames(NameEnumerator aEnumerator,
                               void* aClosure);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

private:
  virtual ~nsScriptNameSpaceManager();

  
  
  
  
  nsGlobalNameStruct *AddToHash(PLDHashTable *aTable, const nsAString *aKey,
                                const char16_t **aClassName = nullptr);
  nsGlobalNameStruct *AddToHash(PLDHashTable *aTable, const char *aKey,
                                const char16_t **aClassName = nullptr)
  {
    NS_ConvertASCIItoUTF16 key(aKey);
    return AddToHash(aTable, &key, aClassName);
  }
  
  void RemoveFromHash(PLDHashTable *aTable, const nsAString *aKey);

  nsresult FillHash(nsICategoryManager *aCategoryManager,
                    const char *aCategory);
  nsresult RegisterInterface(const char* aIfName,
                             const nsIID *aIfIID,
                             bool* aFoundOld);

  








  nsresult AddCategoryEntryToHash(nsICategoryManager* aCategoryManager,
                                  const char* aCategory,
                                  nsISupports* aEntry);

  







  nsresult RemoveCategoryEntryFromHash(nsICategoryManager* aCategoryManager,
                                       const char* aCategory,
                                       nsISupports* aEntry);

  
  nsresult OperateCategoryEntryHash(nsICategoryManager* aCategoryManager,
                                    const char* aCategory,
                                    nsISupports* aEntry,
                                    bool aRemove);

  nsGlobalNameStruct* LookupNameInternal(const nsAString& aName,
                                         const char16_t **aClassName = nullptr);

  PLDHashTable mGlobalNames;
  PLDHashTable mNavigatorNames;

  bool mIsInitialized;
};

#endif 
