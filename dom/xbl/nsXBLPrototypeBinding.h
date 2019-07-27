




#ifndef nsXBLPrototypeBinding_h__
#define nsXBLPrototypeBinding_h__

#include "nsClassHashtable.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsICSSLoaderObserver.h"
#include "nsInterfaceHashtable.h"
#include "nsWeakReference.h"
#include "nsXBLDocumentInfo.h"
#include "nsXBLProtoImpl.h"
#include "nsXBLProtoImplMethod.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLPrototypeResources.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsXBLAttributeEntry;
class nsXBLBinding;
class nsXBLProtoImplField;

namespace mozilla {
class CSSStyleSheet;
} 







class nsXBLPrototypeBinding MOZ_FINAL
{
public:
  nsIContent* GetBindingElement() const { return mBinding; }
  void SetBindingElement(nsIContent* aElement);

  nsIURI* BindingURI() const { return mBindingURI; }
  nsIURI* AlternateBindingURI() const { return mAlternateBindingURI; }
  nsIURI* DocURI() const { return mXBLDocInfoWeak->DocumentURI(); }
  nsIURI* GetBaseBindingURI() const { return mBaseBindingURI; }

  
  
  bool CompareBindingURI(nsIURI* aURI) const;

  bool GetAllowScripts() const;

  nsresult BindingAttached(nsIContent* aBoundElement);
  nsresult BindingDetached(nsIContent* aBoundElement);

  bool LoadResources();
  nsresult AddResource(nsIAtom* aResourceType, const nsAString& aSrc);

  bool InheritsStyle() const { return mInheritStyle; }
  void SetInheritsStyle(bool aInheritStyle) { mInheritStyle = aInheritStyle; }

  nsXBLPrototypeHandler* GetPrototypeHandlers() { return mPrototypeHandler; }
  void SetPrototypeHandlers(nsXBLPrototypeHandler* aHandler) { mPrototypeHandler = aHandler; }

  nsXBLProtoImplAnonymousMethod* GetConstructor();
  nsresult SetConstructor(nsXBLProtoImplAnonymousMethod* aConstructor);
  nsXBLProtoImplAnonymousMethod* GetDestructor();
  nsresult SetDestructor(nsXBLProtoImplAnonymousMethod* aDestructor);

  nsXBLProtoImplField* FindField(const nsString& aFieldName) const
  {
    return mImplementation ? mImplementation->FindField(aFieldName) : nullptr;
  }

  
  
  bool ResolveAllFields(JSContext* cx, JS::Handle<JSObject*> obj) const
  {
    return !mImplementation || mImplementation->ResolveAllFields(cx, obj);
  }

  
  
  void UndefineFields(JSContext* cx, JS::Handle<JSObject*> obj) const {
    if (mImplementation) {
      mImplementation->UndefineFields(cx, obj);
    }
  }

  const nsCString& ClassName() const {
    return mImplementation ? mImplementation->mClassName : EmptyCString();
  }

  nsresult InitClass(const nsCString& aClassName, JSContext * aContext,
                     JS::Handle<JSObject*> aScriptObject,
                     JS::MutableHandle<JSObject*> aClassObject,
                     bool* aNew);

  nsresult ConstructInterfaceTable(const nsAString& aImpls);

  void SetImplementation(nsXBLProtoImpl* aImpl) { mImplementation = aImpl; }
  nsXBLProtoImpl* GetImplementation() { return mImplementation; }
  nsresult InstallImplementation(nsXBLBinding* aBinding);
  bool HasImplementation() const { return mImplementation != nullptr; }

  void AttributeChanged(nsIAtom* aAttribute, int32_t aNameSpaceID,
                        bool aRemoveFlag, nsIContent* aChangedElement,
                        nsIContent* aAnonymousContent, bool aNotify);

  void SetBasePrototype(nsXBLPrototypeBinding* aBinding);
  nsXBLPrototypeBinding* GetBasePrototype() { return mBaseBinding; }

  nsXBLDocumentInfo* XBLDocumentInfo() const { return mXBLDocInfoWeak; }
  bool IsChrome() { return mXBLDocInfoWeak->IsChrome(); }

  void SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent);

  void AppendStyleSheet(mozilla::CSSStyleSheet* aSheet);
  void RemoveStyleSheet(mozilla::CSSStyleSheet* aSheet);
  void InsertStyleSheetAt(size_t aIndex, mozilla::CSSStyleSheet* aSheet);
  mozilla::CSSStyleSheet* StyleSheetAt(size_t aIndex) const;
  size_t SheetCount() const;
  bool HasStyleSheets() const;
  void AppendStyleSheetsTo(nsTArray<mozilla::CSSStyleSheet*>& aResult) const;

  nsIStyleRuleProcessor* GetRuleProcessor();

  nsresult FlushSkinSheets();

  nsIAtom* GetBaseTag(int32_t* aNamespaceID);
  void SetBaseTag(int32_t aNamespaceID, nsIAtom* aTag);

  bool ImplementsInterface(REFNSIID aIID) const;

  nsresult AddResourceListener(nsIContent* aBoundElement);

  void Initialize();

  nsresult ResolveBaseBinding();

  const nsCOMArray<nsXBLKeyEventHandler>* GetKeyEventHandlers()
  {
    if (!mKeyHandlersRegistered) {
      CreateKeyHandlers();
      mKeyHandlersRegistered = true;
    }

    return &mKeyHandlers;
  }

private:
  nsresult Read(nsIObjectInputStream* aStream,
                nsXBLDocumentInfo* aDocInfo,
                nsIDocument* aDocument,
                uint8_t aFlags);

  










public:
  static nsresult ReadNewBinding(nsIObjectInputStream* aStream,
                                 nsXBLDocumentInfo* aDocInfo,
                                 nsIDocument* aDocument,
                                 uint8_t aFlags);

  


  nsresult Write(nsIObjectOutputStream* aStream);

  




  nsresult ReadContentNode(nsIObjectInputStream* aStream,
                           nsIDocument* aDocument,
                           nsNodeInfoManager* aNim,
                           nsIContent** aChild);

  





























  nsresult WriteContentNode(nsIObjectOutputStream* aStream, nsIContent* aNode);

  





  nsresult ReadNamespace(nsIObjectInputStream* aStream, int32_t& aNameSpaceID);
  nsresult WriteNamespace(nsIObjectOutputStream* aStream, int32_t aNameSpaceID);

public:
  nsXBLPrototypeBinding();
  ~nsXBLPrototypeBinding();

  
  
  
  
  nsresult Init(const nsACString& aRef,
                nsXBLDocumentInfo* aInfo,
                nsIContent* aElement,
                bool aFirstBinding = false);

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;
  void Unlink();
  void Trace(const TraceCallbacks& aCallbacks, void *aClosure) const;


public:
  



  nsIContent* GetImmediateChild(nsIAtom* aTag);
  nsIContent* LocateInstance(nsIContent* aBoundElt,
                             nsIContent* aTemplRoot,
                             nsIContent* aCopyRoot,
                             nsIContent* aTemplChild);

  bool ChromeOnlyContent() { return mChromeOnlyContent; }
  bool BindToUntrustedContent() { return mBindToUntrustedContent; }

  typedef nsClassHashtable<nsISupportsHashKey, nsXBLAttributeEntry> InnerAttributeTable;

protected:
  
  void EnsureAttributeTable();
  
  void AddToAttributeTable(int32_t aSourceNamespaceID, nsIAtom* aSourceTag,
                           int32_t aDestNamespaceID, nsIAtom* aDestTag,
                           nsIContent* aContent);
  void ConstructAttributeTable(nsIContent* aElement);
  void CreateKeyHandlers();

private:
  void EnsureResources();


protected:
  nsCOMPtr<nsIURI> mBindingURI;
  nsCOMPtr<nsIURI> mAlternateBindingURI; 
  nsCOMPtr<nsIContent> mBinding; 
  nsAutoPtr<nsXBLPrototypeHandler> mPrototypeHandler; 

  
  nsCOMPtr<nsIURI> mBaseBindingURI;

  nsXBLProtoImpl* mImplementation; 
                                   

  nsXBLPrototypeBinding* mBaseBinding; 
  bool mInheritStyle;
  bool mCheckedBaseProto;
  bool mKeyHandlersRegistered;
  bool mChromeOnlyContent;
  bool mBindToUntrustedContent;

  nsAutoPtr<nsXBLPrototypeResources> mResources; 

  nsXBLDocumentInfo* mXBLDocInfoWeak; 

  
  
  
  nsAutoPtr<nsClassHashtable<nsUint32HashKey, InnerAttributeTable>> mAttributeTable;

  class IIDHashKey : public PLDHashEntryHdr
  {
  public:
    typedef const nsIID& KeyType;
    typedef const nsIID* KeyTypePointer;

    explicit IIDHashKey(const nsIID* aKey)
      : mKey(*aKey)
    {}
    IIDHashKey(const IIDHashKey& aOther)
      : mKey(aOther.GetKey())
    {}
    ~IIDHashKey()
    {}

    KeyType GetKey() const
    {
      return mKey;
    }
    bool KeyEquals(const KeyTypePointer aKey) const
    {
      return mKey.Equals(*aKey);
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return &aKey;
    }
    static PLDHashNumber HashKey(const KeyTypePointer aKey)
    {
      
      return aKey->m0;
    }

    enum { ALLOW_MEMMOVE = true };

  private:
    nsIID mKey;
  };
  nsInterfaceHashtable<IIDHashKey, nsIContent> mInterfaceTable; 

  int32_t mBaseNameSpaceID;    
  nsCOMPtr<nsIAtom> mBaseTag;  

  nsCOMArray<nsXBLKeyEventHandler> mKeyHandlers;
};

#endif
