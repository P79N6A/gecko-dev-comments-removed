




#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

#include "nsTArray.h"

namespace JS {
class Value;
} 
namespace mozilla {
namespace dom {
class Function;
} 
} 

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ 0x53c8e80e, 0xcc78, 0x48bc, \
 { 0xba, 0x63, 0x0c, 0xb9, 0xdb, 0xf7, 0x06, 0x34 } }






class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  
  
  virtual mozilla::dom::Function *GetCallback() = 0;

  
  virtual const PRUnichar *GetHandlerText() = 0;

  
  
  
  virtual void GetLocation(const char **aFileName, uint32_t *aLineNo) = 0;

  
  virtual const nsTArray<JS::Value>& GetArgs() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
