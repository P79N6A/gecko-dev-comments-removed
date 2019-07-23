




































#include "nsVersionComparator.h"

#include <stdio.h>

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <versionA> <versionB>\n", argv[0]);
    return 1;
  }

  PRInt32 i = NS_CompareVersions(argv[1], argv[2]);

  const char *format;

  if (i < 0)
    format = "%s < %s\n";
  else if (i > 0)
    format = "%s > %s\n";
  else
    format = "%s = %s\n";

  fprintf(stdout, format, argv[1], argv[2]);
  return 0;
}

