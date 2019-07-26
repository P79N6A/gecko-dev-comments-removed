









#include <stdio.h>

#include "pkix.h"
#include "pkixt.h"
#include "pkix_error.h"

#undef PKIX_ERRORENTRY
#define PKIX_ERRORENTRY(name,desc,plerr) #name

const char * const PKIX_ErrorNames[] =
{
#include "pkix_errorstrings.h"
};

#undef PKIX_ERRORENTRY


int
main(int argc, char **argv)
{
  int i = 0;
  for (; i < PKIX_NUMERRORCODES; ++i) {
    printf("code %d %s\n", i, PKIX_ErrorNames[i]);
  }
  return 0;
}
