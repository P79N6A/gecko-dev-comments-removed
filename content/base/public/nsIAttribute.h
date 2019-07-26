




#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsINode.h"

class nsDOMAttributeMap;
class nsIContent;

#define NS_IATTRIBUTE_IID  \
{ 0x727dc139, 0xf516, 0x46ff, \
  { 0x86, 0x11, 0x18, 0xea, 0xee, 0x4a, 0x3e, 0x6a } }

class nsIAttribute : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IATTRIBUTE_IID)

  virtual void SetMap(nsDOMAttributeMap *aMap) = 0;
  
  nsDOMAttributeMap *GetMap()
  {
    return mAttrMap;
  }

  nsINodeInfo *NodeInfo() const
  {
    return mNodeInfo;
  }

  



  virtual nsresult SetOwnerDocument(nsIDocument* aDocument) = 0;

protected:
#ifdef MOZILLA_INTERNAL_API
  nsIAttribute(nsDOMAttributeMap *aAttrMap,
               already_AddRefed<nsINodeInfo>& aNodeInfo,
               bool aNsAware);
#endif 
  virtual ~nsIAttribute();

  nsRefPtr<nsDOMAttributeMap> mAttrMap;
  bool mNsAware;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif 
