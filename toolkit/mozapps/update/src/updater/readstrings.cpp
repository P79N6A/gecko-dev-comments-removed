





































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

  
  if (!fgets(results->title, MAX_TEXT_LEN, fp))
    return READ_ERROR;
  if (!fgets(results->title, MAX_TEXT_LEN, fp))
    return READ_ERROR;

  
  if (!fgets(results->title, MAX_TEXT_LEN, fp))
    return READ_ERROR;
  if (!fgets(results->info, MAX_TEXT_LEN, fp))
    return READ_ERROR;

  
  char *strings[] = {
    results->title, results->info, NULL
  };
  for (char **p = strings; *p; ++p) {
    int len = strlen(*p);
    if (len)
      (*p)[len - 1] = '\0';

    char *eq = strchr(*p, '=');
    if (!eq)
      return PARSE_ERROR;
    memmove(*p, eq + 1, len - (eq - *p + 1));
  }

  return OK;
}
