






































#include "crashreporter.h"


#pragma warning( disable : 4530 )
#include <fstream>
#include <sstream>

using std::string;
using std::istream;
using std::ifstream;
using std::istringstream;
using std::ostream;
using std::ofstream;

StringTable  gStrings;
int          gArgc;
const char** gArgv;

static string       gDumpFile;
static string       gExtraFile;
static string       gSettingsPath;

static string kExtraDataExtension = ".extra";

static bool ReadStrings(istream &in, StringTable &strings)
{
  string currentSection;
  while (!in.eof()) {
    string line;
    std::getline(in, line);
    int sep = line.find('=');
    if (sep >= 0) {
      string key, value;
      key = line.substr(0, sep);
      value = line.substr(sep + 1);
      strings[key] = value;
    }
  }

  return true;
}

static bool ReadStringsFromFile(const string& path,
                                StringTable& strings)
{
  ifstream f(path.c_str(), std::ios::in);
  if (!f.is_open()) return false;

  return ReadStrings(f, strings);
}

static bool ReadConfig()
{
  string iniPath;
  if (!UIGetIniPath(iniPath))
    return false;

  if (!ReadStringsFromFile(iniPath, gStrings))
    return false;

  return true;
}

static string GetExtraDataFilename(const string& dumpfile)
{
  string filename(dumpfile);
  int dot = filename.rfind('.');
  if (dot < 0)
    return "";

  filename.replace(dot, filename.length() - dot, kExtraDataExtension);
  return filename;
}

static string Basename(const string& file)
{
  int slashIndex = file.rfind(UI_DIR_SEPARATOR);
  if (slashIndex >= 0)
    return file.substr(slashIndex + 1);
  else
    return file;
}

static bool MoveCrashData(const string& toDir,
                          string& dumpfile,
                          string& extrafile)
{
  if (!UIEnsurePathExists(toDir)) {
    UIError(toDir.c_str());
    return false;
  }

  string newDump = toDir + UI_DIR_SEPARATOR + Basename(dumpfile);
  string newExtra = toDir + UI_DIR_SEPARATOR + Basename(extrafile);

  if (!UIMoveFile(dumpfile, newDump) ||
      !UIMoveFile(extrafile, newExtra)) {
    UIError(dumpfile.c_str());
    UIError(newDump.c_str());
    return false;
  }

  dumpfile = newDump;
  extrafile = newExtra;

  return true;
}

static bool AddSubmittedReport(const string& serverResponse)
{
  StringTable responseItems;
  istringstream in(serverResponse);
  ReadStrings(in, responseItems);

  if (responseItems.find("CrashID") == responseItems.end())
    return false;

  string submittedDir =
    gSettingsPath + UI_DIR_SEPARATOR + "submitted";
  if (!UIEnsurePathExists(submittedDir)) {
    return false;
  }

  string path = submittedDir + UI_DIR_SEPARATOR +
    responseItems["CrashID"] + ".txt";

  ofstream file(path.c_str());
  if (!file.is_open())
    return false;

  char buf[1024];
  UI_SNPRINTF(buf, 1024,
              gStrings["CrashID"].c_str(),
              responseItems["CrashID"].c_str());
  file << buf << "\n";

  if (responseItems.find("ViewURL") != responseItems.end()) {
    UI_SNPRINTF(buf, 1024,
                gStrings["ViewURL"].c_str(),
                responseItems["ViewURL"].c_str());
    file << buf << "\n";
  }

  file.close();

  return true;

}

bool CrashReporterSendCompleted(bool success,
                                const string& serverResponse)
{
  if (success) {
    if (!gDumpFile.empty())
      UIDeleteFile(gDumpFile);
    if (!gExtraFile.empty())
      UIDeleteFile(gExtraFile);

    return AddSubmittedReport(serverResponse);
  }
  return true;
}

int main(int argc, const char** argv)
{
  gArgc = argc;
  gArgv = argv;

  if (!ReadConfig()) {
    UIError("Couldn't read configuration");
    return 0;
  }

  if (!UIInit())
    return 0;

  if (argc > 1) {
    gDumpFile = argv[1];
  }

  if (gDumpFile.empty()) {
    
    UIShowDefaultUI();
  } else {
    gExtraFile = GetExtraDataFilename(gDumpFile);
    if (gExtraFile.empty()) {
      UIError("Couldn't get extra data filename");
      return 0;
    }

    StringTable queryParameters;
    if (!ReadStringsFromFile(gExtraFile, queryParameters)) {
      UIError("Couldn't read extra data");
      return 0;
    }

    if (queryParameters.find("ProductName") == queryParameters.end()) {
      UIError("No product name specified");
      return 0;
    }

    if (queryParameters.find("ServerURL") == queryParameters.end()) {
      UIError("No server URL specified");
      return 0;
    }

    string product = queryParameters["ProductName"];
    string vendor = queryParameters["Vendor"];
    if (!UIGetSettingsPath(vendor, product, gSettingsPath)) {
      UIError("Couldn't get settings path");
      return 0;
    }

    string pendingDir = gSettingsPath + UI_DIR_SEPARATOR + "pending";
    if (!MoveCrashData(pendingDir, gDumpFile, gExtraFile)) {
      UIError("Couldn't move crash data");
      return 0;
    }

    string sendURL = queryParameters["ServerURL"];
    
    queryParameters.erase("ServerURL");

    
    
    
    char* urlEnv = getenv("MOZ_CRASHREPORTER_URL");
    if (urlEnv && *urlEnv) {
      sendURL = urlEnv;
    }

    UIShowCrashUI(gDumpFile, queryParameters, sendURL);
  }

  UIShutdown();

  return 0;
}

#if defined(XP_WIN) && !defined(__GNUC__)


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
  
  return main(__argc, (const char**)__argv);
}
#endif
