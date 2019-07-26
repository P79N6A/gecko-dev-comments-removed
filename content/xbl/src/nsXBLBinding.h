




#ifndef nsXBLBinding_h_
#define nsXBLBinding_h_

#include "nsXBLService.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsINodeList.h"
#include "nsIStyleRuleProcessor.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "js/TypeDecls.h"

class nsXBLPrototypeBinding;
class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;

namespace mozilla {
namespace dom {

class XBLChildrenElement;

}
}

class nsAnonymousContentList;




class nsXBLBinding
{
public:
  nsXBLBinding(nsXBLPrototypeBinding* aProtoBinding);
  ~nsXBLBinding();

  









  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsXBLBinding)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)

  nsXBLPrototypeBinding* PrototypeBinding() const { return mPrototypeBinding; }
  nsIContent* GetAnonymousContent() { return mContent.get(); }
  nsXBLBinding* GetBindingWithContent();

  nsXBLBinding* GetBaseBinding() const { return mNextBinding; }
  void SetBaseBinding(nsXBLBinding *aBinding);

  nsIContent* GetBoundElement() { return mBoundElement; }
  void SetBoundElement(nsIContent *aElement);

  void SetJSClass(nsXBLJSClass *aClass) {
    MOZ_ASSERT(!mJSClass && aClass);
    mJSClass = aClass;
  }

  







  bool LookupMember(JSContext* aCx, JS::Handle<jsid> aId,
                    JS::MutableHandle<JSPropertyDescriptor> aDesc);

  


  bool HasField(nsString& aName);

protected:

  


  bool LookupMemberInternal(JSContext* aCx, nsString& aName,
                            JS::Handle<jsid> aNameAsId,
                            JS::MutableHandle<JSPropertyDescriptor> aDesc,
                            JS::Handle<JSObject*> aXBLScope);

public:

  void MarkForDeath();
  bool MarkedForDeath() const { return mMarkedForDeath; }

  bool HasStyleSheets() const;
  bool InheritsStyle() const;
  bool ImplementsInterface(REFNSIID aIID) const;

  void GenerateAnonymousContent();
  void InstallAnonymousContent(nsIContent* aAnonParent, nsIContent* aElement,
                               bool aNativeAnon);
  static void UninstallAnonymousContent(nsIDocument* aDocument,
                                        nsIContent* aAnonParent);
  void InstallEventHandlers();
  nsresult InstallImplementation();

  void ExecuteAttachedHandler();
  void ExecuteDetachedHandler();
  void UnhookEventHandlers();

  nsIAtom* GetBaseTag(int32_t* aNameSpaceID);
  nsXBLBinding* RootBinding();

  
  
  bool ResolveAllFields(JSContext *cx, JS::Handle<JSObject*> obj) const;

  void AttributeChanged(nsIAtom* aAttribute, int32_t aNameSpaceID,
                        bool aRemoveFlag, bool aNotify);

  void ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument);

  void WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData);

  static nsresult DoInitJSClass(JSContext *cx, JS::Handle<JSObject*> global,
                                JS::Handle<JSObject*> obj,
                                const nsAFlatCString& aClassName,
                                nsXBLPrototypeBinding* aProtoBinding,
                                JS::MutableHandle<JSObject*> aClassObject,
                                bool* aNew);

  bool AllowScripts();  

  mozilla::dom::XBLChildrenElement* FindInsertionPointFor(nsIContent* aChild);

  bool HasFilteredInsertionPoints()
  {
    return !mInsertionPoints.IsEmpty();
  }

  mozilla::dom::XBLChildrenElement* GetDefaultInsertionPoint()
  {
    return mDefaultInsertionPoint;
  }

  
  void ClearInsertionPoints();

  
  
  nsAnonymousContentList* GetAnonymousNodeList();


protected:

  bool mMarkedForDeath;
  bool mUsingXBLScope;

  nsXBLPrototypeBinding* mPrototypeBinding; 
  nsCOMPtr<nsIContent> mContent; 
  nsRefPtr<nsXBLBinding> mNextBinding; 
  nsRefPtr<nsXBLJSClass> mJSClass; 
                                   
                                   

  nsIContent* mBoundElement; 

  
  
  
  
  
  
  nsRefPtr<mozilla::dom::XBLChildrenElement> mDefaultInsertionPoint;
  nsTArray<nsRefPtr<mozilla::dom::XBLChildrenElement> > mInsertionPoints;
  nsRefPtr<nsAnonymousContentList> mAnonymousContentList;

  mozilla::dom::XBLChildrenElement* FindInsertionPointForInternal(nsIContent* aChild);
};

#endif 
