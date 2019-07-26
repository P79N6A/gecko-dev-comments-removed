




#ifndef nsINativeMenuService_h_
#define nsINativeMenuService_h_

#include "nsISupports.h"

class nsIWidget;
class nsIContent;


#define NS_INATIVEMENUSERVICE_IID \
{ 0x90DF88F9, 0xF084, 0x4EF3, \
{ 0x82, 0x9A, 0x49, 0x49, 0x6E, 0x63, 0x6D, 0xED} }

class nsINativeMenuService : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INATIVEMENUSERVICE_IID)
  
  
  
  NS_IMETHOD CreateNativeMenuBar(nsIWidget* aParent, nsIContent* aMenuBarNode)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINativeMenuService, NS_INATIVEMENUSERVICE_IID)

#endif 
