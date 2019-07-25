




































#include <windows.h>
#include "updatelogging.h"

BOOL PathAppendSafe(LPWSTR base, LPCWSTR extra);
BOOL VerifySameFiles(LPCWSTR file1Path, LPCWSTR file2Path, BOOL &sameContent);




#define COMPARE_BLOCKSIZE 32768





#define UPDATER_IDENTITY_STRING \
  "moz-updater.exe-4cdccec4-5ee0-4a06-9817-4cd899a9db49"
#define IDS_UPDATER_IDENTITY 1006
