#include "toolspath.h"

int 
main(int argc, char **argv)
{  
  char* args[1000];
  int i = 0;
  
  args[i++] = ASM_PATH;
  args[i++] = "-I\"" WCE_INC "\""; 
  args[i++] = "-CPU ARM1136";  

  i += argpath_conv(&argv[1], &args[i]);

  return run(args);
}
