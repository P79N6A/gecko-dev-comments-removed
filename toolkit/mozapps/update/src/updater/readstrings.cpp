





































#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "readstrings.h"
#include "errors.h"
#include "prtypes.h"

#ifdef XP_WIN
# define NS_tfopen _wfopen
# define OPEN_MODE L"rb"
#else
# define NS_tfopen fopen
# define OPEN_MODE "r"
#endif


class AutoFILE {
public:
  AutoFILE(FILE *fp) : fp_(fp) {}
  ~AutoFILE() { if (fp_) fclose(fp_); }
  operator FILE *() { return fp_; }
private:
  FILE *fp_;
};

static const char kNL[] = "\r\n";
static const char kEquals[] = "=";
static const char kWhitespace[] = " \t";
static const char kRBracket[] = "]";

static const char*
NS_strspnp(const char *delims, const char *str)
{
  const char *d;
  do {
    for (d = delims; *d != '\0'; ++d) {
      if (*str == *d) {
        ++str;
        break;
      }
    }
  } while (*d);

  return str;
}

static char*
NS_strtok(const char *delims, char **str)
{
  if (!*str)
    return NULL;

  char *ret = (char*) NS_strspnp(delims, *str);

  if (!*ret) {
    *str = ret;
    return NULL;
  }

  char *i = ret;
  do {
    for (const char *d = delims; *d != '\0'; ++d) {
      if (*i == *d) {
        *i = '\0';
        *str = ++i;
        return ret;
      }
    }
    ++i;
  } while (*i);

  *str = NULL;
  return ret;
}


int
ReadStrings(const NS_tchar *path, StringTable *results)
{
  AutoFILE fp = NS_tfopen(path, OPEN_MODE);

  if (!fp)
    return READ_ERROR;

  
  if (fseek(fp, 0, SEEK_END) != 0)
    return READ_ERROR;

  long flen = ftell(fp);
  if (flen == 0)
    return READ_ERROR;

  char *fileContents = new char[flen + 1];
  if (!fileContents)
    return MEM_ERROR;

  
  if (fseek(fp, 0, SEEK_SET) != 0)
    return READ_ERROR;

  int rd = fread(fileContents, sizeof(char), flen, fp);
  if (rd != flen)
    return READ_ERROR;

  fileContents[flen] = '\0';

  char *buffer = fileContents;
  PRBool inStringsSection = PR_FALSE;
  const unsigned None = 0, Title = 1, Info = 2, All = 3;
  unsigned read = None;

  while (char *token = NS_strtok(kNL, &buffer)) {
    if (token[0] == '#' || token[0] == ';') 
      continue;

    token = (char*) NS_strspnp(kWhitespace, token);
    if (!*token) 
      continue;

    if (token[0] == '[') { 
      ++token;
      char const * currSection = token;

      char *rb = NS_strtok(kRBracket, &token);
      if (!rb || NS_strtok(kWhitespace, &token)) {
        
        
        
        
        inStringsSection = PR_FALSE;
      }
      else
        inStringsSection = strcmp(currSection, "Strings") == 0;

      continue;
    }

    if (!inStringsSection) {
      
      
      
      continue;
    }

    char *key = token;
    char *e = NS_strtok(kEquals, &token);
    if (!e)
      continue;

    if (strcmp(key, "Title") == 0) {
      strncpy(results->title, token, MAX_TEXT_LEN - 1);
      results->title[MAX_TEXT_LEN - 1] = 0;
      read |= Title;
    }
    else if (strcmp(key, "Info") == 0) {
      strncpy(results->info, token, MAX_TEXT_LEN - 1);
      results->info[MAX_TEXT_LEN - 1] = 0;
      read |= Info;
    }
  }

  return (read == All) ? OK : PARSE_ERROR;
}
