




































#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nsXPCOM.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsIMemory.h"

int main(int argc, char **argv)
{
  nsCOMPtr<nsIMemory> mem;
  nsresult rv = NS_GetMemoryManager(getter_AddRefs(mem));

  if (!mem || NS_FAILED(rv))
  {
    printf("Could not get the memory manager\n");
    return -1;
  }

  
  
  
  
  
  
  void *big_alloc = malloc(1024 * 1024 * 16);

  const int highpower = 500000;
  
  char* buffers[highpower];
  for (int i=0; i<highpower; i++)
    buffers[i] = nsnull;
  
  for (int i=0; i<highpower; i++)
  {
    PRBool lowMem = PR_FALSE;
    size_t s = 4096; 
    buffers[i] = (char*) malloc(s);
    
    
    if (!buffers[i])
      printf("Could not allocate a buffer of size %ld\n", s);
    else
    {
      for (int j=0; j<s; j++)
        buffers[i][j] = 'a';
    }
   
    PRIntervalTime start = PR_IntervalNow();
    mem->IsLowMemory(&lowMem);
    PRIntervalTime cost = PR_IntervalNow() - start;
    
    
    printf("Total Allocated: %ld. \tLow Memory now?  %s\t Took (%d)\n",
           s*i,
	   lowMem ? "Yes" : "No",
	   PR_IntervalToMilliseconds(cost));
  
    if (lowMem)
      break;
  }

  for(int i=0; i<highpower; i++)
  {
    if (buffers[i])
      free(buffers[i]);
  }
  return 0;
}
