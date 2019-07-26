



#include <excpt.h>
#include <stdlib.h>

void fail() {
   try {
      int i = 0, j = 1;
      j /= i;
   } catch(...) {
     exit(1);
   }
}

int main() {
   __try {
      fail();
   } __except(EXCEPTION_EXECUTE_HANDLER) {
     return 2;
   }
   return 3;
}
