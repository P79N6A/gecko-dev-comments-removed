






































#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsINode.h"

class nsDOMAttributeMap;
class nsIContent;

#define NS_IATTRIBUTE_IID  \
{ 0x68b13198, 0x6d81, 0x4ab6, \
  { 0xb9, 0x98, 0xd0, 0xa4, 0x55, 0x82, 0x5f, 0xb1 } }

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
  nsIAttribute(nsDOMAttributeMap *aAttrMap, nsINodeInfo *aNodeInfo)
    : nsINode(aNodeInfo), mAttrMap(aAttrMap)
  {
  }

  nsDOMAttributeMap *mAttrMap; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif 
