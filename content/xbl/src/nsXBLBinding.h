





































#ifndef nsXBLBinding_h_
#define nsXBLBinding_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsINodeList.h"
#include "nsIStyleRuleProcessor.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"

class nsXBLPrototypeBinding;
class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsObjectHashtable;
class nsXBLInsertionPoint;
typedef nsTArray<nsRefPtr<nsXBLInsertionPoint> > nsInsertionPointList;
struct JSContext;
struct JSObject;




class nsXBLBinding
{
public:
  nsXBLBinding(nsXBLPrototypeBinding* aProtoBinding);
  ~nsXBLBinding();

  









  NS_INLINE_DECL_REFCOUNTING(nsXBLBinding)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)

  nsXBLPrototypeBinding* PrototypeBinding() { return mPrototypeBinding; }
  nsIContent* GetAnonymousContent() { return mContent.get(); }

  nsXBLBinding* GetBaseBinding() { return mNextBinding; }
  void SetBaseBinding(nsXBLBinding *aBinding);

  nsIContent* GetBoundElement() { return mBoundElement; }
  void SetBoundElement(nsIContent *aElement);

  bool IsStyleBinding() const { return mIsStyleBinding; }
  void SetIsStyleBinding(bool aIsStyle) { mIsStyleBinding = aIsStyle; }

  void MarkForDeath();
  bool MarkedForDeath() const { return mMarkedForDeath; }

  bool HasStyleSheets() const;
  bool InheritsStyle() const;
  bool ImplementsInterface(REFNSIID aIID) const;

  void GenerateAnonymousContent();
  void InstallAnonymousContent(nsIContent* aAnonParent, nsIContent* aElement);
  static void UninstallAnonymousContent(nsIDocument* aDocument,
                                        nsIContent* aAnonParent);
  void InstallEventHandlers();
  nsresult InstallImplementation();

  void ExecuteAttachedHandler();
  void ExecuteDetachedHandler();
  void UnhookEventHandlers();

  nsIAtom* GetBaseTag(PRInt32* aNameSpaceID);
  nsXBLBinding* RootBinding();
  nsXBLBinding* GetFirstStyleBinding();

  
  
  bool ResolveAllFields(JSContext *cx, JSObject *obj) const;

  
  
  nsresult GetInsertionPointsFor(nsIContent* aParent,
                                 nsInsertionPointList** aResult);

  nsInsertionPointList* GetExistingInsertionPointsFor(nsIContent* aParent);

  
  
  nsIContent* GetInsertionPoint(const nsIContent* aChild, PRUint32* aIndex);

  nsIContent* GetSingleInsertionPoint(PRUint32* aIndex,
                                      bool* aMultipleInsertionPoints);

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        bool aRemoveFlag, bool aNotify);

  void ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument);

  void WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData);

  nsINodeList* GetAnonymousNodes();

  static nsresult DoInitJSClass(JSContext *cx, JSObject *global, JSObject *obj,
                                const nsAFlatCString& aClassName,
                                nsXBLPrototypeBinding* aProtoBinding,
                                JSObject** aClassObject);

  bool AllowScripts();  

  void RemoveInsertionParent(nsIContent* aParent);
  bool HasInsertionParent(nsIContent* aParent);


protected:

  nsXBLPrototypeBinding* mPrototypeBinding; 
  nsCOMPtr<nsIContent> mContent; 
  nsRefPtr<nsXBLBinding> mNextBinding; 
  
  nsIContent* mBoundElement; 
  
  
  nsClassHashtable<nsISupportsHashKey, nsInsertionPointList>* mInsertionPointTable;

  bool mIsStyleBinding;
  bool mMarkedForDeath;
};

#endif 
