






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsPIDOMEventTarget.h"

class nsIDOMWindow;
class nsIFocusController;
struct JSContext;


#define NS_IWINDOWROOT_IID \
{ 0xa9f58a8b, 0x55cd, 0x47fb, \
  { 0xae, 0xaa, 0xf5, 0x40, 0x10, 0xff, 0xd1, 0x54 } }

class nsPIWindowRoot : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  NS_IMETHOD GetFocusController(nsIFocusController** aResult)=0;

  virtual nsIDOMWindow* GetWindow()=0;

  virtual void SetParentTarget(nsPIDOMEventTarget* aTarget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
