



#include <malloc.h>
#include <string.h>

int main() {
  char* stuff = reinterpret_cast<char*>(_alloca(256));
  strcpy(stuff, "blah");
  return 0;
}
