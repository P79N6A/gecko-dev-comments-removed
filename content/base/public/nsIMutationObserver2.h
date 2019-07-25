




































#ifndef nsIMutationObserver2_h___
#define nsIMutationObserver2_h___

#include "nsIMutationObserver.h"

class nsIContent;
class nsINode;

#define NS_IMUTATION_OBSERVER_2_IID \
{0x61ac1cfd, 0xf3ef, 0x4408, \
  {0x8a, 0x72, 0xee, 0xf0, 0x41, 0xbe, 0xc7, 0xe9 } }







class nsIMutationObserver2 : public nsIMutationObserver
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMUTATION_OBSERVER_2_IID)

  








  virtual void AttributeChildRemoved(nsINode* aAttribute,
                                     nsIContent* aChild) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMutationObserver2, NS_IMUTATION_OBSERVER_2_IID)

#define NS_DECL_NSIMUTATIONOBSERVER2_ATTRIBUTECHILDREMOVED                \
    virtual void AttributeChildRemoved(nsINode* aAttribute,               \
                                       nsIContent* aChild);

#define NS_DECL_NSIMUTATIONOBSERVER2                                      \
    NS_DECL_NSIMUTATIONOBSERVER                                           \
    NS_DECL_NSIMUTATIONOBSERVER2_ATTRIBUTECHILDREMOVED

#define NS_IMPL_NSIMUTATIONOBSERVER2_CONTENT(_class)                      \
NS_IMPL_NSIMUTATIONOBSERVER_CONTENT(_class)                               \
void                                                                      \
_class::AttributeChildRemoved(nsINode* aAttribute, nsIContent *aChild)    \
{                                                                         \
}


#endif 

