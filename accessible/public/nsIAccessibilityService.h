





#ifndef _nsIAccessibilityService_h_
#define _nsIAccessibilityService_h_

#include "nsIAccessibleRetrieval.h"
#include "nsIAccessibleEvent.h"

#include "nsAutoPtr.h"

class Accessible;
class nsINode;
class nsIContent;
class nsIDocument;
class nsIFrame;
class nsIPresShell;
class nsObjectFrame;


#define NS_IACCESSIBILITYSERVICE_IID \
{ 0x84dd9182, 0x6639, 0x4377, \
 { 0xa4, 0x13, 0xad, 0xe1, 0xae, 0x4e, 0x52, 0xdd } }

class nsIAccessibilityService : public nsIAccessibleRetrieval
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IACCESSIBILITYSERVICE_IID)

  







  virtual Accessible* GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                bool aCanCreate) = 0;

   



  virtual Accessible* AddNativeRootAccessible(void* aAtkAccessible) = 0;
  virtual void RemoveNativeRootAccessible(Accessible* aRootAccessible) = 0;

  





  virtual void FireAccessibleEvent(PRUint32 aEvent, Accessible* aTarget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAccessibilityService,
                              NS_IACCESSIBILITYSERVICE_IID)



#define NS_ACCESSIBILITY_SERVICE_CID \
{ 0xde401c37, 0x9a7f, 0x4278, { 0xa6, 0xf8, 0x3d, 0xe2, 0x83, 0x39, 0x89, 0xef } }

extern nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult);

#endif
