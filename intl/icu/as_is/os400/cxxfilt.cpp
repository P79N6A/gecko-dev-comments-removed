

#include <stdio.h>
#include <demangle.h>

void showSym(char *str) {
  char *rest;
  struct Name *name = Demangle(str, rest); 

  printf("# '%s'\n", str);
  if(*rest) printf("\trest: '%s'\n", rest);
  if(name->Kind() == MemberFunction) {
    
    
    
    printf("\t=> %s\n", ((MemberFunctionName *) name)->Text());
  } else {
    printf("\t(not MemberFunction)\n");
  }
}





int main(int argc,  char *argv[]) {
  if(argc>1) {
    for(int i=1;i<argc;i++) {
       showSym(argv[i]);
    }
  } else {
    printf("Usage: %s <symbol> ...\n", argv[0]);
  }



}
