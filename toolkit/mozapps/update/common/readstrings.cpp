





#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "readstrings.h"
#include "errors.h"

#ifdef XP_WIN
# define NS_tfopen _wfopen
# define OPEN_MODE L"rb"
#else
# define NS_tfopen fopen
# define OPEN_MODE "r"
#endif


class AutoFILE {
public:
  explicit AutoFILE(FILE *fp) : fp_(fp) {}
  ~AutoFILE() { if (fp_) fclose(fp_); }
  operator FILE *() { return fp_; }
private:
  FILE *fp_;
};

class AutoCharArray {
public:
  explicit AutoCharArray(size_t len) { ptr_ = new char[len]; }
  ~AutoCharArray() { delete[] ptr_; }
  operator char *() { return ptr_; }
private:
  char *ptr_;
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
    return nullptr;

  char *ret = (char*) NS_strspnp(delims, *str);

  if (!*ret) {
    *str = ret;
    return nullptr;
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

  *str = nullptr;
  return ret;
}





static int
find_key(const char *keyList, char* key)
{
  if (!keyList)
    return -1;

  int index = 0;
  const char *p = keyList;
  while (*p)
  {
    if (strcmp(key, p) == 0)
      return index;

    p += strlen(p) + 1;
    index++;
  }

  
  return -1;
}











int
ReadStrings(const NS_tchar *path,
            const char *keyList,
            unsigned int numStrings,
            char results[][MAX_TEXT_LEN],
            const char *section)
{
  AutoFILE fp(NS_tfopen(path, OPEN_MODE));

  if (!fp)
    return READ_ERROR;

  
  if (fseek(fp, 0, SEEK_END) != 0)
    return READ_ERROR;

  long len = ftell(fp);
  if (len <= 0)
    return READ_ERROR;

  size_t flen = size_t(len);
  AutoCharArray fileContents(flen + 1);
  if (!fileContents)
    return READ_STRINGS_MEM_ERROR;

  
  if (fseek(fp, 0, SEEK_SET) != 0)
    return READ_ERROR;

  size_t rd = fread(fileContents, sizeof(char), flen, fp);
  if (rd != flen)
    return READ_ERROR;

  fileContents[flen] = '\0';

  char *buffer = fileContents;
  bool inStringsSection = false;

  unsigned int read = 0;

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
        
        
        
        
        inStringsSection = false;
      }
      else {
        if (section)
          inStringsSection = strcmp(currSection, section) == 0;
        else
          inStringsSection = strcmp(currSection, "Strings") == 0;
      }

      continue;
    }

    if (!inStringsSection) {
      
      
      
      continue;
    }

    char *key = token;
    char *e = NS_strtok(kEquals, &token);
    if (!e)
      continue;

    int keyIndex = find_key(keyList, key);
    if (keyIndex >= 0 && (unsigned int)keyIndex < numStrings)
    {
      strncpy(results[keyIndex], token, MAX_TEXT_LEN - 1);
      results[keyIndex][MAX_TEXT_LEN - 1] = '\0';
      read++;
    }
  }

  return (read == numStrings) ? OK : PARSE_ERROR;
}



int
ReadStrings(const NS_tchar *path, StringTable *results)
{
  const unsigned int kNumStrings = 2;
  const char *kUpdaterKeys = "Title\0Info\0";
  char updater_strings[kNumStrings][MAX_TEXT_LEN];

  int result = ReadStrings(path, kUpdaterKeys, kNumStrings, updater_strings);

  strncpy(results->title, updater_strings[0], MAX_TEXT_LEN - 1);
  results->title[MAX_TEXT_LEN - 1] = '\0';
  strncpy(results->info, updater_strings[1], MAX_TEXT_LEN - 1);
  results->info[MAX_TEXT_LEN - 1] = '\0';

  return result;
}
