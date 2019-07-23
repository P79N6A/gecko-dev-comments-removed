








































#ifndef nsDOMAttribute_h___
#define nsDOMAttribute_h___

#include "nsIAttribute.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsINodeInfo.h"
#include "nsIDOM3Node.h"
#include "nsIDOM3Attr.h"
#include "nsDOMAttributeMap.h"
#include "nsCycleCollectionParticipant.h"
#include "nsContentUtils.h"
#include "nsIDOMXPathNSResolver.h"

class nsDOMAttribute;



class nsDOMAttribute : public nsIAttribute,
                       public nsIDOMAttr,
                       public nsIDOM3Attr,
                       public nsIDOMXPathNSResolver
{
public:
  nsDOMAttribute(nsDOMAttributeMap* aAttrMap, nsINodeInfo *aNodeInfo,
                 const nsAString& aValue);
  virtual ~nsDOMAttribute();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNODE

  
  NS_DECL_NSIDOM3NODE

  
  NS_DECL_NSIDOMATTR

  
  NS_DECL_NSIDOM3ATTR

  
  void SetMap(nsDOMAttributeMap *aMap);
  nsIContent *GetContent() const;
  nsresult SetOwnerDocument(nsIDocument* aDocument);

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsresult GetListenerManager(PRBool aCreateIfNotFound,
                                      nsIEventListenerManager** aResult);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv)
  {
    return nsContentUtils::GetContextForEventHandlers(this, aRv);
  }
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMAttribute, nsIAttribute)

protected:
  static PRBool sInitialized;

private:
  nsresult EnsureChildState(PRBool aSetText, PRBool &aHasChild) const;

  PRUint32 GetChildCount(PRBool aSetText) const
  {
    PRBool hasChild;
    EnsureChildState(aSetText, hasChild);

    return hasChild ? 1 : 0;
  }

  nsString mValue;
  
  
  
  nsIContent* mChild;

  nsIContent *GetContentInternal() const
  {
    return mAttrMap ? mAttrMap->GetContent() : nsnull;
  }
};


#endif 
