



#include <cstdio>
#include <unistd.h>
#include "Zip.h"

extern "C" void report_mapping() { }







const char *test_entries[] = {
  "baz", "foo", "bar", "qux"
};















const char *no_central_dir_entries[] = {
  "a", "b", "c", "d"
};

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "TEST-FAIL | TestZip | Expecting the directory containing test Zips\n");
    return 1;
  }
  chdir(argv[1]);
  Zip::Stream s;
  Zip z("test.zip");
  for (size_t i = 0; i < sizeof(test_entries) / sizeof(*test_entries); i++) {
    if (!z.GetStream(test_entries[i], &s)) {
      fprintf(stderr, "TEST-UNEXPECTED-FAIL | TestZip | test.zip: Couldn't get entry \"%s\"\n", test_entries[i]);
      return 1;
    }
  }
  fprintf(stderr, "TEST-PASS | TestZip | test.zip could be accessed fully\n");

  Zip z2("no_central_dir.zip");
  for (size_t i = 0; i < sizeof(no_central_dir_entries)
                         / sizeof(*no_central_dir_entries); i++) {
    if (!z2.GetStream(no_central_dir_entries[i], &s)) {
      fprintf(stderr, "TEST-UNEXPECTED-FAIL | TestZip | no_central_dir.zip: Couldn't get entry \"%s\"\n", no_central_dir_entries[i]);
      return 1;
    }
  }
  fprintf(stderr, "TEST-PASS | TestZip | no_central_dir.zip could be accessed in order\n");

  return 0;
}
