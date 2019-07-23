





































#ifndef nsXBLPrototypeBinding_h__
#define nsXBLPrototypeBinding_h__

#include "nsCOMPtr.h"
#include "nsXBLPrototypeResources.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImplMethod.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"
#include "nsIContent.h"
#include "nsHashtable.h"
#include "nsIXBLDocumentInfo.h"
#include "nsCOMArray.h"
#include "nsXBLProtoImpl.h"

class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsSupportsHashtable;
class nsIXBLService;
class nsFixedSizeAllocator;
class nsXBLProtoImplField;
class nsXBLBinding;







class nsXBLPrototypeBinding
{
public:
  already_AddRefed<nsIContent> GetBindingElement();
  void SetBindingElement(nsIContent* aElement);

  nsIURI* BindingURI() const { return mBindingURI; }
  nsIURI* DocURI() const { return mXBLDocInfoWeak->DocumentURI(); }

  nsresult GetAllowScripts(PRBool* aResult);

  nsresult BindingAttached(nsIContent* aBoundElement);
  nsresult BindingDetached(nsIContent* aBoundElement);

  PRBool LoadResources();
  nsresult AddResource(nsIAtom* aResourceType, const nsAString& aSrc);

  PRBool InheritsStyle() const { return mInheritStyle; }

  nsXBLPrototypeHandler* GetPrototypeHandlers() { return mPrototypeHandler; }
  void SetPrototypeHandlers(nsXBLPrototypeHandler* aHandler) { mPrototypeHandler = aHandler; }

  nsXBLProtoImplAnonymousMethod* GetConstructor();
  nsresult SetConstructor(nsXBLProtoImplAnonymousMethod* aConstructor);
  nsXBLProtoImplAnonymousMethod* GetDestructor();
  nsresult SetDestructor(nsXBLProtoImplAnonymousMethod* aDestructor);

  nsXBLProtoImplField* FindField(const nsString& aFieldName) const
  {
    return mImplementation ? mImplementation->FindField(aFieldName) : nsnull;
  }

  
  
  PRBool ResolveAllFields(JSContext* cx, JSObject* obj) const
  {
    return !mImplementation || mImplementation->ResolveAllFields(cx, obj);
  }

  
  
  void UndefineFields(JSContext* cx, JSObject* obj) const {
    if (mImplementation) {
      mImplementation->UndefineFields(cx, obj);
    }
  }

  const nsCString& ClassName() const {
    return mImplementation ? mImplementation->mClassName : EmptyCString();
  }

  nsresult InitClass(const nsCString& aClassName, JSContext * aContext,
                     JSObject * aGlobal, JSObject * aScriptObject,
                     void ** aClassObject);

  nsresult ConstructInterfaceTable(const nsAString& aImpls);
  
  void SetImplementation(nsXBLProtoImpl* aImpl) { mImplementation = aImpl; }
  nsresult InstallImplementation(nsIContent* aBoundElement);
  PRBool HasImplementation() const { return mImplementation != nsnull; }

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        PRBool aRemoveFlag, nsIContent* aChangedElement,
                        nsIContent* aAnonymousContent, PRBool aNotify);

  void SetBasePrototype(nsXBLPrototypeBinding* aBinding);
  nsXBLPrototypeBinding* GetBasePrototype() { return mBaseBinding; }

  nsIXBLDocumentInfo* XBLDocumentInfo() const { return mXBLDocInfoWeak; }
  PRBool IsChrome() { return mXBLDocInfoWeak->IsChrome(); }
  
  PRBool HasBasePrototype() { return mHasBaseProto; }
  void SetHasBasePrototype(PRBool aHasBase) { mHasBaseProto = aHasBase; }

  void SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent);

  nsIStyleRuleProcessor* GetRuleProcessor();
  nsCOMArray<nsICSSStyleSheet>* GetStyleSheets();

  PRBool HasInsertionPoints() { return mInsertionPointTable != nsnull; }
  
  PRBool HasStyleSheets() {
    return mResources && mResources->mStyleSheetList.Count() > 0;
  }

  nsresult FlushSkinSheets();

  void InstantiateInsertionPoints(nsXBLBinding* aBinding);

  
  
  nsIContent* GetInsertionPoint(nsIContent* aBoundElement,
                                nsIContent* aCopyRoot, nsIContent *aChild,
                                PRUint32* aIndex);

  nsIContent* GetSingleInsertionPoint(nsIContent* aBoundElement,
                                      nsIContent* aCopyRoot,
                                      PRUint32* aIndex, PRBool* aMultiple);

  nsIAtom* GetBaseTag(PRInt32* aNamespaceID);
  void SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag);

  PRBool ImplementsInterface(REFNSIID aIID) const;

  nsresult AddResourceListener(nsIContent* aBoundElement);

  void Initialize();

  const nsCOMArray<nsXBLKeyEventHandler>* GetKeyEventHandlers()
  {
    if (!mKeyHandlersRegistered) {
      CreateKeyHandlers();
      mKeyHandlersRegistered = PR_TRUE;
    }

    return &mKeyHandlers;
  }

public:
  nsXBLPrototypeBinding();
  ~nsXBLPrototypeBinding();

  
  
  
  
  nsresult Init(const nsACString& aRef,
                nsIXBLDocumentInfo* aInfo,
                nsIContent* aElement);

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;
  void UnlinkJSObjects();
  void Trace(TraceCallback aCallback, void *aClosure) const;


  static PRUint32 gRefCnt;
 
  static nsFixedSizeAllocator* kAttrPool;





public:
  



  nsIContent* GetImmediateChild(nsIAtom* aTag);
  nsIContent* LocateInstance(nsIContent* aBoundElt,
                             nsIContent* aTemplRoot,
                             nsIContent* aCopyRoot,
                             nsIContent* aTemplChild);

protected:  
  void ConstructAttributeTable(nsIContent* aElement);
  void ConstructInsertionTable(nsIContent* aElement);
  void GetNestedChildren(nsIAtom* aTag, PRInt32 aNamespace,
                         nsIContent* aContent,
                         nsCOMArray<nsIContent> & aList);
  void CreateKeyHandlers();

protected:
  
  class nsIIDKey : public nsHashKey {
    protected:
      nsIID mKey;
  
    public:
      nsIIDKey(REFNSIID key) : mKey(key) {}
      ~nsIIDKey(void) {}

      PRUint32 HashCode(void) const {
        
        return mKey.m0;
      }

      PRBool Equals(const nsHashKey *aKey) const {
        return mKey.Equals( ((nsIIDKey*) aKey)->mKey);
      }

      nsHashKey *Clone(void) const {
        return new nsIIDKey(mKey);
      }
  };


protected:
  nsCOMPtr<nsIURI> mBindingURI;
  nsCOMPtr<nsIContent> mBinding; 
  nsAutoPtr<nsXBLPrototypeHandler> mPrototypeHandler; 
  
  nsXBLProtoImpl* mImplementation; 
                                   

  nsXBLPrototypeBinding* mBaseBinding; 
  PRPackedBool mInheritStyle;
  PRPackedBool mHasBaseProto;
  PRPackedBool mKeyHandlersRegistered;
 
  nsXBLPrototypeResources* mResources; 
                                      
  nsIXBLDocumentInfo* mXBLDocInfoWeak; 

  nsObjectHashtable* mAttributeTable; 
                                      
                                      

  nsObjectHashtable* mInsertionPointTable; 
                                           

  nsSupportsHashtable* mInterfaceTable; 

  PRInt32 mBaseNameSpaceID;    
  nsCOMPtr<nsIAtom> mBaseTag;  

  nsCOMArray<nsXBLKeyEventHandler> mKeyHandlers;
};

#endif

