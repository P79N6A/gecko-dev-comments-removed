




































#include "crashreporter.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include <algorithm>
#include <cctype>

using std::string;

bool UIInit()
{
  
  return true;
}

void UIShutdown()
{
  
}

void UIShowDefaultUI()
{
  
}

void UIShowCrashUI(const string& dumpfile,
                   const StringTable& queryParameters,
                   const string& sendURL)
{
  
}

void UIError(const string& message)
{
  
  printf("Error: %s\n", message.c_str());
}

bool UIGetIniPath(string& path)
{
  path = gArgv[0];
  path.append(".ini");

  return true;
}





bool UIGetSettingsPath(const string& vendor,
                       const string& product,
                       string& settingsPath)
{
  char* home = getenv("HOME");
  
  if (!home)
    return false;

  settingsPath = home;
  settingsPath += "/.";
  if (!vendor.empty()) {
    string lc_vendor;
    std::transform(vendor.begin(), vendor.end(), back_inserter(lc_vendor),
		   (int(*)(int)) std::tolower);
    settingsPath += lc_vendor + "/";
  }
  string lc_product;
  std::transform(product.begin(), product.end(), back_inserter(lc_product),
		 (int(*)(int)) std::tolower);
  settingsPath += lc_product + "/Crash Reports";
  printf("settingsPath: %s\n", settingsPath.c_str());
  return UIEnsurePathExists(settingsPath);
}

bool UIEnsurePathExists(const string& path)
{
  int ret = mkdir(path.c_str(), S_IRWXU);
  int e = errno;
  if (ret == -1 && e != EEXIST)
    return false;

  return true;
}

bool UIMoveFile(const string& file, const string& newfile)
{
  return (rename(file.c_str(), newfile.c_str()) != -1);
}

bool UIDeleteFile(const string& file)
{
  return (unlink(file.c_str()) != -1);
}
