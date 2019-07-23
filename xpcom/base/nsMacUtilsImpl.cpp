




































#include "nsMacUtilsImpl.h"

#include <CoreFoundation/CoreFoundation.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach-o/fat.h>

NS_IMPL_ISUPPORTS1(nsMacUtilsImpl, nsIMacUtils)




NS_IMETHODIMP nsMacUtilsImpl::GetIsUniversalBinary(PRBool *aIsUniversalBinary)
{
  static PRBool sInitialized = PR_FALSE,
                sIsUniversalBinary = PR_FALSE;

  if (sInitialized) {
    *aIsUniversalBinary = sIsUniversalBinary;
    return NS_OK;
  }

  PRBool foundPPC = PR_FALSE,
         foundX86 = PR_FALSE;
  CFURLRef executableURL = nsnull;
  int fd = -1;

  CFBundleRef mainBundle;
  if (!(mainBundle = ::CFBundleGetMainBundle()))
    goto done;

  if (!(executableURL = ::CFBundleCopyExecutableURL(mainBundle)))
    goto done;

  char executablePath[PATH_MAX];
  if (!::CFURLGetFileSystemRepresentation(executableURL, PR_TRUE,
                                          (UInt8*) executablePath,
                                          sizeof(executablePath)))
    goto done;

  if ((fd = open(executablePath, O_RDONLY)) == -1)
    goto done;

  struct fat_header fatHeader;
  if (read(fd, &fatHeader, sizeof(fatHeader)) != sizeof(fatHeader))
    goto done;

  
  fatHeader.magic = CFSwapInt32BigToHost(fatHeader.magic);
  fatHeader.nfat_arch = CFSwapInt32BigToHost(fatHeader.nfat_arch);

  
  if (fatHeader.magic != FAT_MAGIC)
    goto done;

  
  
  for (PRUint32 i = 0 ; i < fatHeader.nfat_arch ; i++) {
    struct fat_arch fatArch;
    if (read(fd, &fatArch, sizeof(fatArch)) != sizeof(fatArch))
      goto done;

    
    fatArch.cputype = CFSwapInt32BigToHost(fatArch.cputype);

    
    
    
    if (fatArch.cputype == CPU_TYPE_POWERPC)
      foundPPC = PR_TRUE;
    else if (fatArch.cputype == CPU_TYPE_I386)
      foundX86 = PR_TRUE;
  }

  if (foundPPC && foundX86)
    sIsUniversalBinary = PR_TRUE;

done:
  if (fd != -1)
    close(fd);
  if (executableURL)
    ::CFRelease(executableURL);

  *aIsUniversalBinary = sIsUniversalBinary;
  sInitialized = PR_TRUE;

  return NS_OK;
}



NS_IMETHODIMP nsMacUtilsImpl::GetIsTranslated(PRBool *aIsTranslated)
{
#ifdef __ppc__
  static PRBool  sInitialized = PR_FALSE;

  
  
  
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
