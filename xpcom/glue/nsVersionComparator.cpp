



#include "nsVersionComparator.h"
#include <algorithm>


struct VersionPart {
  int32_t     numA;

  const char *strB;    
  uint32_t    strBlen;

  int32_t     numC;

  char       *extraD;  
};

#ifdef XP_WIN
struct VersionPartW {
  int32_t     numA;

  const PRUnichar *strB;    
  uint32_t    strBlen;

  int32_t     numC;

  PRUnichar       *extraD;  

};
#endif






static char*
ParseVP(char *part, VersionPart &result)
{
  char *dot;

  result.numA = 0;
  result.strB = nullptr;
  result.strBlen = 0;
  result.numC = 0;
  result.extraD = nullptr;

  if (!part)
    return part;

  dot = strchr(part, '.');
  if (dot)
    *dot = '\0';

  if (part[0] == '*' && part[1] == '\0') {
    result.numA = PR_INT32_MAX;
    result.strB = "";
  }
  else {
    result.numA = strtol(part, const_cast<char**>(&result.strB), 10);
  }

  if (!*result.strB) {
    result.strB = nullptr;
    result.strBlen = 0;
  }
  else {
    if (result.strB[0] == '+') {
      static const char kPre[] = "pre";

      ++result.numA;
      result.strB = kPre;
      result.strBlen = sizeof(kPre) - 1;
    }
    else {
      const char *numstart = strpbrk(result.strB, "0123456789+-");
      if (!numstart) {
	result.strBlen = strlen(result.strB);
      }
      else {
	result.strBlen = numstart - result.strB;

	result.numC = strtol(numstart, &result.extraD, 10);
	if (!*result.extraD)
	  result.extraD = nullptr;
      }
    }
  }

  if (dot) {
    ++dot;

    if (!*dot)
      dot = nullptr;
  }

  return dot;
}







#ifdef XP_WIN
static PRUnichar*
ParseVP(PRUnichar *part, VersionPartW &result)
{

  PRUnichar *dot;

  result.numA = 0;
  result.strB = nullptr;
  result.strBlen = 0;
  result.numC = 0;
  result.extraD = nullptr;

  if (!part)
    return part;

  dot = wcschr(part, '.');
  if (dot)
    *dot = '\0';

  if (part[0] == '*' && part[1] == '\0') {
    result.numA = PR_INT32_MAX;
    result.strB = L"";
  }
  else {
    result.numA = wcstol(part, const_cast<PRUnichar**>(&result.strB), 10);
  }

  if (!*result.strB) {
    result.strB = nullptr;
    result.strBlen = 0;
  }
  else {
    if (result.strB[0] == '+') {
      static const PRUnichar kPre[] = L"pre";

      ++result.numA;
      result.strB = kPre;
      result.strBlen = sizeof(kPre) - 1;
    }
    else {
      const PRUnichar *numstart = wcspbrk(result.strB, L"0123456789+-");
      if (!numstart) {
	result.strBlen = wcslen(result.strB);
      }
      else {
	result.strBlen = numstart - result.strB;

	result.numC = wcstol(numstart, &result.extraD, 10);
	if (!*result.extraD)
	  result.extraD = nullptr;
      }
    }
  }

  if (dot) {
    ++dot;

    if (!*dot)
      dot = nullptr;
  }

  return dot;
}
#endif


static int32_t
ns_strcmp(const char *str1, const char *str2)
{
  
  if (!str1)
    return str2 != 0;

  if (!str2)
    return -1;

  return strcmp(str1, str2);
}


static int32_t
ns_strnncmp(const char *str1, uint32_t len1, const char *str2, uint32_t len2)
{
  
  if (!str1)
    return str2 != 0;

  if (!str2)
    return -1;

  for (; len1 && len2; --len1, --len2, ++str1, ++str2) {
    if (*str1 < *str2)
      return -1;

    if (*str1 > *str2)
      return 1;
  }

  if (len1 == 0)
    return len2 == 0 ? 0 : -1;

  return 1;
}


static int32_t
ns_cmp(int32_t n1, int32_t n2)
{
  if (n1 < n2)
    return -1;

  return n1 != n2;
}




static int32_t
CompareVP(VersionPart &v1, VersionPart &v2)
{
  int32_t r = ns_cmp(v1.numA, v2.numA);
  if (r)
    return r;

  r = ns_strnncmp(v1.strB, v1.strBlen, v2.strB, v2.strBlen);
  if (r)
    return r;

  r = ns_cmp(v1.numC, v2.numC);
  if (r)
    return r;

  return ns_strcmp(v1.extraD, v2.extraD);
}




#ifdef XP_WIN
static int32_t
CompareVP(VersionPartW &v1, VersionPartW &v2)
{
  int32_t r = ns_cmp(v1.numA, v2.numA);
  if (r)
    return r;

  r = wcsncmp(v1.strB, v2.strB, std::min(v1.strBlen,v2.strBlen));
  if (r)
    return r;

  r = ns_cmp(v1.numC, v2.numC);
  if (r)
    return r;

  if (!v1.extraD)
    return v2.extraD != 0;

  if (!v2.extraD)
    return -1;

  return wcscmp(v1.extraD, v2.extraD);
}
#endif

namespace mozilla {

#ifdef XP_WIN
int32_t
CompareVersions(const PRUnichar *A, const PRUnichar *B)
{
  PRUnichar *A2 = wcsdup(A);
  if (!A2)
    return 1;

  PRUnichar *B2 = wcsdup(B);
  if (!B2) {
    free(A2);
    return 1;
  }

  int32_t result;
  PRUnichar *a = A2, *b = B2;

  do {
    VersionPartW va, vb;

    a = ParseVP(a, va);
    b = ParseVP(b, vb);

    result = CompareVP(va, vb);
    if (result)
      break;

  } while (a || b);

  free(A2);
  free(B2);

  return result;
}
#endif

int32_t
CompareVersions(const char *A, const char *B)
{
  char *A2 = strdup(A);
  if (!A2)
    return 1;

  char *B2 = strdup(B);
  if (!B2) {
    free(A2);
    return 1;
  }

  int32_t result;
  char *a = A2, *b = B2;

  do {
    VersionPart va, vb;

    a = ParseVP(a, va);
    b = ParseVP(b, vb);

    result = CompareVP(va, vb);
    if (result)
      break;

  } while (a || b);

  free(A2);
  free(B2);

  return result;
}

} 

