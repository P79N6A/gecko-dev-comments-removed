





































#ifndef nsXBLBinding_h_
#define nsXBLBinding_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIDOMNodeList.h"
#include "nsIStyleRuleProcessor.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"

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
  PRBool ShouldBuildChildFrames() const;

  void GenerateAnonymousContent();
  void InstallAnonymousContent(nsIContent* aAnonParent, nsIContent* aElement);
  void InstallEventHandlers();
  nsresult InstallImplementation();

  void ExecuteAttachedHandler();
  void ExecuteDetachedHandler();
  void UnhookEventHandlers();

  nsIAtom* GetBaseTag(PRInt32* aNameSpaceID);
  nsXBLBinding* GetFirstBindingWithConstructor();
  nsXBLBinding* RootBinding();
  nsXBLBinding* GetFirstStyleBinding();

  
  
  nsresult GetInsertionPointsFor(nsIContent* aParent,
                                 nsInsertionPointList** aResult);

  nsIContent* GetInsertionPoint(nsIContent* aChild, PRUint32* aIndex);

  nsIContent* GetSingleInsertionPoint(PRUint32* aIndex,
                                      PRBool* aMultipleInsertionPoints);

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        PRBool aRemoveFlag, PRBool aNotify);

  void ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument);

  void WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData);

  already_AddRefed<nsIDOMNodeList> GetAnonymousNodes();

  static nsresult DoInitJSClass(JSContext *cx, JSObject *global, JSObject *obj,
                                const nsAFlatCString& aClassName,
                                void **aClassObject);


protected:
  nsresult InitClass(const nsCString& aClassName, nsIScriptContext* aContext,
                     nsIDocument* aDocument, void** aScriptObject,
                     void** aClassObject);

  PRBool AllowScripts();  
  

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
