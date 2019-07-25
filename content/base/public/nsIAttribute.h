






































#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsINode.h"

class nsDOMAttributeMap;
class nsIContent;

#define NS_IATTRIBUTE_IID  \
{ 0xf809b623, 0x5b1e, 0x4121, \
  { 0xb8, 0x9d, 0x19, 0x24, 0x7b, 0x70, 0x77, 0x08 } }

class nsIAttribute : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IATTRIBUTE_IID)

  virtual void SetMap(nsDOMAttributeMap *aMap) = 0;
  
  nsDOMAttributeMap *GetMap()
  {
    return mAttrMap;
  }

  nsINodeInfo *NodeInfo()
  {
    return mNodeInfo;
  }

  virtual nsIContent* GetContent() const = 0;

  



  virtual nsresult SetOwnerDocument(nsIDocument* aDocument) = 0;

protected:
#ifdef MOZILLA_INTERNAL_API
  nsIAttribute(nsDOMAttributeMap *aAttrMap, already_AddRefed<nsINodeInfo> aNodeInfo,
               bool aNsAware)
    : nsINode(aNodeInfo), mAttrMap(aAttrMap), mNsAware(aNsAware)
  {
  }
#endif 

  nsDOMAttributeMap *mAttrMap; 
  bool mNsAware;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif 
