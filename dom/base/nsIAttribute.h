




#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsINode.h"

class nsDOMAttributeMap;

#define NS_IATTRIBUTE_IID  \
{ 0x233a9c4d, 0xb27f, 0x4662, \
    { 0xbd, 0x90, 0xba, 0xd6, 0x2e, 0x76, 0xc8, 0xe1 } }

class nsIAttribute : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IATTRIBUTE_IID)

  virtual void SetMap(nsDOMAttributeMap *aMap) = 0;
  
  nsDOMAttributeMap *GetMap()
  {
    return mAttrMap;
  }

  mozilla::dom::NodeInfo *NodeInfo() const
  {
    return mNodeInfo;
  }

  



  virtual nsresult SetOwnerDocument(nsIDocument* aDocument) = 0;

protected:
#ifdef MOZILLA_INTERNAL_API
  nsIAttribute(nsDOMAttributeMap *aAttrMap,
               already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
               bool aNsAware);
#endif 
  virtual ~nsIAttribute();

  nsRefPtr<nsDOMAttributeMap> mAttrMap;
  bool mNsAware;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif 
