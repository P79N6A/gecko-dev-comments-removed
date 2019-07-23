





































#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUF 4096
int main(int argc, char *argv[])
{
  DWORD dwError;
  char  szShortPath[MAX_BUF];

  if(argc <= 1)
  {
    printf("\n usage: %s <path>\n", argv[0]);
    exit(1);
  }

  dwError = GetShortPathName(argv[1], szShortPath, sizeof(szShortPath));
  printf("%s", szShortPath);
  return(dwError);
}

