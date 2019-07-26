




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
#include "jsapi.h"

class nsXBLPrototypeBinding;
class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsObjectHashtable;
class nsXBLInsertionPoint;
typedef nsTArray<nsRefPtr<nsXBLInsertionPoint> > nsInsertionPointList;
struct JSContext;
class JSObject;




class nsXBLBinding
{
public:
  nsXBLBinding(nsXBLPrototypeBinding* aProtoBinding);
  ~nsXBLBinding();

  









  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsXBLBinding)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)

  nsXBLPrototypeBinding* PrototypeBinding() { return mPrototypeBinding; }
  nsIContent* GetAnonymousContent() { return mContent.get(); }

  nsXBLBinding* GetBaseBinding() { return mNextBinding; }
  void SetBaseBinding(nsXBLBinding *aBinding);

  nsIContent* GetBoundElement() { return mBoundElement; }
  void SetBoundElement(nsIContent *aElement);

  void SetJSClass(nsXBLJSClass *aClass) {
    MOZ_ASSERT(!mJSClass && aClass);
    mJSClass = aClass;
  }

  bool IsStyleBinding() const { return mIsStyleBinding; }
  void SetIsStyleBinding(bool aIsStyle) { mIsStyleBinding = aIsStyle; }

  







  bool LookupMember(JSContext* aCx, JS::HandleId aId, JSPropertyDescriptor* aDesc);

  


  bool HasField(nsString& aName);

protected:

  


  bool LookupMemberInternal(JSContext* aCx, nsString& aName, JS::HandleId aNameAsId,
                            JSPropertyDescriptor* aDesc, JSObject* aXBLScope);

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
  nsXBLBinding* GetFirstStyleBinding();

  
  
  bool ResolveAllFields(JSContext *cx, JSObject *obj) const;

  
  
  void GetInsertionPointsFor(nsIContent* aParent,
                             nsInsertionPointList** aResult);

  nsInsertionPointList* GetExistingInsertionPointsFor(nsIContent* aParent);

  
  
  nsIContent* GetInsertionPoint(const nsIContent* aChild, uint32_t* aIndex);

  nsIContent* GetSingleInsertionPoint(uint32_t* aIndex,
                                      bool* aMultipleInsertionPoints);

  void AttributeChanged(nsIAtom* aAttribute, int32_t aNameSpaceID,
                        bool aRemoveFlag, bool aNotify);

  void ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument);

  void WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData);

  nsINodeList* GetAnonymousNodes();

  static nsresult DoInitJSClass(JSContext *cx, JSObject *global, JSObject *obj,
                                const nsAFlatCString& aClassName,
                                nsXBLPrototypeBinding* aProtoBinding,
                                JSObject** aClassObject, bool* aNew);

  bool AllowScripts();  

  void RemoveInsertionParent(nsIContent* aParent);
  bool HasInsertionParent(nsIContent* aParent);


protected:

  bool mIsStyleBinding;
  bool mMarkedForDeath;

  nsXBLPrototypeBinding* mPrototypeBinding; 
  nsCOMPtr<nsIContent> mContent; 
  nsRefPtr<nsXBLBinding> mNextBinding; 
  nsRefPtr<nsXBLJSClass> mJSClass; 
                                   
                                   
  
  nsIContent* mBoundElement; 
  
  
  nsClassHashtable<nsISupportsHashKey, nsInsertionPointList>* mInsertionPointTable;
};

#endif 
