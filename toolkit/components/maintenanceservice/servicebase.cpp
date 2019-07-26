



#include "servicebase.h"
#include "nsWindowsHelpers.h"


#include "nsWindowsRestart.cpp"









BOOL
VerifySameFiles(LPCWSTR file1Path, LPCWSTR file2Path, BOOL &sameContent)
{
  sameContent = FALSE;
  nsAutoHandle file1(CreateFileW(file1Path, GENERIC_READ, FILE_SHARE_READ, 
                                 nullptr, OPEN_EXISTING, 0, nullptr));
  if (INVALID_HANDLE_VALUE == file1) {
    return FALSE;
  }
  nsAutoHandle file2(CreateFileW(file2Path, GENERIC_READ, FILE_SHARE_READ, 
                                 nullptr, OPEN_EXISTING, 0, nullptr));
  if (INVALID_HANDLE_VALUE == file2) {
    return FALSE;
  }

  DWORD fileSize1 = GetFileSize(file1, nullptr);
  DWORD fileSize2 = GetFileSize(file2, nullptr);
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
    if (!ReadFile(file1, buf1, COMPARE_BLOCKSIZE, &readAmount, nullptr) ||
        readAmount != COMPARE_BLOCKSIZE) {
      return FALSE;
    }

    if (!ReadFile(file2, buf2, COMPARE_BLOCKSIZE, &readAmount, nullptr) ||
        readAmount != COMPARE_BLOCKSIZE) {
      return FALSE;
    }

    if (memcmp(buf1, buf2, COMPARE_BLOCKSIZE)) {
      
      return TRUE;
    }
  }

  if (leftOver) {
    if (!ReadFile(file1, buf1, leftOver, &readAmount, nullptr) ||
        readAmount != leftOver) {
      return FALSE;
    }

    if (!ReadFile(file2, buf2, leftOver, &readAmount, nullptr) ||
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
