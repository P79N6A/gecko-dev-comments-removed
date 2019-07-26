





#ifndef mozilla_dom_DocumentFragment_h__
#define mozilla_dom_DocumentFragment_h__

#include "mozilla/dom/FragmentOrElement.h"
#include "nsIDOMDocumentFragment.h"

class nsINodeInfo;
class nsIAtom;
class nsAString;
class nsIDocument;
class nsIContent;

namespace mozilla {
namespace dom {

class Element;

class DocumentFragment : public FragmentOrElement,
                         public nsIDOMDocumentFragment
{
public:
  using FragmentOrElement::GetFirstChild;

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  

  DocumentFragment(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~DocumentFragment()
  {
  }

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  
  virtual already_AddRefed<nsINodeInfo>
    GetExistingAttrNameFromQName(const nsAString& aStr) const
  {
    return nullptr;
  }

  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify)
  {
    return NS_OK;
  }
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute, 
                             bool aNotify)
  {
    return NS_OK;
  }
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const
  {
    return nullptr;
  }
  virtual uint32_t GetAttrCount() const
  {
    return 0;
  }

  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsIAtom* DoGetID() const;
  virtual nsIAtom *GetIDAttributeName() const;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
  {
    NS_ASSERTION(false, "Trying to bind a fragment to a tree");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  virtual void UnbindFromTree(bool aDeep, bool aNullParent)
  {
    NS_ASSERTION(false, "Trying to unbind a fragment from a tree");
    return;
  }

  virtual Element* GetNameSpaceElement()
  {
    return nullptr;
  }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif

protected:
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

} 
} 


#endif 
