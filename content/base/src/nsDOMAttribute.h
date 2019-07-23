








































#ifndef nsDOMAttribute_h___
#define nsDOMAttribute_h___

#include "nsIAttribute.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsGenericDOMNodeList.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsINodeInfo.h"
#include "nsIDOM3Node.h"
#include "nsIDOM3Attr.h"
#include "nsDOMAttributeMap.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMAttribute;



class nsDOMAttribute : public nsIDOMAttr,
                       public nsIDOM3Attr,
                       public nsIAttribute
{
public:
  nsDOMAttribute(nsDOMAttributeMap* aAttrMap, nsINodeInfo *aNodeInfo,
                 const nsAString& aValue);

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
  NS_IMETHOD GetListenerManager(PRBool aCreateIfNotFound,
                                nsIEventListenerManager** aResult);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMAttribute, nsIAttribute)

protected:
  static PRBool sInitialized;

private:
  nsresult EnsureChildState(PRBool aSetText, PRBool &aHasChild) const;

  nsString mValue;
  
  
  nsCOMPtr<nsIContent> mChild;

  nsIContent *GetContentInternal() const
  {
    return mAttrMap ? mAttrMap->GetContent() : nsnull;
  }
};


#endif 
