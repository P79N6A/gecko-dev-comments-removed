



































#ifndef __nsIRegistryUtils_h
#define __nsIRegistryUtils_h

#define NS_REGISTRY_CONTRACTID "@mozilla.org/registry;1"
#define NS_REGISTRY_CLASSNAME "Mozilla Registry"

#define NS_REGISTRY_CID                            \
{                                                  \
  0xbe761f00,                                      \
  0xa3b0,                                          \
  0x11d2,                                          \
  {0x99, 0x6c, 0x00, 0x80, 0xc7, 0xcb, 0x10, 0x81} \
}



#define NS_ERROR_REG_BADTYPE          NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 1 )
#define NS_ERROR_REG_NO_MORE          NS_ERROR_GENERATE_SUCCESS( NS_ERROR_MODULE_REG, 2 )
#define NS_ERROR_REG_NOT_FOUND        NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 3 )
#define NS_ERROR_REG_NOFILE	          NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 4 )
#define NS_ERROR_REG_BUFFER_TOO_SMALL NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 5 )
#define NS_ERROR_REG_NAME_TOO_LONG    NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 6 )
#define NS_ERROR_REG_NO_PATH          NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 7 )
#define NS_ERROR_REG_READ_ONLY        NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 8 )
#define NS_ERROR_REG_BAD_UTF8         NS_ERROR_GENERATE_FAILURE( NS_ERROR_MODULE_REG, 9 )

#endif
