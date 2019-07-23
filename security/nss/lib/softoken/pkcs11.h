












































#ifndef _PKCS11_H_
#define _PKCS11_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

































































































































































































#include "pkcs11t.h"

#define __PASTE(x,y)      x##y



#include "pkcs11p.h"





#define CK_NEED_ARG_LIST  1
#define CK_PKCS11_FUNCTION_INFO(name) \
  CK_DECLARE_FUNCTION(CK_RV, name)



#include "pkcs11f.h"

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO









#define CK_NEED_ARG_LIST  1
#define CK_PKCS11_FUNCTION_INFO(name) \
  typedef CK_DECLARE_FUNCTION_POINTER(CK_RV, __PASTE(CK_,name))



#include "pkcs11f.h"

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO











#define CK_PKCS11_FUNCTION_INFO(name) \
  __PASTE(CK_,name) name;
  
struct CK_FUNCTION_LIST {

  CK_VERSION    version;  




#include "pkcs11f.h" 

};

#undef CK_PKCS11_FUNCTION_INFO


#undef __PASTE


#include "pkcs11u.h"

#ifdef __cplusplus
}
#endif

#endif
