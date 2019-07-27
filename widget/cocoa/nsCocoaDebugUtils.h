




#ifndef nsCocoaDebugUtils_h_
#define nsCocoaDebugUtils_h_

#include <CoreServices/CoreServices.h>






typedef struct _CSTypeRef {
  unsigned long type;
  void* contents;
} CSTypeRef;

typedef CSTypeRef CSSymbolicatorRef;
typedef CSTypeRef CSSymbolOwnerRef;
typedef CSTypeRef CSSymbolRef;
typedef CSTypeRef CSSourceInfoRef;

typedef struct _CSRange {
  unsigned long long location;
  unsigned long long length;
} CSRange;

typedef unsigned long long CSArchitecture;

#define kCSNow LONG_MAX

extern "C" {

CSSymbolicatorRef
CSSymbolicatorCreateWithPid(pid_t pid);

CSSymbolicatorRef
CSSymbolicatorCreateWithPidFlagsAndNotification(pid_t pid,
                                                uint32_t flags,
                                                uint32_t notification);

CSArchitecture
CSSymbolicatorGetArchitecture(CSSymbolicatorRef symbolicator);

CSSymbolOwnerRef
CSSymbolicatorGetSymbolOwnerWithAddressAtTime(CSSymbolicatorRef symbolicator,
                                              unsigned long long address,
                                              long time);

const char*
CSSymbolOwnerGetName(CSSymbolOwnerRef owner);

unsigned long long
CSSymbolOwnerGetBaseAddress(CSSymbolOwnerRef owner);

CSSymbolRef
CSSymbolOwnerGetSymbolWithAddress(CSSymbolOwnerRef owner,
                                  unsigned long long address);

CSSourceInfoRef
CSSymbolOwnerGetSourceInfoWithAddress(CSSymbolOwnerRef owner,
                                      unsigned long long address);

const char*
CSSymbolGetName(CSSymbolRef symbol);

CSRange
CSSymbolGetRange(CSSymbolRef symbol);

const char*
CSSourceInfoGetFilename(CSSourceInfoRef info);

uint32_t
CSSourceInfoGetLineNumber(CSSourceInfoRef info);

CSTypeRef
CSRetain(CSTypeRef);

void
CSRelease(CSTypeRef);

bool
CSIsNull(CSTypeRef);

void
CSShow(CSTypeRef);

const char*
CSArchitectureGetFamilyName(CSArchitecture);

} 

class nsCocoaDebugUtils
{
public:
  
  
  
  static void DebugLog(const char* aFormat, ...);

  
  
  static void PrintStackTrace();

  
  
  static char* GetOwnerName(void* aAddress);

  
  
  static char* GetAddressString(void* aAddress);

private:
  static void DebugLogInt(bool aDecorate, const char* aFormat, ...);
  static void DebugLogV(bool aDecorate, CFStringRef aFormat, va_list aArgs);

  static void PrintAddress(void* aAddress);

  
  
  static char* GetOwnerNameInt(void* aAddress,
                               CSTypeRef aOwner = sInitializer);
  static char* GetAddressStringInt(void* aAddress,
                                   CSTypeRef aOwner = sInitializer);

  static CSSymbolicatorRef GetSymbolicatorRef();
  static void ReleaseSymbolicator();

  static CSTypeRef sInitializer;
  static CSSymbolicatorRef sSymbolicator;
};

#endif 
