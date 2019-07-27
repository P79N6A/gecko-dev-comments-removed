





#include "nsMacUtilsImpl.h"

#include <CoreFoundation/CoreFoundation.h>

NS_IMPL_ISUPPORTS(nsMacUtilsImpl, nsIMacUtils)

nsresult
nsMacUtilsImpl::GetArchString(nsAString& aArchString)
{
  if (!mBinaryArchs.IsEmpty()) {
    aArchString.Assign(mBinaryArchs);
    return NS_OK;
  }

  aArchString.Truncate();

  bool foundPPC = false,
       foundX86 = false,
       foundPPC64 = false,
       foundX86_64 = false;

  CFBundleRef mainBundle = ::CFBundleGetMainBundle();
  if (!mainBundle) {
    return NS_ERROR_FAILURE;
  }

  CFArrayRef archList = ::CFBundleCopyExecutableArchitectures(mainBundle);
  if (!archList) {
    return NS_ERROR_FAILURE;
  }

  CFIndex archCount = ::CFArrayGetCount(archList);
  for (CFIndex i = 0; i < archCount; i++) {
    CFNumberRef arch =
      static_cast<CFNumberRef>(::CFArrayGetValueAtIndex(archList, i));

    int archInt = 0;
    if (!::CFNumberGetValue(arch, kCFNumberIntType, &archInt)) {
      ::CFRelease(archList);
      return NS_ERROR_FAILURE;
    }

    if (archInt == kCFBundleExecutableArchitecturePPC) {
      foundPPC = true;
    } else if (archInt == kCFBundleExecutableArchitectureI386) {
      foundX86 = true;
    } else if (archInt == kCFBundleExecutableArchitecturePPC64) {
      foundPPC64 = true;
    } else if (archInt == kCFBundleExecutableArchitectureX86_64) {
      foundX86_64 = true;
    }
  }

  ::CFRelease(archList);

  
  
  if (foundPPC) {
    mBinaryArchs.AppendLiteral("ppc");
  }

  if (foundX86) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append('-');
    }
    mBinaryArchs.AppendLiteral("i386");
  }

  if (foundPPC64) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append('-');
    }
    mBinaryArchs.AppendLiteral("ppc64");
  }

  if (foundX86_64) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append('-');
    }
    mBinaryArchs.AppendLiteral("x86_64");
  }

  aArchString.Assign(mBinaryArchs);

  return (aArchString.IsEmpty() ? NS_ERROR_FAILURE : NS_OK);
}

NS_IMETHODIMP
nsMacUtilsImpl::GetIsUniversalBinary(bool* aIsUniversalBinary)
{
  if (NS_WARN_IF(!aIsUniversalBinary)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aIsUniversalBinary = false;

  nsAutoString archString;
  nsresult rv = GetArchString(archString);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  *aIsUniversalBinary = (archString.Find("-") > -1);

  return NS_OK;
}

NS_IMETHODIMP
nsMacUtilsImpl::GetArchitecturesInBinary(nsAString& aArchString)
{
  return GetArchString(aArchString);
}



NS_IMETHODIMP
nsMacUtilsImpl::GetIsTranslated(bool* aIsTranslated)
{
#ifdef __ppc__
  static bool    sInitialized = false;

  
  
  
  static int32_t sIsNative = 1;

  if (!sInitialized) {
    size_t sz = sizeof(sIsNative);
    sysctlbyname("sysctl.proc_native", &sIsNative, &sz, nullptr, 0);
    sInitialized = true;
  }

  *aIsTranslated = !sIsNative;
#else
  
  
  *aIsTranslated = false;
#endif

  return NS_OK;
}
