






































#include "crashreporter.h"

#ifdef _MSC_VER

# pragma warning( disable : 4530 )
#endif

#include <fstream>
#include <sstream>

using std::string;
using std::istream;
using std::ifstream;
using std::istringstream;
using std::ostringstream;
using std::ostream;
using std::ofstream;
using std::vector;

StringTable  gStrings;
int          gArgc;
const char** gArgv;

static string       gDumpFile;
static string       gExtraFile;
static string       gSettingsPath;

static string kExtraDataExtension = ".extra";

static string Unescape(const string& str)
{
  string ret;
  for (string::const_iterator iter = str.begin();
       iter != str.end();
       iter++) {
    if (*iter == '\\') {
      iter++;
      if (*iter == '\\'){
        ret.push_back('\\');
      } else if (*iter == 'n') {
        ret.push_back('\n');
      } else if (*iter == 't') {
        ret.push_back('\t');
      }
    } else {
      ret.push_back(*iter);
    }
  }

  return ret;
}

static bool ReadStrings(istream& in, StringTable& strings, bool unescape)
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
      if (unescape)
        value = Unescape(value);
      strings[key] = value;
    }
  }

  return true;
}

static bool ReadStringsFromFile(const string& path,
                                StringTable& strings,
                                bool unescape)
{
  ifstream f(path.c_str(), std::ios::in);
  if (!f.is_open()) return false;

  return ReadStrings(f, strings, unescape);
}

static bool ReadConfig()
{
  string iniPath;
  if (!UIGetIniPath(iniPath))
    return false;

  if (!ReadStringsFromFile(iniPath, gStrings, true))
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
    return false;
  }

  string newDump = toDir + UI_DIR_SEPARATOR + Basename(dumpfile);
  string newExtra = toDir + UI_DIR_SEPARATOR + Basename(extrafile);

  if (!UIMoveFile(dumpfile, newDump) ||
      !UIMoveFile(extrafile, newExtra)) {
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
  ReadStrings(in, responseItems, false);

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
    const char* noDelete = getenv("MOZ_CRASHREPORTER_NO_DELETE_DUMP");
    if (!noDelete || *noDelete == '\0') {
      if (!gDumpFile.empty())
        UIDeleteFile(gDumpFile);
      if (!gExtraFile.empty())
        UIDeleteFile(gExtraFile);
    }

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
    if (!ReadStringsFromFile(gExtraFile, queryParameters, true)) {
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

    vector<string> restartArgs;

    ostringstream paramName;
    int i = 0;
    paramName << "MOZ_CRASHREPORTER_RESTART_ARG_" << i++;
    const char *param = getenv(paramName.str().c_str());
    while (param && *param) {
      restartArgs.push_back(param);

      paramName << "MOZ_CRASHREPORTER_RESTART_ARG_" << i++;
      param = getenv(paramName.str().c_str());
    };

    
    
    
    char* urlEnv = getenv("MOZ_CRASHREPORTER_URL");
    if (urlEnv && *urlEnv) {
      sendURL = urlEnv;
    }

    
    char buf[4096];
    UI_SNPRINTF(buf, sizeof(buf),
                gStrings[ST_RESTART].c_str(),
                product.c_str());
    gStrings[ST_RESTART] = buf;

    UI_SNPRINTF(buf, sizeof(buf),
                gStrings[ST_CRASHREPORTERDESCRIPTION].c_str(),
                product.c_str());
    gStrings[ST_CRASHREPORTERDESCRIPTION] = buf;

    UI_SNPRINTF(buf, sizeof(buf),
                gStrings[ST_CHECKSUBMIT].c_str(),
                vendor.empty() ? "Mozilla" : vendor.c_str());
    gStrings[ST_CHECKSUBMIT] = buf;

    UI_SNPRINTF(buf, sizeof(buf),
                gStrings[ST_CRASHREPORTERTITLE].c_str(),
                vendor.empty() ? "Mozilla" : vendor.c_str());
    gStrings[ST_CRASHREPORTERTITLE] = buf;

    UIShowCrashUI(gDumpFile, queryParameters, sendURL, restartArgs);
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
