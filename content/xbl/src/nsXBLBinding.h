





































#ifndef nsXBLBinding_h_
#define nsXBLBinding_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsINodeList.h"
#include "nsIStyleRuleProcessor.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

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

  









  nsrefcnt AddRef()
  {
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsXBLBinding", sizeof(nsXBLBinding));
    return mRefCnt;
  }

  nsrefcnt Release()
  {
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsXBLBinding");
    if (mRefCnt == 0) {
      mRefCnt = 1;
      delete this;
      return 0;
    }
    return mRefCnt;
  }

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)

  nsXBLPrototypeBinding* PrototypeBinding() { return mPrototypeBinding; }
  nsIContent* GetAnonymousContent() { return mContent.get(); }

  nsXBLBinding* GetBaseBinding() { return mNextBinding; }
  void SetBaseBinding(nsXBLBinding *aBinding);

  nsIContent* GetBoundElement() { return mBoundElement; }
  void SetBoundElement(nsIContent *aElement);

  PRBool IsStyleBinding() const { return mIsStyleBinding; }
  void SetIsStyleBinding(PRBool aIsStyle) { mIsStyleBinding = aIsStyle; }

  void MarkForDeath();
  PRBool MarkedForDeath() const { return mMarkedForDeath; }

  PRBool HasStyleSheets() const;
  PRBool InheritsStyle() const;
  PRBool ImplementsInterface(REFNSIID aIID) const;

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

  
  
  PRBool ResolveAllFields(JSContext *cx, JSObject *obj) const;

  
  
  nsresult GetInsertionPointsFor(nsIContent* aParent,
                                 nsInsertionPointList** aResult);

  nsInsertionPointList* GetExistingInsertionPointsFor(nsIContent* aParent);

  
  
  nsIContent* GetInsertionPoint(nsIContent* aChild, PRUint32* aIndex);

  nsIContent* GetSingleInsertionPoint(PRUint32* aIndex,
                                      PRBool* aMultipleInsertionPoints);

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        PRBool aRemoveFlag, PRBool aNotify);

  void ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument);

  void WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData);

  nsINodeList* GetAnonymousNodes();

  static nsresult DoInitJSClass(JSContext *cx, JSObject *global, JSObject *obj,
                                const nsAFlatCString& aClassName,
                                nsXBLPrototypeBinding* aProtoBinding,
                                void **aClassObject);

  PRBool AllowScripts();  

  void RemoveInsertionParent(nsIContent* aParent);
  PRBool HasInsertionParent(nsIContent* aParent);


protected:

  nsAutoRefCnt mRefCnt;
  nsXBLPrototypeBinding* mPrototypeBinding; 
  nsCOMPtr<nsIContent> mContent; 
  nsRefPtr<nsXBLBinding> mNextBinding; 
  
  nsIContent* mBoundElement; 
  
  
  nsClassHashtable<nsISupportsHashKey, nsInsertionPointList>* mInsertionPointTable;

  PRPackedBool mIsStyleBinding;
  PRPackedBool mMarkedForDeath;
};

#endif 
