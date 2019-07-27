





#ifndef _nsIAccessibilityService_h_
#define _nsIAccessibilityService_h_

#include "nsIAccessibleRetrieval.h"
#include "nsIAccessibleEvent.h"

#include "nsAutoPtr.h"

namespace mozilla {
namespace a11y {

class Accessible;

} 
} 

class nsIPresShell;


#define NS_IACCESSIBILITYSERVICE_IID \
{ 0x0e7e6879, 0x854b, 0x4260, \
 { 0xbc, 0x6e, 0x52, 0x5b, 0x5f, 0xb5, 0xcf, 0x34 } }

class nsIAccessibilityService : public nsIAccessibleRetrieval
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IACCESSIBILITYSERVICE_IID)

  







  virtual mozilla::a11y::Accessible*
    GetRootDocumentAccessible(nsIPresShell* aPresShell, bool aCanCreate) = 0;

   



  virtual mozilla::a11y::Accessible*
    AddNativeRootAccessible(void* aAtkAccessible) = 0;
  virtual void
    RemoveNativeRootAccessible(mozilla::a11y::Accessible* aRootAccessible) = 0;

  





  virtual void FireAccessibleEvent(uint32_t aEvent,
                                   mozilla::a11y::Accessible* aTarget) = 0;

  


  virtual bool HasAccessible(nsIDOMNode* aDOMNode) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAccessibilityService,
                              NS_IACCESSIBILITYSERVICE_IID)



#define NS_ACCESSIBILITY_SERVICE_CID \
{ 0xde401c37, 0x9a7f, 0x4278, { 0xa6, 0xf8, 0x3d, 0xe2, 0x83, 0x39, 0x89, 0xef } }

extern nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult);

#endif
