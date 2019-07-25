




import sys

f = open(sys.argv[1] + ".cc", "w")
f.write("""\
#include <stdio.h>

int main() {
  puts("Hello %s");
  return 0;
}
""" % sys.argv[1])
f.close()
