





































#include <stdlib.h>
#include <stdio.h>
#include "nsIPipe.h"
#include "nsStreamUtils.h"
#include "nsString.h"
#include "nsCOMPtr.h"



static PRBool test_consume_stream() {
  const char kData[] =
      "Get your facts first, and then you can distort them as much as you "
      "please.";

  nsCOMPtr<nsIInputStream> input;
  nsCOMPtr<nsIOutputStream> output;
  NS_NewPipe(getter_AddRefs(input),
             getter_AddRefs(output),
             10, PR_UINT32_MAX);
  if (!input || !output)
    return PR_FALSE;

  PRUint32 n = 0;
  output->Write(kData, sizeof(kData) - 1, &n);
  if (n != (sizeof(kData) - 1))
    return PR_FALSE;
  output = nsnull;  

  nsCString buf;
  if (NS_FAILED(NS_ConsumeStream(input, PR_UINT32_MAX, buf)))
    return PR_FALSE;

  if (!buf.Equals(kData))
    return PR_FALSE;

  return PR_TRUE; 
}



typedef PRBool (*TestFunc)();
#define DECL_TEST(name) { #name, name }

static const struct Test {
  const char* name;
  TestFunc    func;
} tests[] = {
  DECL_TEST(test_consume_stream),
  { nsnull, nsnull }
};

int main(int argc, char **argv) {
  int count = 1;
  if (argc > 1)
    count = atoi(argv[1]);

  if (NS_FAILED(NS_InitXPCOM2(nsnull, nsnull, nsnull)))
    return -1;

  while (count--) {
    for (const Test* t = tests; t->name != nsnull; ++t) {
      printf("%25s : %s\n", t->name, t->func() ? "SUCCESS" : "FAILURE");
    }
  }
  
  NS_ShutdownXPCOM(nsnull);
  return 0;
}
