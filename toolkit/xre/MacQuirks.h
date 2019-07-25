





#ifndef MacQuirks_h__
#define MacQuirks_h__

#include <sys/types.h>
#include <sys/sysctl.h>
#include "CoreFoundation/CoreFoundation.h"
#include "CoreServices/CoreServices.h"
#include "Carbon/Carbon.h"





struct VersionPart {
  int32_t     numA;

  const char *strB;    
  uint32_t    strBlen;

  int32_t     numC;

  char       *extraD;  
};






static char*
ParseVP(char *part, VersionPart &result)
{
  char *dot;

  result.numA = 0;
  result.strB = NULL;
  result.strBlen = 0;
  result.numC = 0;
  result.extraD = NULL;

  if (!part)
    return part;

  dot = strchr(part, '.');
  if (dot)
    *dot = '\0';

  if (part[0] == '*' && part[1] == '\0') {
    result.numA = INT32_MAX;
    result.strB = "";
  }
  else {
    result.numA = strtol(part, const_cast<char**>(&result.strB), 10);
  }

  if (!*result.strB) {
    result.strB = NULL;
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
    result.extraD = NULL;
      }
    }
  }

  if (dot) {
    ++dot;

    if (!*dot)
      dot = NULL;
  }

  return dot;
}



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



int32_t
NS_CompareVersions(const char *A, const char *B)
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


static void
TriggerQuirks()
{
  int mib[2];

  mib[0] = CTL_KERN;
  mib[1] = KERN_OSRELEASE;
  
  char release[sizeof("10.7.99")];
  size_t len = sizeof(release);
  
  int ret = sysctl(mib, 2, release, &len, NULL, 0);
  
  
  
  if (ret == 0 && NS_CompareVersions(release, "10.8.0") >= 0 && NS_CompareVersions(release, "11") < 0) {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle) {
      CFRetain(mainBundle);

      CFStringRef bundleID = CFBundleGetIdentifier(mainBundle);
      if (bundleID) {
        CFRetain(bundleID);

        CFMutableDictionaryRef dict = (CFMutableDictionaryRef)CFBundleGetInfoDictionary(mainBundle);
        CFDictionarySetValue(dict, CFSTR("CFBundleIdentifier"), CFSTR("org.mozilla.firefox"));

        
        
        
#ifdef __i386__
        ProcessSerialNumber psn;
        ::GetCurrentProcess(&psn);
#else
        SInt32 major;
        ::Gestalt(gestaltSystemVersionMajor, &major);
#endif

        
        dict = (CFMutableDictionaryRef)CFBundleGetInfoDictionary(mainBundle);
        CFDictionarySetValue(dict, CFSTR("CFBundleIdentifier"), bundleID);

        CFRelease(bundleID);
      }
      CFRelease(mainBundle);
    }
  }
}

#endif 
