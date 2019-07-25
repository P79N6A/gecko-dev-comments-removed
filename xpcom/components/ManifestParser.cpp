




































#include "ManifestParser.h"

#include <string.h>

#include "prio.h"
#include "prprf.h"
#if defined(XP_WIN)
#include <windows.h>
#elif defined(XP_MACOSX)
#include <CoreServices/CoreServices.h>
#elif defined(MOZ_WIDGET_GTK2)
#include <gtk/gtk.h>
#endif

#ifdef ANDROID
#include "AndroidBridge.h"
#endif

#include "mozilla/Services.h"

#include "nsConsoleMessage.h"
#include "nsTextFormatter.h"
#include "nsVersionComparator.h"
#include "nsXPCOMCIDInternal.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIXULAppInfo.h"
#include "nsIXULRuntime.h"

struct ManifestDirective
{
  const char* directive;
  int argc;

  
  
  bool componentonly;

  bool ischrome;

  
  bool contentflags;

  
  
  void (nsComponentManagerImpl::*mgrfunc)
    (nsComponentManagerImpl::ManifestProcessingContext& cx,
     int lineno, char *const * argv);
  void (nsChromeRegistry::*regfunc)
    (nsChromeRegistry::ManifestProcessingContext& cx,
     int lineno, char *const *argv,
     bool platform, bool contentaccessible);

  bool isContract;
};
static const ManifestDirective kParsingTable[] = {
  { "manifest", 1, false, true, false,
    &nsComponentManagerImpl::ManifestManifest, NULL },
  { "binary-component", 1, true, false, false,
    &nsComponentManagerImpl::ManifestBinaryComponent, NULL },
  { "interfaces",       1, true, false, false,
    &nsComponentManagerImpl::ManifestXPT, NULL },
  { "component",        2, true, false, false,
    &nsComponentManagerImpl::ManifestComponent, NULL },
  { "contract",         2, true, false, false,
    &nsComponentManagerImpl::ManifestContract, NULL, true},
  { "category",         3, true, false, false,
    &nsComponentManagerImpl::ManifestCategory, NULL },
  { "content",          2, true, true,  true,
    NULL, &nsChromeRegistry::ManifestContent },
  { "locale",           3, true, true,  false,
    NULL, &nsChromeRegistry::ManifestLocale },
  { "skin",             3, false, true,  false,
    NULL, &nsChromeRegistry::ManifestSkin },
  { "overlay",          2, true, true,  false,
    NULL, &nsChromeRegistry::ManifestOverlay },
  { "style",            2, false, true,  false,
    NULL, &nsChromeRegistry::ManifestStyle },
  { "override",         2, true, true,  false,
    NULL, &nsChromeRegistry::ManifestOverride },
  { "resource",         2, true, true,  false,
    NULL, &nsChromeRegistry::ManifestResource }
};

static const char kWhitespace[] = "\t ";

static bool IsNewline(char c)
{
  return c == '\n' || c == '\r';
}

namespace {
struct AutoPR_smprintf_free
{
  AutoPR_smprintf_free(char* buf)
    : mBuf(buf)
  {
  }

  ~AutoPR_smprintf_free()
  {
    if (mBuf)
      PR_smprintf_free(mBuf);
  }

  operator char*() const {
    return mBuf;
  }

  char* mBuf;
};

} 

void LogMessage(const char* aMsg, ...)
{
  nsCOMPtr<nsIConsoleService> console =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  if (!console)
    return;

  va_list args;
  va_start(args, aMsg);
  AutoPR_smprintf_free formatted(PR_vsmprintf(aMsg, args));
  va_end(args);

  nsCOMPtr<nsIConsoleMessage> error =
    new nsConsoleMessage(NS_ConvertUTF8toUTF16(formatted).get());
  console->LogMessage(error);
}

void LogMessageWithContext(nsILocalFile* aFile, const char* aPath,
                           PRUint32 aLineNumber, const char* aMsg, ...)
{
  va_list args;
  va_start(args, aMsg);
  AutoPR_smprintf_free formatted(PR_vsmprintf(aMsg, args));
  va_end(args);
  if (!formatted)
    return;

  nsString file;
  aFile->GetPath(file);
  if (aPath) {
    file.Append(':');
    file.Append(NS_ConvertUTF8toUTF16(aPath));
  }

  nsCOMPtr<nsIScriptError> error =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
  if (!error) {
    
    
    LogMessage("Warning: in file '%s', line %i: %s",
               NS_ConvertUTF16toUTF8(file).get(),
               aLineNumber, (char*) formatted);
    return;
  }

  nsCOMPtr<nsIConsoleService> console =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  if (!console)
    return;

  nsresult rv = error->Init(NS_ConvertUTF8toUTF16(formatted).get(),
			    file.get(), NULL,
			    aLineNumber, 0, nsIScriptError::warningFlag,
			    "chrome registration");
  if (NS_FAILED(rv))
    return;

  console->LogMessage(error);
}












static bool
CheckFlag(const nsSubstring& aFlag, const nsSubstring& aData, bool& aResult)
{
  if (!StringBeginsWith(aData, aFlag))
    return false;

  if (aFlag.Length() == aData.Length()) {
    
    aResult = true;
    return true;
  }

  if (aData.CharAt(aFlag.Length()) != '=') {
    
    return false;
  }

  if (aData.Length() == aFlag.Length() + 1) {
    aResult = false;
    return true;
  }

  switch (aData.CharAt(aFlag.Length() + 1)) {
  case '1':
  case 't': 
  case 'y': 
    aResult = true;
    return true;

  case '0':
  case 'f': 
  case 'n': 
    aResult = false;
    return true;
  }

  return false;
}

enum TriState {
  eUnspecified,
  eBad,
  eOK
};













static bool
CheckStringFlag(const nsSubstring& aFlag, const nsSubstring& aData,
                const nsSubstring& aValue, TriState& aResult)
{
  if (aData.Length() < aFlag.Length() + 1)
    return false;

  if (!StringBeginsWith(aData, aFlag))
    return false;

  bool comparison = true;
  if (aData[aFlag.Length()] != '=') {
    if (aData[aFlag.Length()] == '!' &&
        aData.Length() >= aFlag.Length() + 2 &&
        aData[aFlag.Length() + 1] == '=')
      comparison = false;
    else
      return false;
  }

  if (aResult != eOK) {
    nsDependentSubstring testdata = Substring(aData, aFlag.Length() + (comparison ? 1 : 2));
    if (testdata.Equals(aValue))
      aResult = comparison ? eOK : eBad;
    else
      aResult = comparison ? eBad : eOK;
  }

  return true;
}


















#define COMPARE_EQ    1 << 0
#define COMPARE_LT    1 << 1
#define COMPARE_GT    1 << 2

static bool
CheckVersionFlag(const nsString& aFlag, const nsString& aData,
                 const nsString& aValue, TriState& aResult)
{
  if (aData.Length() < aFlag.Length() + 2)
    return false;

  if (!StringBeginsWith(aData, aFlag))
    return false;

  if (aValue.Length() == 0) {
    if (aResult != eOK)
      aResult = eBad;
    return true;
  }

  PRUint32 comparison;
  nsAutoString testdata;

  switch (aData[aFlag.Length()]) {
  case '=':
    comparison = COMPARE_EQ;
    testdata = Substring(aData, aFlag.Length() + 1);
    break;

  case '<':
    if (aData[aFlag.Length() + 1] == '=') {
      comparison = COMPARE_EQ | COMPARE_LT;
      testdata = Substring(aData, aFlag.Length() + 2);
    }
    else {
      comparison = COMPARE_LT;
      testdata = Substring(aData, aFlag.Length() + 1);
    }
    break;

  case '>':
    if (aData[aFlag.Length() + 1] == '=') {
      comparison = COMPARE_EQ | COMPARE_GT;
      testdata = Substring(aData, aFlag.Length() + 2);
    }
    else {
      comparison = COMPARE_GT;
      testdata = Substring(aData, aFlag.Length() + 1);
    }
    break;

  default:
    return false;
  }

  if (testdata.Length() == 0)
    return false;

  if (aResult != eOK) {
    PRInt32 c = NS_CompareVersions(NS_ConvertUTF16toUTF8(aValue).get(),
                                   NS_ConvertUTF16toUTF8(testdata).get());
    if ((c == 0 && comparison & COMPARE_EQ) ||
	(c < 0 && comparison & COMPARE_LT) ||
	(c > 0 && comparison & COMPARE_GT))
      aResult = eOK;
    else
      aResult = eBad;
  }

  return true;
}


static void
ToLowerCase(char* token)
{
  for (; *token; ++token)
    *token = NS_ToLower(*token);
}

namespace {

struct CachedDirective
{
  int lineno;
  char* argv[4];
};

} 

static void
ParseManifestCommon(NSLocationType aType, nsILocalFile* aFile,
                    nsComponentManagerImpl::ManifestProcessingContext& mgrcx,
                    nsChromeRegistry::ManifestProcessingContext& chromecx,
                    const char* aPath, char* buf, bool aChromeOnly)
{
  nsresult rv;

  NS_NAMED_LITERAL_STRING(kPlatform, "platform");
  NS_NAMED_LITERAL_STRING(kContentAccessible, "contentaccessible");
  NS_NAMED_LITERAL_STRING(kApplication, "application");
  NS_NAMED_LITERAL_STRING(kAppVersion, "appversion");
  NS_NAMED_LITERAL_STRING(kOs, "os");
  NS_NAMED_LITERAL_STRING(kOsVersion, "osversion");
  NS_NAMED_LITERAL_STRING(kABI, "abi");

  
  NS_NAMED_LITERAL_STRING(kXPCNativeWrappers, "xpcnativewrappers");

  nsAutoString appID;
  nsAutoString appVersion;
  nsAutoString osTarget;
  nsAutoString abi;

  nsCOMPtr<nsIXULAppInfo> xapp (do_GetService(XULAPPINFO_SERVICE_CONTRACTID));
  if (xapp) {
    nsCAutoString s;
    rv = xapp->GetID(s);
    if (NS_SUCCEEDED(rv))
      CopyUTF8toUTF16(s, appID);

    rv = xapp->GetVersion(s);
    if (NS_SUCCEEDED(rv))
      CopyUTF8toUTF16(s, appVersion);
    
    nsCOMPtr<nsIXULRuntime> xruntime (do_QueryInterface(xapp));
    if (xruntime) {
      rv = xruntime->GetOS(s);
      if (NS_SUCCEEDED(rv)) {
        ToLowerCase(s);
        CopyUTF8toUTF16(s, osTarget);
      }

      rv = xruntime->GetXPCOMABI(s);
      if (NS_SUCCEEDED(rv) && osTarget.Length()) {
        ToLowerCase(s);
        CopyUTF8toUTF16(s, abi);
        abi.Insert(PRUnichar('_'), 0);
        abi.Insert(osTarget, 0);
      }
    }
  }

  nsAutoString osVersion;
#if defined(XP_WIN)
  OSVERSIONINFO info = { sizeof(OSVERSIONINFO) };
  if (GetVersionEx(&info)) {
    nsTextFormatter::ssprintf(osVersion, NS_LITERAL_STRING("%ld.%ld").get(),
                                         info.dwMajorVersion,
                                         info.dwMinorVersion);
  }
#elif defined(XP_MACOSX)
  SInt32 majorVersion, minorVersion;
  if ((Gestalt(gestaltSystemVersionMajor, &majorVersion) == noErr) &&
      (Gestalt(gestaltSystemVersionMinor, &minorVersion) == noErr)) {
    nsTextFormatter::ssprintf(osVersion, NS_LITERAL_STRING("%ld.%ld").get(),
                                         majorVersion,
                                         minorVersion);
  }
#elif defined(MOZ_WIDGET_GTK2)
  nsTextFormatter::ssprintf(osVersion, NS_LITERAL_STRING("%ld.%ld").get(),
                                       gtk_major_version,
                                       gtk_minor_version);
#elif defined(ANDROID)
  if (mozilla::AndroidBridge::Bridge()) {
    mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build$VERSION", "RELEASE", osVersion);
  }
#endif

  
  
  nsTArray<CachedDirective> contracts;

  char *token;
  char *newline = buf;
  PRUint32 line = 0;

  
  while (*newline) {
    while (*newline && IsNewline(*newline)) {
      ++newline;
      ++line;
    }
    if (!*newline)
      break;

    token = newline;
    while (*newline && !IsNewline(*newline))
      ++newline;

    if (*newline) {
      *newline = '\0';
      ++newline;
    }
    ++line;

    if (*token == '#') 
      continue;

    char *whitespace = token;
    token = nsCRT::strtok(whitespace, kWhitespace, &whitespace);
    if (!token) continue;

    const ManifestDirective* directive = NULL;
    for (const ManifestDirective* d = kParsingTable;
	 d < kParsingTable + NS_ARRAY_LENGTH(kParsingTable);
	 ++d) {
      if (!strcmp(d->directive, token)) {
	directive = d;
	break;
      }
    }
    if (!directive) {
      LogMessageWithContext(aFile, aPath, line,
                            "Ignoring unrecognized chrome manifest directive '%s'.",
                            token);
      continue;
    }
    if (directive->componentonly && NS_COMPONENT_LOCATION != aType) {
      LogMessageWithContext(aFile, aPath, line,
                            "Skin manifest not allowed to use '%s' directive.",
                            token);
      continue;
    }

    NS_ASSERTION(directive->argc < 4, "Need to reset argv array length");
    char* argv[4];
    for (int i = 0; i < directive->argc; ++i)
      argv[i] = nsCRT::strtok(whitespace, kWhitespace, &whitespace);

    if (!argv[directive->argc - 1]) {
      LogMessageWithContext(aFile, aPath, line,
                            "Not enough arguments for chrome manifest directive '%s', expected %i.",
                            token, directive->argc);
      continue;
    }

    bool ok = true;
    TriState stAppVersion = eUnspecified;
    TriState stApp = eUnspecified;
    TriState stOsVersion = eUnspecified;
    TriState stOs = eUnspecified;
    TriState stABI = eUnspecified;
    bool platform = false;
    bool contentAccessible = false;

    while (NULL != (token = nsCRT::strtok(whitespace, kWhitespace, &whitespace)) && ok) {
      ToLowerCase(token);
      NS_ConvertASCIItoUTF16 wtoken(token);

      if (CheckStringFlag(kApplication, wtoken, appID, stApp) ||
          CheckStringFlag(kOs, wtoken, osTarget, stOs) ||
          CheckStringFlag(kABI, wtoken, abi, stABI) ||
          CheckVersionFlag(kOsVersion, wtoken, osVersion, stOsVersion) ||
          CheckVersionFlag(kAppVersion, wtoken, appVersion, stAppVersion))
        continue;

      if (directive->contentflags &&
          (CheckFlag(kPlatform, wtoken, platform) ||
           CheckFlag(kContentAccessible, wtoken, contentAccessible)))
        continue;

      bool xpcNativeWrappers = true; 
      if (CheckFlag(kXPCNativeWrappers, wtoken, xpcNativeWrappers)) {
        LogMessageWithContext(aFile, aPath, line,
                              "Warning: Ignoring obsolete chrome registration modifier '%s'.",
                              token);
        continue;
      }

      LogMessageWithContext(aFile, aPath, line,
                            "Unrecognized chrome manifest modifier '%s'.",
                            token);
      ok = false;
    }

    if (!ok ||
        stApp == eBad ||
        stAppVersion == eBad ||
        stOs == eBad ||
        stOsVersion == eBad ||
        stABI == eBad)
      continue;

    if (directive->regfunc) {
      if (GeckoProcessType_Default != XRE_GetProcessType())
        continue;

      if (!nsChromeRegistry::gChromeRegistry) {
        nsCOMPtr<nsIChromeRegistry> cr =
          mozilla::services::GetChromeRegistryService();
        if (!nsChromeRegistry::gChromeRegistry) {
          LogMessageWithContext(aFile, aPath, line,
                                "Chrome registry isn't available yet.");
          continue;
        }
      }

      (nsChromeRegistry::gChromeRegistry->*(directive->regfunc))
	(chromecx, line, argv, platform, contentAccessible);
    }
    else if (directive->ischrome || !aChromeOnly) {
      if (directive->isContract) {
        CachedDirective* cd = contracts.AppendElement();
        cd->lineno = line;
        cd->argv[0] = argv[0];
        cd->argv[1] = argv[1];
      }
      else
        (nsComponentManagerImpl::gComponentManager->*(directive->mgrfunc))
          (mgrcx, line, argv);
    }
  }

  for (PRUint32 i = 0; i < contracts.Length(); ++i) {
    CachedDirective& d = contracts[i];
    nsComponentManagerImpl::gComponentManager->ManifestContract
      (mgrcx, d.lineno, d.argv);
  }
}

void
ParseManifest(NSLocationType type, nsILocalFile* file,
              char* buf, bool aChromeOnly)
{
  nsComponentManagerImpl::ManifestProcessingContext mgrcx(type, file, aChromeOnly);
  nsChromeRegistry::ManifestProcessingContext chromecx(type, file);
  ParseManifestCommon(type, file, mgrcx, chromecx, NULL, buf, aChromeOnly);
}

void
ParseManifest(NSLocationType type, nsIZipReader* reader, const char* jarPath,
              char* buf, bool aChromeOnly)
{
  nsComponentManagerImpl::ManifestProcessingContext mgrcx(type, reader, jarPath, aChromeOnly);
  nsChromeRegistry::ManifestProcessingContext chromecx(type, mgrcx.mFile, jarPath);
  ParseManifestCommon(type, mgrcx.mFile, mgrcx, chromecx, jarPath,
                      buf, aChromeOnly);
}
