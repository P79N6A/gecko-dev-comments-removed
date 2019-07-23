






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsPIDOMEventTarget.h"

class nsIDOMWindow;
class nsIFocusController;


#define NS_IWINDOWROOT_IID \
{ 0x440f8d32, 0x818d, 0x468a, \
  { 0xac, 0x75, 0x59, 0x16, 0xfa, 0x1e, 0xa1, 0x98 } }

class nsPIWindowRoot : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  NS_IMETHOD GetFocusController(nsIFocusController** aResult)=0;

  virtual nsIDOMWindow* GetWindow();
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
