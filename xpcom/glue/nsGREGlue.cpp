






































#include "nsXPCOMGlue.h"

#include "nsINIParser.h"
#include "nsVersionComparator.h"
#include "nsXPCOMPrivate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_WIN32
# include <windows.h>
# include <mbstring.h>
# include <io.h>
# define snprintf _snprintf
# define R_OK 04
#elif defined(XP_OS2)
# define INCL_DOS
# include <os2.h>
#elif defined(XP_MACOSX)
# include <CFBundle.h>
# include <unistd.h>
# include <dirent.h>
#elif defined(XP_UNIX)
# include <unistd.h>
# include <sys/param.h>
# include <dirent.h>
#elif defined(XP_BEOS)
# include <FindDirectory.h>
# include <Path.h>
# include <unistd.h>
# include <sys/param.h>
# include <OS.h>
# include <image.h>
# include <dirent.h>
# include <FindDirectory.h>
#endif

#include <sys/stat.h>







static PRBool safe_strncat(char *dest, const char *append, PRUint32 count)
{
  char *end = dest + count - 1;

  
  while (*dest)
    ++dest;

  while (*append && dest < end) {
    *dest = *append;
    ++dest, ++append;
  }

  *dest = '\0';

  return *append == '\0';
}

static PRBool
CheckVersion(const char* toCheck,
             const GREVersionRange *versions,
             PRUint32 versionsLength);

#if defined(XP_MACOSX)

static PRBool
GRE_FindGREFramework(const char* rootPath,
                     const GREVersionRange *versions,
                     PRUint32 versionsLength,
                     const GREProperty *properties,
                     PRUint32 propertiesLength,
                     char* buffer, PRUint32 buflen);

#elif defined(XP_UNIX) || defined(XP_BEOS)

static PRBool
GRE_GetPathFromConfigDir(const char* dirname,
                         const GREVersionRange *versions,
                         PRUint32 versionsLength,
                         const GREProperty *properties,
                         PRUint32 propertiesLength,
                         char* buffer, PRUint32 buflen);

static PRBool
GRE_GetPathFromConfigFile(const char* filename,
                          const GREVersionRange *versions,
                          PRUint32 versionsLength,
                          const GREProperty *properties,
                          PRUint32 propertiesLength,
                          char* buffer, PRUint32 buflen);

#elif defined(XP_WIN)

static PRBool
GRE_GetPathFromRegKey(HKEY aRegKey,
                      const GREVersionRange *versions,
                      PRUint32 versionsLength,
                      const GREProperty *properties,
                      PRUint32 propertiesLength,
                      char* buffer, PRUint32 buflen);

#endif

nsresult
GRE_GetGREPathWithProperties(const GREVersionRange *versions,
                             PRUint32 versionsLength,
                             const GREProperty *properties,
                             PRUint32 propertiesLength,
                             char *aBuffer, PRUint32 aBufLen)
{
  
  const char* env = getenv("GRE_HOME");
  if (env && *env) {
    char p[MAXPATHLEN];
    snprintf(p, sizeof(p), "%s" XPCOM_FILE_PATH_SEPARATOR XPCOM_DLL, env);
    p[sizeof(p) - 1] = '\0';

#if XP_UNIX
    if (realpath(p, aBuffer))
      return NS_OK;
#elif XP_WIN
    if (_fullpath(aBuffer, p, aBufLen))
      return NS_OK;
#elif XP_OS2
    
    if (realpath(p, aBuffer)) {
      for (char* ptr = strchr(aBuffer, '/'); ptr; ptr = strchr(ptr, '/'))
        *ptr = '\\';
      return NS_OK;
    }
#elif XP_BEOS
    BPath path;
    status_t result;
    result = path.SetTo(p,0,true);
    if (result == B_OK)
    {
      sprintf(aBuffer, path.Path());
      return NS_OK;
    }
#else
    
    
#endif

    if (strlen(p) >= aBufLen)
      return NS_ERROR_FILE_NAME_TOO_LONG;

    strcpy(aBuffer, p);

    return NS_OK;
  }

  
  env = getenv("USE_LOCAL_GRE");
  if (env && *env) {
    *aBuffer = nsnull;
    return NS_OK;
  }

#ifdef XP_MACOSX
  aBuffer[0] = '\0';

  
  CFBundleRef appBundle = CFBundleGetMainBundle();
  if (appBundle) {
    CFURLRef fwurl = CFBundleCopyPrivateFrameworksURL(appBundle);
    CFURLRef absfwurl = nsnull;
    if (fwurl) {
      absfwurl = CFURLCopyAbsoluteURL(fwurl);
      CFRelease(fwurl);
    }

    if (absfwurl) {
      CFURLRef xulurl =
        CFURLCreateCopyAppendingPathComponent(NULL, absfwurl,
                                              CFSTR(GRE_FRAMEWORK_NAME),
                                              PR_TRUE);

      if (xulurl) {
        CFURLRef xpcomurl =
          CFURLCreateCopyAppendingPathComponent(NULL, xulurl,
                                                CFSTR("libxpcom.dylib"),
                                                PR_FALSE);

        if (xpcomurl) {
          char tbuffer[MAXPATHLEN];

          if (CFURLGetFileSystemRepresentation(xpcomurl, PR_TRUE,
                                               (UInt8*) tbuffer,
                                               sizeof(tbuffer)) &&
              access(tbuffer, R_OK | X_OK) == 0) {
            if (!realpath(tbuffer, aBuffer)) {
              aBuffer[0] = '\0';
            }
          }

          CFRelease(xpcomurl);
        }

        CFRelease(xulurl);
      }

      CFRelease(absfwurl);
    }
  }

  if (aBuffer[0])
    return NS_OK;

  
  const char *home = getenv("HOME");
  if (home && *home && GRE_FindGREFramework(home,
                                            versions, versionsLength,
                                            properties, propertiesLength,
                                            aBuffer, aBufLen)) {
    return NS_OK;
  }

  
  if (GRE_FindGREFramework("",
                           versions, versionsLength,
                           properties, propertiesLength,
                           aBuffer, aBufLen)) {
    return NS_OK;
  }

#elif defined(XP_UNIX) 
  env = getenv("MOZ_GRE_CONF");
  if (env && GRE_GetPathFromConfigFile(env,
                                       versions, versionsLength,
                                       properties, propertiesLength,
                                       aBuffer, aBufLen)) {
    return NS_OK;
  }

  env = getenv("HOME");
  if (env && *env) {
    char buffer[MAXPATHLEN];

    

    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_CONF_NAME, env);
    
    if (GRE_GetPathFromConfigFile(buffer,
                                  versions, versionsLength,
                                  properties, propertiesLength,
                                  aBuffer, aBufLen)) {
      return NS_OK;
    }

    

    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_USER_CONF_DIR, env);

    if (GRE_GetPathFromConfigDir(buffer,
                                 versions, versionsLength,
                                 properties, propertiesLength,
                                 aBuffer, aBufLen)) {
      return NS_OK;
    }
  }

  
  if (GRE_GetPathFromConfigFile(GRE_CONF_PATH,
                                versions, versionsLength,
                                properties, propertiesLength,
                                aBuffer, aBufLen)) {
    return NS_OK;
  }

  
  if (GRE_GetPathFromConfigDir(GRE_CONF_DIR,
                               versions, versionsLength,
                               properties, propertiesLength,
                               aBuffer, aBufLen)) {
    return NS_OK;
  }

#elif defined(XP_BEOS)
  env = getenv("MOZ_GRE_CONF");
  if (env && GRE_GetPathFromConfigFile(env,
                                       versions, versionsLength,
                                       properties, propertiesLength,
                                       aBuffer, aBufLen)) {
    return NS_OK;
  }

  char p[MAXPATHLEN]; 
  if (find_directory(B_USER_SETTINGS_DIRECTORY, 0, 0, p, MAXPATHLEN)) {
    char buffer[MAXPATHLEN];

    
    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_CONF_NAME, p);
    
    if (GRE_GetPathFromConfigFile(buffer,
                                  versions, versionsLength,
                                  properties, propertiesLength,
                                  aBuffer, aBufLen)) {
      return NS_OK;
    }

    
    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_CONF_DIR, p);

    if (GRE_GetPathFromConfigDir(buffer,
                                 versions, versionsLength,
                                 properties, propertiesLength,
                                 aBuffer, aBufLen)) {
      return NS_OK;
    }
  }
  
  
  
  if (find_directory(B_COMMON_SETTINGS_DIRECTORY, 0, 0, p, MAXPATHLEN)) {
    char buffer[MAXPATHLEN];
    
    
    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_CONF_PATH, p);
    if (GRE_GetPathFromConfigFile(buffer,
                                  versions, versionsLength,
                                  properties, propertiesLength,
                                  aBuffer, aBufLen)) {
      return NS_OK;
    }

    
    snprintf(buffer, sizeof(buffer),
             "%s" XPCOM_FILE_PATH_SEPARATOR GRE_CONF_DIR, p);
    if (GRE_GetPathFromConfigDir(buffer,
                                 versions, versionsLength,
                                 properties, propertiesLength,
                                 aBuffer, aBufLen)) {
      return NS_OK;
    }
  }

#elif defined(XP_WIN)
  HKEY hRegKey = NULL;
    
  
  
  
  
  
  
  
  
  
  
  
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, GRE_WIN_REG_LOC, 0,
                     KEY_READ, &hRegKey) == ERROR_SUCCESS) {
    PRBool ok = GRE_GetPathFromRegKey(hRegKey,
                                      versions, versionsLength,
                                      properties, propertiesLength,
                                      aBuffer, aBufLen);
    ::RegCloseKey(hRegKey);

    if (ok)
      return NS_OK;
  }

  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, GRE_WIN_REG_LOC, 0,
                     KEY_ENUMERATE_SUB_KEYS, &hRegKey) == ERROR_SUCCESS) {
    PRBool ok = GRE_GetPathFromRegKey(hRegKey,
                                      versions, versionsLength,
                                      properties, propertiesLength,
                                      aBuffer, aBufLen);
    ::RegCloseKey(hRegKey);

    if (ok)
      return NS_OK;
  }
#endif

  return NS_ERROR_FAILURE;
}

static PRBool
CheckVersion(const char* toCheck,
             const GREVersionRange *versions,
             PRUint32 versionsLength)
{
  
  for (const GREVersionRange *versionsEnd = versions + versionsLength;
       versions < versionsEnd;
       ++versions) {
    PRInt32 c = NS_CompareVersions(toCheck, versions->lower);
    if (c < 0)
      continue;

    if (!c && !versions->lowerInclusive)
      continue;

    c = NS_CompareVersions(toCheck, versions->upper);
    if (c > 0)
      continue;

    if (!c && !versions->upperInclusive)
      continue;

    return PR_TRUE;
  }

  return PR_FALSE;
}

#ifdef XP_MACOSX
PRBool
GRE_FindGREFramework(const char* rootPath,
                     const GREVersionRange *versions,
                     PRUint32 versionsLength,
                     const GREProperty *properties,
                     PRUint32 propertiesLength,
                     char* buffer, PRUint32 buflen)
{
  PRBool found = PR_FALSE;

  snprintf(buffer, buflen,
           "%s/Library/Frameworks/" GRE_FRAMEWORK_NAME "/Versions", rootPath);
  DIR *dir = opendir(buffer);
  if (dir) {
    struct dirent *entry;
    while (!found && (entry = readdir(dir))) {
      if (CheckVersion(entry->d_name, versions, versionsLength)) {
        snprintf(buffer, buflen,
                 "%s/Library/Frameworks/" GRE_FRAMEWORK_NAME
                 "/Versions/%s/" XPCOM_DLL, rootPath, entry->d_name);
        if (access(buffer, R_OK | X_OK) == 0)
          found = PR_TRUE;
      }
    }

    closedir(dir);
  }
  
  if (found)
    return PR_TRUE;

  buffer[0] = '\0';
  return PR_FALSE;
}
    
#elif defined(XP_UNIX) || defined(XP_BEOS)

static PRBool IsConfFile(const char *filename)
{
  const char *dot = strrchr(filename, '.');

  return (dot && strcmp(dot, ".conf") == 0);
}

PRBool
GRE_GetPathFromConfigDir(const char* dirname,
                         const GREVersionRange *versions,
                         PRUint32 versionsLength,
                         const GREProperty *properties,
                         PRUint32 propertiesLength,
                         char* buffer, PRUint32 buflen)
{
  
  
  
  DIR *dir = opendir(dirname);
  if (!dir)
    return nsnull;

  PRBool found = PR_FALSE;
  struct dirent *entry;

  while (!found && (entry = readdir(dir))) {

    
    
    if (!IsConfFile(entry->d_name))
      continue;

    char fullPath[MAXPATHLEN];
    snprintf(fullPath, sizeof(fullPath), "%s" XPCOM_FILE_PATH_SEPARATOR "%s",
             dirname, entry->d_name);

    found = GRE_GetPathFromConfigFile(fullPath,
                                      versions, versionsLength,
                                      properties, propertiesLength,
                                      buffer, buflen);
  }

  closedir(dir);

  return found;
}

#define READ_BUFFER_SIZE 1024

struct INIClosure
{
  nsINIParser           *parser;
  const GREVersionRange *versions;
  PRUint32               versionsLength;
  const GREProperty     *properties;
  PRUint32               propertiesLength;
  char                  *pathBuffer;
  PRUint32               buflen;
  PRBool                 found;
};

static PRBool
CheckINIHeader(const char *aHeader, void *aClosure)
{
  nsresult rv;

  INIClosure *c = NS_REINTERPRET_CAST(INIClosure *, aClosure);

  if (!CheckVersion(aHeader, c->versions, c->versionsLength))
    return PR_TRUE;

  const GREProperty *properties = c->properties;
  const GREProperty *endProperties = properties + c->propertiesLength;
  for (; properties < endProperties; ++properties) {
    char buffer[MAXPATHLEN];
    rv = c->parser->GetString(aHeader, properties->property,
                             buffer, sizeof(buffer));
    if (NS_FAILED(rv))
      return PR_TRUE;

    if (strcmp(buffer, properties->value))
      return PR_TRUE;
  }

  rv = c->parser->GetString(aHeader, "GRE_PATH", c->pathBuffer, c->buflen);
  if (NS_FAILED(rv))
    return PR_TRUE;

  if (!safe_strncat(c->pathBuffer, "/" XPCOM_DLL, c->buflen) ||
      access(c->pathBuffer, R_OK))
    return PR_TRUE;

  
  c->found = PR_TRUE;
  return PR_FALSE;
}

PRBool
GRE_GetPathFromConfigFile(const char* filename,
                          const GREVersionRange *versions,
                          PRUint32 versionsLength,
                          const GREProperty *properties,
                          PRUint32 propertiesLength,
                          char* pathBuffer, PRUint32 buflen)
{
  nsINIParser parser;
  nsresult rv = parser.Init(filename);
  if (NS_FAILED(rv))
    return PR_FALSE;

  INIClosure c = {
    &parser,
    versions, versionsLength,
    properties, propertiesLength,
    pathBuffer, buflen,
    PR_FALSE
  };

  parser.GetSections(CheckINIHeader, &c);
  return c.found;
}

#elif defined(XP_WIN)

static PRBool
CopyWithEnvExpansion(char* aDest, const char* aSource, PRUint32 aBufLen,
                     DWORD aType)
{
  switch (aType) {
  case REG_SZ:
    if (strlen(aSource) >= aBufLen)
      return PR_FALSE;

    strcpy(aDest, aSource);
    return PR_TRUE;

  case REG_EXPAND_SZ:
    if (ExpandEnvironmentStrings(aSource, aDest, aBufLen) > aBufLen)
      return PR_FALSE;

    return PR_TRUE;
  };

  

  return PR_FALSE;
}

PRBool
GRE_GetPathFromRegKey(HKEY aRegKey,
                      const GREVersionRange *versions,
                      PRUint32 versionsLength,
                      const GREProperty *properties,
                      PRUint32 propertiesLength,
                      char* aBuffer, PRUint32 aBufLen)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  DWORD i = 0;

  while (PR_TRUE) {
    char name[MAXPATHLEN + 1];
    DWORD nameLen = MAXPATHLEN;
    if (::RegEnumKeyEx(aRegKey, i, name, &nameLen, NULL, NULL, NULL, NULL) !=
        ERROR_SUCCESS) {
      break;
    }

    HKEY subKey = NULL;
    if (::RegOpenKeyEx(aRegKey, name, 0, KEY_QUERY_VALUE, &subKey) !=
        ERROR_SUCCESS) {
      continue;
    }

    char version[40];
    DWORD versionlen = 40;
    char pathbuf[MAXPATHLEN];
    DWORD pathlen;
    DWORD pathtype;

    PRBool ok = PR_FALSE;

    if (::RegQueryValueEx(subKey, "Version", NULL, NULL,
                          (BYTE*) version, &versionlen) == ERROR_SUCCESS &&
        CheckVersion(version, versions, versionsLength)) {

      ok = PR_TRUE;
      const GREProperty *props = properties;
      const GREProperty *propsEnd = properties + propertiesLength;
      for (; ok && props < propsEnd; ++props) {
        pathlen = sizeof(pathbuf);

        if (::RegQueryValueEx(subKey, props->property, NULL, &pathtype,
                              (BYTE*) pathbuf, &pathlen) != ERROR_SUCCESS ||
            strcmp(pathbuf, props->value))
          ok = PR_FALSE;
      }

      pathlen = sizeof(pathbuf);
      if (ok &&
          (!::RegQueryValueEx(subKey, "GreHome", NULL, &pathtype,
                              (BYTE*) pathbuf, &pathlen) == ERROR_SUCCESS ||
           !*pathbuf ||
           !CopyWithEnvExpansion(aBuffer, pathbuf, aBufLen, pathtype))) {
        ok = PR_FALSE;
      }
      else if (!safe_strncat(aBuffer, "\\" XPCOM_DLL, aBufLen) ||
               access(aBuffer, R_OK)) {
        ok = PR_FALSE;
      }
    }

    RegCloseKey(subKey);

    if (ok)
      return PR_TRUE;

    ++i;
  }

  aBuffer[0] = '\0';

  return PR_FALSE;
}
#endif 
