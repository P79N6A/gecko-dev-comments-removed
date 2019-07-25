





































#ifndef nsXBLPrototypeBinding_h__
#define nsXBLPrototypeBinding_h__

#include "nsCOMPtr.h"
#include "nsXBLPrototypeResources.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImplMethod.h"
#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"
#include "nsIContent.h"
#include "nsHashtable.h"
#include "nsClassHashtable.h"
#include "nsXBLDocumentInfo.h"
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
class nsCSSStyleSheet;




struct InsertionItem {
  PRUint32 insertionIndex;
  nsIAtom* tag;
  nsIContent* defaultContent;

  InsertionItem(PRUint32 aInsertionIndex, nsIAtom* aTag, nsIContent* aDefaultContent)
    : insertionIndex(aInsertionIndex), tag(aTag), defaultContent(aDefaultContent) { }

  bool operator<(const InsertionItem& item) const
  {
    NS_ASSERTION(insertionIndex != item.insertionIndex || defaultContent == item.defaultContent,
                 "default content is different for same index");
    return insertionIndex < item.insertionIndex;
  }

  bool operator==(const InsertionItem& item) const
  {
    NS_ASSERTION(insertionIndex != item.insertionIndex || defaultContent == item.defaultContent,
                 "default content is different for same index");
    return insertionIndex == item.insertionIndex;
  }
};

typedef nsClassHashtable<nsISupportsHashKey, nsAutoTArray<InsertionItem, 1> > ArrayOfInsertionPointsByContent;







class nsXBLPrototypeBinding
{
public:
  already_AddRefed<nsIContent> GetBindingElement();
  void SetBindingElement(nsIContent* aElement);

  nsIURI* BindingURI() const { return mBindingURI; }
  nsIURI* AlternateBindingURI() const { return mAlternateBindingURI; }
  nsIURI* DocURI() const { return mXBLDocInfoWeak->DocumentURI(); }
  nsIURI* GetBaseBindingURI() const { return mBaseBindingURI; }

  
  
  bool CompareBindingURI(nsIURI* aURI) const;

  bool GetAllowScripts();

  nsresult BindingAttached(nsIContent* aBoundElement);
  nsresult BindingDetached(nsIContent* aBoundElement);

  bool LoadResources();
  nsresult AddResource(nsIAtom* aResourceType, const nsAString& aSrc);

  bool InheritsStyle() const { return mInheritStyle; }

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

  
  
  bool ResolveAllFields(JSContext* cx, JSObject* obj) const
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
                     JSObject** aClassObject);

  nsresult ConstructInterfaceTable(const nsAString& aImpls);
  
  void SetImplementation(nsXBLProtoImpl* aImpl) { mImplementation = aImpl; }
  nsresult InstallImplementation(nsIContent* aBoundElement);
  bool HasImplementation() const { return mImplementation != nsnull; }

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        bool aRemoveFlag, nsIContent* aChangedElement,
                        nsIContent* aAnonymousContent, bool aNotify);

  void SetBasePrototype(nsXBLPrototypeBinding* aBinding);
  nsXBLPrototypeBinding* GetBasePrototype() { return mBaseBinding; }

  nsXBLDocumentInfo* XBLDocumentInfo() const { return mXBLDocInfoWeak; }
  bool IsChrome() { return mXBLDocInfoWeak->IsChrome(); }
  
  void SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent);

  nsIStyleRuleProcessor* GetRuleProcessor();
  nsXBLPrototypeResources::sheet_array_type* GetStyleSheets();

  bool HasInsertionPoints() { return mInsertionPointTable != nsnull; }
  
  bool HasStyleSheets() {
    return mResources && mResources->mStyleSheetList.Length() > 0;
  }

  nsresult FlushSkinSheets();

  void InstantiateInsertionPoints(nsXBLBinding* aBinding);

  
  
  nsIContent* GetInsertionPoint(nsIContent* aBoundElement,
                                nsIContent* aCopyRoot,
                                const nsIContent *aChild,
                                PRUint32* aIndex);

  nsIContent* GetSingleInsertionPoint(nsIContent* aBoundElement,
                                      nsIContent* aCopyRoot,
                                      PRUint32* aIndex, bool* aMultiple);

  nsIAtom* GetBaseTag(PRInt32* aNamespaceID);
  void SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag);

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

  






  nsresult Read(nsIObjectInputStream* aStream,
                nsXBLDocumentInfo* aDocInfo,
                nsIDocument* aDocument,
                PRUint8 aFlags);

  


  nsresult Write(nsIObjectOutputStream* aStream);

  




  nsresult ReadContentNode(nsIObjectInputStream* aStream,
                           nsIDocument* aDocument,
                           nsNodeInfoManager* aNim,
                           nsIContent** aChild);

  





































  nsresult WriteContentNode(nsIObjectOutputStream* aStream,
                            nsIContent* aNode,
                            ArrayOfInsertionPointsByContent& aInsertionPointsByContent);

  





  nsresult ReadNamespace(nsIObjectInputStream* aStream, PRInt32& aNameSpaceID);
  nsresult WriteNamespace(nsIObjectOutputStream* aStream, PRInt32 aNameSpaceID);

public:
  nsXBLPrototypeBinding();
  ~nsXBLPrototypeBinding();

  
  
  
  
  nsresult Init(const nsACString& aRef,
                nsXBLDocumentInfo* aInfo,
                nsIContent* aElement,
                bool aFirstBinding = false);

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
  
  void EnsureAttributeTable();
  
  void AddToAttributeTable(PRInt32 aSourceNamespaceID, nsIAtom* aSourceTag,
                           PRInt32 aDestNamespaceID, nsIAtom* aDestTag,
                           nsIContent* aContent);
  void ConstructAttributeTable(nsIContent* aElement);
  void ConstructInsertionTable(nsIContent* aElement);
  void GetNestedChildren(nsIAtom* aTag, PRInt32 aNamespace,
                         nsIContent* aContent,
                         nsCOMArray<nsIContent> & aList);
  void CreateKeyHandlers();


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
 
  nsXBLPrototypeResources* mResources; 
                                      
  nsXBLDocumentInfo* mXBLDocInfoWeak; 

  nsObjectHashtable* mAttributeTable; 
                                      
                                      

  nsObjectHashtable* mInsertionPointTable; 
                                           

  nsSupportsHashtable* mInterfaceTable; 

  PRInt32 mBaseNameSpaceID;    
  nsCOMPtr<nsIAtom> mBaseTag;  

  nsCOMArray<nsXBLKeyEventHandler> mKeyHandlers;
};

#endif
