




































#ifndef nsInstallFileOpEnums_h__
#define nsInstallFileOpEnums_h__

typedef enum nsInstallFileOpEnums {
  NS_FOP_DIR_CREATE          = 0,
  NS_FOP_DIR_REMOVE          = 1,
  NS_FOP_DIR_RENAME          = 2,
  NS_FOP_FILE_COPY           = 3,
  NS_FOP_FILE_DELETE         = 4,
  NS_FOP_FILE_EXECUTE        = 5,
  NS_FOP_FILE_MOVE           = 6,
  NS_FOP_FILE_RENAME         = 7,
  NS_FOP_WIN_SHORTCUT        = 8,
  NS_FOP_MAC_ALIAS           = 9,
  NS_FOP_UNIX_LINK           = 10,
  NS_FOP_FILE_SET_STAT       = 11,
  NS_FOP_WIN_REGISTER_SERVER = 12

} nsInstallFileOpEnums;

#endif 
