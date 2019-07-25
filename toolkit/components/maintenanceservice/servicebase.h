




































#include <windows.h>
#include "updatelogging.h"

BOOL PathAppendSafe(LPWSTR base, LPCWSTR extra);
BOOL VerifySameFiles(LPCWSTR file1Path, LPCWSTR file2Path, BOOL &sameContent);




#define COMPARE_BLOCKSIZE 32768
