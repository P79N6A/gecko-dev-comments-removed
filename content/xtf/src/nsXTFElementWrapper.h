





































#ifndef __NS_XTFELEMENTWRAPPER_H__
#define __NS_XTFELEMENTWRAPPER_H__

#include "nsIXTFElementWrapper.h"
#include "nsXMLElement.h"
#include "nsIXTFAttributeHandler.h"
#include "nsIXTFElement.h"

typedef nsXMLElement nsXTFElementWrapperBase;



#define NS_XTFELEMENTWRAPPER_IID \
{ 0x599eb85f, 0xabc0, 0x4b52, { 0xa1, 0xb0, 0xea, 0x10, 0x3d, 0x48, 0xe3, 0xae } }


class nsXTFElementWrapper : public nsXTFElementWrapperBase,
                            public nsIXTFElementWrapper,
                            public nsXPCClassInfo
{
public:
  nsXTFElementWrapper(already_AddRefed<nsINodeInfo> aNodeInfo, nsIXTFElement* aXTFElement);
  virtual ~nsXTFElementWrapper();
  nsresult Init();

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XTFELEMENTWRAPPER_IID)

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXTFElementWrapper,
                                                     nsXTFElementWrapperBase)

  
  NS_DECL_NSIXTFELEMENTWRAPPER
    
  
#ifdef HAVE_CPP_AMBIGUITY_RESOLVING_USING
  using nsINode::GetProperty;
  using nsINode::SetProperty;
#endif

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                         PRBool aNotify);
  nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  nsIAtom *GetIDAttributeName() const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   nsIAtom* aPrefix, const nsAString& aValue,
                   PRBool aNotify);
  PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                 nsAString& aResult) const;
  PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRInt32 FindAttrValueIn(PRInt32 aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const;
  nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                     PRBool aNotify);
  const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const;
  PRUint32 GetAttrCount() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;

  virtual nsEventStates IntrinsicState() const;

  virtual void BeginAddingChildren();
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);

  virtual nsIAtom *GetClassAttributeName() const;
  virtual const nsAttrValue* DoGetClasses() const;

  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent);

  
  NS_IMETHOD GetAttribute(const nsAString& aName,
                          nsAString& aReturn);
  NS_IMETHOD RemoveAttribute(const nsAString& aName);
  NS_IMETHOD HasAttribute(const nsAString& aName, PRBool* aReturn);
  
  
  NS_DECL_NSICLASSINFO

  
  NS_FORWARD_SAFE_NSIXPCSCRIPTABLE(GetBaseXPCClassInfo())

  
  virtual void PreserveWrapper(nsISupports *aNative)
  {
    nsXPCClassInfo *ci = GetBaseXPCClassInfo();
    if (ci) {
      ci->PreserveWrapper(aNative);
    }
  }
  virtual PRUint32 GetInterfacesBitmap()
  {
    nsXPCClassInfo *ci = GetBaseXPCClassInfo();
    return ci ? ci->GetInterfacesBitmap() :  0;
  }

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  nsresult CloneState(nsIDOMElement *aElement)
  {
    return GetXTFElement()->CloneState(aElement);
  }
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo() { return this; }

  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo)
  {
  }

protected:
  virtual nsIXTFElement* GetXTFElement() const
  {
    return mXTFElement;
  }

  static nsXPCClassInfo* GetBaseXPCClassInfo()
  {
    return static_cast<nsXPCClassInfo*>(
      NS_GetDOMClassInfoInstance(eDOMClassInfo_Element_id));
  }

  
  PRBool QueryInterfaceInner(REFNSIID aIID, void** result);

  PRBool HandledByInner(nsIAtom* attr) const;

  void RegUnregAccessKey(PRBool aDoReg);

  nsCOMPtr<nsIXTFElement> mXTFElement;

  PRUint32 mNotificationMask;
  nsCOMPtr<nsIXTFAttributeHandler> mAttributeHandler;

  



  nsEventStates mIntrinsicState;

  
  nsAttrName mTmpAttrName;

  nsCOMPtr<nsIAtom> mClassAttributeName;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXTFElementWrapper, NS_XTFELEMENTWRAPPER_IID)

nsresult
NS_NewXTFElementWrapper(nsIXTFElement* aXTFElement, already_AddRefed<nsINodeInfo> aNodeInfo,
                        nsIContent** aResult);

#endif 
