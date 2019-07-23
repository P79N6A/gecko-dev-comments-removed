




































#ifndef nsWinRegEnums_h__
#define nsWinRegEnums_h__

typedef enum nsWinRegEnum {
  NS_WIN_REG_CREATE          = 1,
  NS_WIN_REG_DELETE          = 2,
  NS_WIN_REG_DELETE_VAL      = 3,
  NS_WIN_REG_SET_VAL_STRING  = 4,
  NS_WIN_REG_SET_VAL_NUMBER  = 5,
  NS_WIN_REG_SET_VAL         = 6

} nsWinRegEnum;


typedef enum nsWinRegValueEnum {
  NS_WIN_REG_SZ                          = 1,
  NS_WIN_REG_EXPAND_SZ                   = 2,
  NS_WIN_REG_BINARY                      = 3,
  NS_WIN_REG_DWORD                       = 4,
  NS_WIN_REG_DWORD_LITTLE_ENDIAN         = 4,
  NS_WIN_REG_DWORD_BIG_ENDIAN            = 5,
  NS_WIN_REG_LINK                        = 6,
  NS_WIN_REG_MULTI_SZ                    = 7,
  NS_WIN_REG_RESOURCE_LIST               = 8,
  NS_WIN_REG_FULL_RESOURCE_DESCRIPTOR    = 9,
  NS_WIN_REG_RESOURCE_REQUIREMENTS_LIST  = 10
} nsWinRegValueEnum;

#endif 
