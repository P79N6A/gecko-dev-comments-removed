





































#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "readstrings.h"
#include "errors.h"


class AutoFILE {
public:
  AutoFILE(FILE *fp) : fp_(fp) {}
  ~AutoFILE() { if (fp_) fclose(fp_); }
  operator FILE *() { return fp_; }
private:
  FILE *fp_;
};


int
ReadStrings(const char *path, StringTable *results)
{
  AutoFILE fp = fopen(path, "r");
  if (!fp)
    return READ_ERROR;

  
  unsigned read = 0;
  char *strings[] = {
    results->title, results->info, NULL
  };
  for (char **p = strings; *p; ++p) {
    while (fgets(*p, MAX_TEXT_LEN, fp)) {
      int len = strlen(*p);
      if (len)
        (*p)[len - 1] = '\0';

      char *eq = strchr(*p, '=');
      if (!eq) 
        continue;
      memmove(*p, eq + 1, len - (eq - *p + 1));
      ++read;
      break;
    }
  }

  return (read == (sizeof(strings) / sizeof(*strings)) - 1) ? OK : READ_ERROR;
}
