





#include "mozilla/EMEUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"

namespace mozilla {

#ifdef PR_LOGGING

PRLogModuleInfo* GetEMELog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("EME");
  }
  return log;
}

PRLogModuleInfo* GetEMEVerboseLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("EMEV");
  }
  return log;
}

#endif

static bool
ContainsOnlyDigits(const nsAString& aString)
{
  nsAString::const_iterator iter, end;
  aString.BeginReading(iter);
  aString.EndReading(end);
  while (iter != end) {
    char16_t ch = *iter;
    if (ch < '0' || ch > '9') {
      return false;
    }
    iter++;
  }
  return true;
}

static bool
ParseKeySystem(const nsAString& aExpectedKeySystem,
               const nsAString& aInputKeySystem,
               int32_t& aOutCDMVersion)
{
  if (!StringBeginsWith(aInputKeySystem, aExpectedKeySystem)) {
    return false;
  }

  if (aInputKeySystem.Length() > aExpectedKeySystem.Length() + 8) {
    
    
    NS_WARNING("Input KeySystem including was suspiciously long");
    return false;
  }

  const char16_t* versionStart = aInputKeySystem.BeginReading() + aExpectedKeySystem.Length();
  const char16_t* end = aInputKeySystem.EndReading();
  if (versionStart == end) {
    
    aOutCDMVersion = NO_CDM_VERSION;
    return true;
  }
  if (*versionStart != '.') {
    
    NS_WARNING("EME keySystem version string not prefixed by '.'");
    return false;
  }
  versionStart++;
  const nsAutoString versionStr(Substring(versionStart, end));
  if (!ContainsOnlyDigits(versionStr)) {
    NS_WARNING("Non-digit character in EME keySystem string's version suffix");
    return false;
  }
  nsresult rv;
  int32_t version = versionStr.ToInteger(&rv);
  if (NS_FAILED(rv) || version < 0) {
    NS_WARNING("Invalid version in EME keySystem string");
    return false;
  }
  aOutCDMVersion = version;

  return true;
}

static const char16_t* sKeySystems[] = {
  MOZ_UTF16("org.w3.clearkey"),
  MOZ_UTF16("com.adobe.access"),
  MOZ_UTF16("com.adobe.primetime"),
};

bool
ParseKeySystem(const nsAString& aInputKeySystem,
               nsAString& aOutKeySystem,
               int32_t& aOutCDMVersion)
{
  for (const char16_t* keySystem : sKeySystems) {
    int32_t minCDMVersion = NO_CDM_VERSION;
    if (ParseKeySystem(nsDependentString(keySystem),
                       aInputKeySystem,
                       minCDMVersion)) {
      aOutKeySystem = keySystem;
      aOutCDMVersion = minCDMVersion;
      return true;
    }
  }
  return false;
}

bool
GetEMEVoucherPath(nsIFile** aPath)
{
  nsCOMPtr<nsIFile> path;
  NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(path));
  if (!path) {
    NS_WARNING("GetEMEVoucherPath can't get NS_GRE_DIR!");
    return false;
  }
  path->AppendNative(NS_LITERAL_CSTRING("voucher.bin"));
  path.forget(aPath);
  return true;
}

bool
EMEVoucherFileExists()
{
  nsCOMPtr<nsIFile> path;
  bool exists;
  return GetEMEVoucherPath(getter_AddRefs(path)) &&
         NS_SUCCEEDED(path->Exists(&exists)) &&
         exists;
}

} 
