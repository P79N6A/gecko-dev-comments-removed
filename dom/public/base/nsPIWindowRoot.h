






































#ifndef nsPIWindowRoot_h__
#define nsPIWindowRoot_h__

#include "nsISupports.h"
#include "nsPIDOMEventTarget.h"

class nsIFocusController;


#define NS_IWINDOWROOT_IID \
{ 0xc18dee5a, 0xdcf9, 0x4391, \
  { 0xa2, 0x0c, 0x58, 0x1e, 0x76, 0x9d, 0x09, 0x5e } }

class nsPIWindowRoot : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWINDOWROOT_IID)

  NS_IMETHOD GetFocusController(nsIFocusController** aResult)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIWindowRoot, NS_IWINDOWROOT_IID)

#endif 
