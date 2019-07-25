




































#include "nsMacUtilsImpl.h"

#include <CoreFoundation/CoreFoundation.h>
#include <sys/sysctl.h>

NS_IMPL_ISUPPORTS1(nsMacUtilsImpl, nsIMacUtils)

nsresult nsMacUtilsImpl::GetArchString(nsAString& archString)
{
  if (!mBinaryArchs.IsEmpty()) {
    archString.Assign(mBinaryArchs);
    return NS_OK;
  }

  archString.Truncate();

  bool foundPPC = false,
         foundX86 = PR_FALSE,
         foundPPC64 = PR_FALSE,
         foundX86_64 = PR_FALSE;

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
    CFNumberRef arch = static_cast<CFNumberRef>(::CFArrayGetValueAtIndex(archList, i));

    int archInt = 0;
    if (!::CFNumberGetValue(arch, kCFNumberIntType, &archInt)) {
      ::CFRelease(archList);
      return NS_ERROR_FAILURE;
    }

    if (archInt == kCFBundleExecutableArchitecturePPC)
      foundPPC = PR_TRUE;
    else if (archInt == kCFBundleExecutableArchitectureI386)
      foundX86 = PR_TRUE;
    else if (archInt == kCFBundleExecutableArchitecturePPC64)
      foundPPC64 = PR_TRUE;
    else if (archInt == kCFBundleExecutableArchitectureX86_64)
      foundX86_64 = PR_TRUE;
  }

  ::CFRelease(archList);

  
  
  if (foundPPC) {
    mBinaryArchs.Append(NS_LITERAL_STRING("ppc"));
  }

  if (foundX86) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append(NS_LITERAL_STRING("-"));
    }
    mBinaryArchs.Append(NS_LITERAL_STRING("i386"));
  }

  if (foundPPC64) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append(NS_LITERAL_STRING("-"));
    }
    mBinaryArchs.Append(NS_LITERAL_STRING("ppc64"));
  }

  if (foundX86_64) {
    if (!mBinaryArchs.IsEmpty()) {
      mBinaryArchs.Append(NS_LITERAL_STRING("-"));
    }
    mBinaryArchs.Append(NS_LITERAL_STRING("x86_64"));
  }

  archString.Assign(mBinaryArchs);

  return (archString.IsEmpty() ? NS_ERROR_FAILURE : NS_OK);
}

NS_IMETHODIMP nsMacUtilsImpl::GetIsUniversalBinary(bool *aIsUniversalBinary)
{
  NS_ENSURE_ARG_POINTER(aIsUniversalBinary);
  *aIsUniversalBinary = PR_FALSE;

  nsAutoString archString;
  nsresult rv = GetArchString(archString);
  if (NS_FAILED(rv))
    return rv;

  
  
  *aIsUniversalBinary = (archString.Find("-") > -1);

  return NS_OK;
}

NS_IMETHODIMP nsMacUtilsImpl::GetArchitecturesInBinary(nsAString& archString)
{
  return GetArchString(archString);
}



NS_IMETHODIMP nsMacUtilsImpl::GetIsTranslated(bool *aIsTranslated)
{
#ifdef __ppc__
  static bool    sInitialized = false;

  
  
  
  static PRInt32 sIsNative = 1;

  if (!sInitialized) {
    size_t sz = sizeof(sIsNative);
    sysctlbyname("sysctl.proc_native", &sIsNative, &sz, NULL, 0);
    sInitialized = PR_TRUE;
  }

  *aIsTranslated = !sIsNative;
#else
  
  
  *aIsTranslated = PR_FALSE;
#endif

  return NS_OK;
}
