




































#include <string.h>

#include "nsXPCOM.h"
#include "nsINIParser.h"
#include "nsILocalFile.h"

static PRBool
StringCB(const char *aKey, const char *aValue, void* aClosure)
{
  printf("%s=%s\n", aKey, aValue);

  return PR_TRUE;
}

static PRBool
SectionCB(const char *aSection, void* aClosure)
{
  nsINIParser *ini = NS_REINTERPRET_CAST(nsINIParser*, aClosure);

  printf("[%s]\n", aSection);

  ini->GetStrings(aSection, StringCB, nsnull);

  printf("\n");

  return PR_TRUE;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <ini-file>\n", argv[0]);
    return 255;
  }

  nsCOMPtr<nsILocalFile> lf;

  nsresult rv = NS_NewNativeLocalFile(nsDependentCString(argv[1]),
                                      PR_TRUE,
                                      getter_AddRefs(lf));
  if (NS_FAILED(rv)) {
    fprintf(stderr, "Error: NS_NewNativeLocalFile failed\n");
    return 1;
  }

  nsINIParser ini;
  rv = ini.Init(lf);
  if (NS_FAILED(rv)) {
    fprintf(stderr, "Error: Init failed.");
    return 2;
  }

  ini.GetSections(SectionCB, &ini);

  return 0;
}

