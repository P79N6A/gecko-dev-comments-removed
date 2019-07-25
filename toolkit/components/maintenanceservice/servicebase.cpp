




































#include "servicebase.h"
#include "nsWindowsHelpers.h"


#include "nsWindowsRestart.cpp"









BOOL
VerifySameFiles(LPCWSTR file1Path, LPCWSTR file2Path, BOOL &sameContent)
{
  sameContent = FALSE;
  nsAutoHandle file1(CreateFileW(file1Path, GENERIC_READ, FILE_SHARE_READ, 
                                 NULL, OPEN_EXISTING, 0, NULL));
  if (!file1) {
    return FALSE;
  }
  nsAutoHandle file2(CreateFileW(file2Path, GENERIC_READ, FILE_SHARE_READ, 
                                 NULL, OPEN_EXISTING, 0, NULL));
  if (!file2) {
    return FALSE;
  }

  DWORD fileSize1 = GetFileSize(file1, NULL);
  DWORD fileSize2 = GetFileSize(file2, NULL);
  if (INVALID_FILE_SIZE == fileSize1 || INVALID_FILE_SIZE == fileSize2) {
    return FALSE;
  }

  if (fileSize1 != fileSize2) {
    
    return TRUE;
  }

  char buf1[COMPARE_BLOCKSIZE];
  char buf2[COMPARE_BLOCKSIZE];
  DWORD numBlocks = fileSize1 / COMPARE_BLOCKSIZE;
  DWORD leftOver = fileSize1 % COMPARE_BLOCKSIZE;
  DWORD readAmount;
  for (DWORD i = 0; i < numBlocks; i++) {
    if (!ReadFile(file1, buf1, COMPARE_BLOCKSIZE, &readAmount, NULL) || 
        readAmount != COMPARE_BLOCKSIZE) {
      return FALSE;
    }

    if (!ReadFile(file2, buf2, COMPARE_BLOCKSIZE, &readAmount, NULL) || 
        readAmount != COMPARE_BLOCKSIZE) {
      return FALSE;
    }

    if (memcmp(buf1, buf2, COMPARE_BLOCKSIZE)) {
      
      return TRUE;
    }
  }

  if (leftOver) {
    if (!ReadFile(file1, buf1, leftOver, &readAmount, NULL) || 
        readAmount != leftOver) {
      return FALSE;
    }

    if (!ReadFile(file2, buf2, leftOver, &readAmount, NULL) || 
        readAmount != leftOver) {
      return FALSE;
    }

    if (memcmp(buf1, buf2, leftOver)) {
      
      return TRUE;
    }
  }

  sameContent = TRUE;
  return TRUE;
}
