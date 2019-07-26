





#include "TestHarness.h"

#include <windows.h>
#include <winnetwk.h>

#include "mozilla/FileUtilsWin.h"
#include "mozilla/DebugOnly.h"
#include "nsCRTGlue.h"

class DriveMapping
{
public:
  DriveMapping(const nsAString& aRemoteUNCPath);
  ~DriveMapping();

  bool
  Init();
  bool
  ChangeDriveLetter();
  wchar_t
  GetDriveLetter() { return mDriveLetter; }

private:
  bool
  DoMapping();
  void
  Disconnect(wchar_t aDriveLetter);

  wchar_t   mDriveLetter;
  nsString  mRemoteUNCPath;
};

DriveMapping::DriveMapping(const nsAString& aRemoteUNCPath)
  : mDriveLetter(0)
  , mRemoteUNCPath(aRemoteUNCPath)
{
}

bool
DriveMapping::Init()
{
  if (mDriveLetter) {
    return false;
  }
  return DoMapping();
}

bool
DriveMapping::DoMapping()
{
  wchar_t drvTemplate[] = L" :";
  NETRESOURCEW netRes = {0};
  netRes.dwType = RESOURCETYPE_DISK;
  netRes.lpLocalName = drvTemplate;
  netRes.lpRemoteName = reinterpret_cast<wchar_t*>(mRemoteUNCPath.BeginWriting());
  wchar_t driveLetter = L'D';
  DWORD result = NO_ERROR;
  do {
    drvTemplate[0] = driveLetter;
    result = WNetAddConnection2W(&netRes, nullptr, nullptr, CONNECT_TEMPORARY);
  } while (result == ERROR_ALREADY_ASSIGNED && ++driveLetter <= L'Z');
  if (result != NO_ERROR) {
    return false;
  }
  mDriveLetter = driveLetter;
  return true;
}

bool
DriveMapping::ChangeDriveLetter()
{
  wchar_t prevDriveLetter = mDriveLetter;
  bool result = DoMapping();
  MOZ_ASSERT(mDriveLetter != prevDriveLetter);
  if (result && prevDriveLetter) {
    Disconnect(prevDriveLetter);
  }
  return result;
}

void
DriveMapping::Disconnect(wchar_t aDriveLetter)
{
  wchar_t drvTemplate[] = {aDriveLetter, L':', L'\0'};
  mozilla::DebugOnly<DWORD> result = WNetCancelConnection2W(drvTemplate, 0, TRUE);
  MOZ_ASSERT(result == NO_ERROR);
}

DriveMapping::~DriveMapping()
{
  if (mDriveLetter) {
    Disconnect(mDriveLetter);
  }
}

bool
DriveToNtPath(const wchar_t aDriveLetter, nsAString& aNtPath)
{
  const wchar_t drvTpl[] = {aDriveLetter, L':', L'\0'};
  aNtPath.SetLength(MAX_PATH);
  DWORD pathLen;
  while (true) {
    pathLen = QueryDosDeviceW(drvTpl, reinterpret_cast<wchar_t*>(aNtPath.BeginWriting()), aNtPath.Length());
    if (pathLen || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      break;
    }
    aNtPath.SetLength(aNtPath.Length() * 2);
  }
  if (!pathLen) {
    return false;
  }
  
  
  aNtPath.SetLength(NS_strlen(aNtPath.BeginReading()));
  return true;
}

bool
TestNtPathToDosPath(const wchar_t* aNtPath,
                    const wchar_t* aExpectedDosPath)
{
  nsAutoString output;
  bool result = mozilla::NtPathToDosPath(nsDependentString(aNtPath), output);
  return result && output == aExpectedDosPath;
}

int main(int argc, char* argv[])
{
  ScopedXPCOM xpcom("NtPathToDosPath");
  if (xpcom.failed()) {
    fail("XPCOM Startup");
    return 1;
  }
  nsAutoString cDrive;
  if (!DriveToNtPath(L'C', cDrive)) {
    fail("Querying for this machine's C:");
    return 1;
  }

  int result = 0;

  
  if (!TestNtPathToDosPath(L"", L"")) {
    fail("Empty string");
    result = 1;
  }
  
  if (TestNtPathToDosPath(L"\\Device\\ThisDeviceDoesNotExist\\Foo", nullptr)) {
    fail("Non-existent device");
    result = 1;
  }
  
  nsAutoString testPath(cDrive);
  testPath.Append(L"\\Foo");
  if (!TestNtPathToDosPath(testPath.get(), L"C:\\Foo")) {
    fail("Base case");
    result = 1;
  }
  
  if (!TestNtPathToDosPath(L"\\??\\C:\\Foo", L"C:\\Foo")) {
    fail("Path specified as symbolic link");
    result = 1;
  }
  
  if (TestNtPathToDosPath(L"\\??\\MountPointManager", nullptr)) {
    fail("Other symbolic link");
    result = 1;
  }
  
  if (TestNtPathToDosPath(L"\\Device\\Afd\\Endpoint", nullptr)) {
    fail("Socket");
    result = 1;
  }
  
  if (!TestNtPathToDosPath(L"\\Device\\Mup\\127.0.0.1\\C$",
                           L"\\\\127.0.0.1\\C$")) {
    fail("Unmapped UNC path (\\Device\\Mup\\)");
    result = 1;
  }
  
  if (!TestNtPathToDosPath(L"\\Device\\LanmanRedirector\\127.0.0.1\\C$",
                           L"\\\\127.0.0.1\\C$")) {
    fail("Unmapped UNC path (\\Device\\LanmanRedirector\\)");
    result = 1;
  }
  DriveMapping drvMapping(NS_LITERAL_STRING("\\\\127.0.0.1\\C$"));
  
  if (drvMapping.Init()) {
    wchar_t expected[] = L" :\\";
    expected[0] = drvMapping.GetDriveLetter();
    nsAutoString networkPath;
    if (!DriveToNtPath(drvMapping.GetDriveLetter(), networkPath)) {
      fail("Querying network drive");
      return 1;
    }
    networkPath += MOZ_UTF16("\\");
    if (!TestNtPathToDosPath(networkPath.get(), expected)) {
      fail("Mapped UNC path");
      result = 1;
    }
    
    
    
    if (!drvMapping.ChangeDriveLetter()) {
      fail("Change drive letter");
      return 1;
    }
    expected[0] = drvMapping.GetDriveLetter();
    if (!DriveToNtPath(drvMapping.GetDriveLetter(), networkPath)) {
      fail("Querying second network drive");
      return 1;
    }
    networkPath += MOZ_UTF16("\\");
    if (!TestNtPathToDosPath(networkPath.get(), expected)) {
      fail("Re-mapped UNC path");
      result = 1;
    }
  }

  return result;
}

