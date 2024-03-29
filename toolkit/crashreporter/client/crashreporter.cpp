




#include "crashreporter.h"

#ifdef _MSC_VER

# pragma warning( disable : 4530 )
#endif

#include <fstream>
#include <sstream>
#include <memory>
#include <time.h>
#include <stdlib.h>
#include <string.h>

using std::string;
using std::istream;
using std::ifstream;
using std::istringstream;
using std::ostringstream;
using std::ostream;
using std::ofstream;
using std::vector;
using std::auto_ptr;

namespace CrashReporter {

StringTable  gStrings;
string       gSettingsPath;
string       gEventsPath;
int          gArgc;
char**       gArgv;

enum SubmissionResult {Succeeded, Failed};

static auto_ptr<ofstream> gLogStream(nullptr);
static string             gReporterDumpFile;
static string             gExtraFile;
static string             gMemoryFile;

static const char kExtraDataExtension[] = ".extra";
static const char kMemoryReportExtension[] = ".memory.json.gz";

void UIError(const string& message)
{
  string errorMessage;
  if (!gStrings[ST_CRASHREPORTERERROR].empty()) {
    char buf[2048];
    UI_SNPRINTF(buf, 2048,
                gStrings[ST_CRASHREPORTERERROR].c_str(),
                message.c_str());
    errorMessage = buf;
  } else {
    errorMessage = message;
  }

  UIError_impl(errorMessage);
}

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

static string Escape(const string& str)
{
  string ret;
  for (string::const_iterator iter = str.begin();
       iter != str.end();
       iter++) {
    if (*iter == '\\') {
      ret += "\\\\";
    } else if (*iter == '\n') {
      ret += "\\n";
    } else if (*iter == '\t') {
      ret += "\\t";
    } else {
      ret.push_back(*iter);
    }
  }

  return ret;
}

bool ReadStrings(istream& in, StringTable& strings, bool unescape)
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

bool ReadStringsFromFile(const string& path,
                         StringTable& strings,
                         bool unescape)
{
  ifstream* f = UIOpenRead(path);
  bool success = false;
  if (f->is_open()) {
    success = ReadStrings(*f, strings, unescape);
    f->close();
  }

  delete f;
  return success;
}

bool WriteStrings(ostream& out,
                  const string& header,
                  StringTable& strings,
                  bool escape)
{
  out << "[" << header << "]" << std::endl;
  for (StringTable::iterator iter = strings.begin();
       iter != strings.end();
       iter++) {
    out << iter->first << "=";
    if (escape)
      out << Escape(iter->second);
    else
      out << iter->second;

    out << std::endl;
  }

  return true;
}

bool WriteStringsToFile(const string& path,
                        const string& header,
                        StringTable& strings,
                        bool escape)
{
  ofstream* f = UIOpenWrite(path.c_str());
  bool success = false;
  if (f->is_open()) {
    success = WriteStrings(*f, header, strings, escape);
    f->close();
  }

  delete f;
  return success;
}

static string Basename(const string& file)
{
  string::size_type slashIndex = file.rfind(UI_DIR_SEPARATOR);
  if (slashIndex != string::npos)
    return file.substr(slashIndex + 1);
  else
    return file;
}

static string GetDumpLocalID()
{
  string localId = Basename(gReporterDumpFile);
  string::size_type dot = localId.rfind('.');

  if (dot == string::npos)
    return "";

  return localId.substr(0, dot);
}

static void WriteSubmissionEvent(SubmissionResult result,
                                 const string& remoteId)
{
  if (gEventsPath.empty()) {
    
    return;
  }

  string localId = GetDumpLocalID();
  string fpath = gEventsPath + UI_DIR_SEPARATOR + localId + "-submission";
  ofstream* f = UIOpenWrite(fpath.c_str(), false, true);
  time_t tm;
  time(&tm);

  if (f->is_open()) {
    *f << "crash.submission.1\n";
    *f << tm << "\n";
    *f << localId << "\n";
    *f << (result == Succeeded ? "true" : "false") << "\n";
    *f << remoteId;

    f->close();
  }

  delete f;
}

void LogMessage(const std::string& message)
{
  if (gLogStream.get()) {
    char date[64];
    time_t tm;
    time(&tm);
    if (strftime(date, sizeof(date) - 1, "%c", localtime(&tm)) == 0)
        date[0] = '\0';
    (*gLogStream) << "[" << date << "] " << message << std::endl;
  }
}

static void OpenLogFile()
{
  string logPath = gSettingsPath + UI_DIR_SEPARATOR + "submit.log";
  gLogStream.reset(UIOpenWrite(logPath.c_str(), true));
}

static bool ReadConfig()
{
  string iniPath;
  if (!UIGetIniPath(iniPath))
    return false;

  if (!ReadStringsFromFile(iniPath, gStrings, true))
    return false;

  
  char* overrideEnv = getenv("MOZ_CRASHREPORTER_STRINGS_OVERRIDE");
  if (overrideEnv && *overrideEnv && UIFileExists(overrideEnv))
    ReadStringsFromFile(overrideEnv, gStrings, true);

  return true;
}

static string
GetAdditionalFilename(const string& dumpfile, const char* extension)
{
  string filename(dumpfile);
  int dot = filename.rfind('.');
  if (dot < 0)
    return "";

  filename.replace(dot, filename.length() - dot, extension);
  return filename;
}

static bool MoveCrashData(const string& toDir,
                          string& dumpfile,
                          string& extrafile,
                          string& memoryfile)
{
  if (!UIEnsurePathExists(toDir)) {
    UIError(gStrings[ST_ERROR_CREATEDUMPDIR]);
    return false;
  }

  string newDump = toDir + UI_DIR_SEPARATOR + Basename(dumpfile);
  string newExtra = toDir + UI_DIR_SEPARATOR + Basename(extrafile);
  string newMemory = toDir + UI_DIR_SEPARATOR + Basename(memoryfile);

  if (!UIMoveFile(dumpfile, newDump)) {
    UIError(gStrings[ST_ERROR_DUMPFILEMOVE]);
    return false;
  }

  if (!UIMoveFile(extrafile, newExtra)) {
    UIError(gStrings[ST_ERROR_EXTRAFILEMOVE]);
    return false;
  }

  if (!memoryfile.empty()) {
    
    if (!UIMoveFile(memoryfile, newMemory)) {
      UIDeleteFile(memoryfile);
      newMemory.erase();
    }
    memoryfile = newMemory;
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

  if (responseItems.find("StopSendingReportsFor") != responseItems.end()) {
    
    string reportPath =
      gSettingsPath + UI_DIR_SEPARATOR + "EndOfLife" +
      responseItems["StopSendingReportsFor"];

    ofstream* reportFile = UIOpenWrite(reportPath);
    if (reportFile->is_open()) {
      
      *reportFile << 1 << "\n";
      reportFile->close();
    }
    delete reportFile;
  }

  if (responseItems.find("Discarded") != responseItems.end()) {
    
    
    return false;
  }

  if (responseItems.find("CrashID") == responseItems.end())
    return false;

  string submittedDir =
    gSettingsPath + UI_DIR_SEPARATOR + "submitted";
  if (!UIEnsurePathExists(submittedDir)) {
    return false;
  }

  string path = submittedDir + UI_DIR_SEPARATOR +
    responseItems["CrashID"] + ".txt";

  ofstream* file = UIOpenWrite(path);
  if (!file->is_open()) {
    delete file;
    return false;
  }

  char buf[1024];
  UI_SNPRINTF(buf, 1024,
              gStrings["CrashID"].c_str(),
              responseItems["CrashID"].c_str());
  *file << buf << "\n";

  if (responseItems.find("ViewURL") != responseItems.end()) {
    UI_SNPRINTF(buf, 1024,
                gStrings["CrashDetailsURL"].c_str(),
                responseItems["ViewURL"].c_str());
    *file << buf << "\n";
  }

  file->close();
  delete file;

  WriteSubmissionEvent(Succeeded, responseItems["CrashID"]);
  return true;
}

void DeleteDump()
{
  const char* noDelete = getenv("MOZ_CRASHREPORTER_NO_DELETE_DUMP");
  if (!noDelete || *noDelete == '\0') {
    if (!gReporterDumpFile.empty())
      UIDeleteFile(gReporterDumpFile);
    if (!gExtraFile.empty())
      UIDeleteFile(gExtraFile);
    if (!gMemoryFile.empty())
      UIDeleteFile(gMemoryFile);
  }
}

void SendCompleted(bool success, const string& serverResponse)
{
  if (success) {
    if (AddSubmittedReport(serverResponse)) {
      DeleteDump();
    }
    else {
      string directory = gReporterDumpFile;
      int slashpos = directory.find_last_of("/\\");
      if (slashpos < 2)
        return;
      directory.resize(slashpos);
      UIPruneSavedDumps(directory);
      WriteSubmissionEvent(Failed, "");
    }
  } else {
    WriteSubmissionEvent(Failed, "");
  }
}

bool ShouldEnableSending()
{
  srand(time(0));
  return ((rand() % 100) < MOZ_CRASHREPORTER_ENABLE_PERCENT);
}

} 

using namespace CrashReporter;

void RewriteStrings(StringTable& queryParameters)
{
  
  string product = queryParameters["ProductName"];
  string vendor = queryParameters["Vendor"];
  if (vendor.empty()) {
    
    vendor = "Mozilla";
  }

  char buf[4096];
  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_CRASHREPORTERVENDORTITLE].c_str(),
              vendor.c_str());
  gStrings[ST_CRASHREPORTERTITLE] = buf;


  string str = gStrings[ST_CRASHREPORTERPRODUCTERROR];
  
  
  
  string::size_type pos = str.find("%s");
  if (pos != string::npos)
    pos = str.find("%s", pos+2);
  if (pos != string::npos) {
    
    UI_SNPRINTF(buf, sizeof(buf),
                gStrings[ST_CRASHREPORTERPRODUCTERROR].c_str(),
                product.c_str(),
                "%s");
    gStrings[ST_CRASHREPORTERERROR] = buf;
  }
  else {
    
    gStrings[ST_CRASHREPORTERERROR] = str;
  }

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_CRASHREPORTERDESCRIPTION].c_str(),
              product.c_str());
  gStrings[ST_CRASHREPORTERDESCRIPTION] = buf;

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_CHECKSUBMIT].c_str(),
              vendor.c_str());
  gStrings[ST_CHECKSUBMIT] = buf;

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_CHECKEMAIL].c_str(),
              vendor.c_str());
  gStrings[ST_CHECKEMAIL] = buf;

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_RESTART].c_str(),
              product.c_str());
  gStrings[ST_RESTART] = buf;

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_QUIT].c_str(),
              product.c_str());
  gStrings[ST_QUIT] = buf;

  UI_SNPRINTF(buf, sizeof(buf),
              gStrings[ST_ERROR_ENDOFLIFE].c_str(),
              product.c_str());
  gStrings[ST_ERROR_ENDOFLIFE] = buf;
}

bool CheckEndOfLifed(string version)
{
  string reportPath =
    gSettingsPath + UI_DIR_SEPARATOR + "EndOfLife" + version;
  return UIFileExists(reportPath);
}

int main(int argc, char** argv)
{
  gArgc = argc;
  gArgv = argv;

  if (!ReadConfig()) {
    UIError("Couldn't read configuration.");
    return 0;
  }

  if (!UIInit())
    return 0;

  if (argc > 1) {
    gReporterDumpFile = argv[1];
  }

  if (gReporterDumpFile.empty()) {
    
    UIShowDefaultUI();
  } else {
    gExtraFile = GetAdditionalFilename(gReporterDumpFile, kExtraDataExtension);
    if (gExtraFile.empty()) {
      UIError(gStrings[ST_ERROR_BADARGUMENTS]);
      return 0;
    }

    if (!UIFileExists(gExtraFile)) {
      UIError(gStrings[ST_ERROR_EXTRAFILEEXISTS]);
      return 0;
    }

    gMemoryFile = GetAdditionalFilename(gReporterDumpFile,
                                        kMemoryReportExtension);
    if (!UIFileExists(gMemoryFile)) {
      gMemoryFile.erase();
    }

    StringTable queryParameters;
    if (!ReadStringsFromFile(gExtraFile, queryParameters, true)) {
      UIError(gStrings[ST_ERROR_EXTRAFILEREAD]);
      return 0;
    }

    if (queryParameters.find("ProductName") == queryParameters.end()) {
      UIError(gStrings[ST_ERROR_NOPRODUCTNAME]);
      return 0;
    }

    
    
    RewriteStrings(queryParameters);

    if (queryParameters.find("ServerURL") == queryParameters.end()) {
      UIError(gStrings[ST_ERROR_NOSERVERURL]);
      return 0;
    }

    
    
#ifdef XP_WIN32
    static const wchar_t kDataDirKey[] = L"MOZ_CRASHREPORTER_DATA_DIRECTORY";
    const wchar_t *settingsPath = _wgetenv(kDataDirKey);
    if (settingsPath && *settingsPath) {
      gSettingsPath = WideToUTF8(settingsPath);
    }
#else
    static const char kDataDirKey[] = "MOZ_CRASHREPORTER_DATA_DIRECTORY";
    const char *settingsPath = getenv(kDataDirKey);
    if (settingsPath && *settingsPath) {
      gSettingsPath = settingsPath;
    }
#endif
    else {
      string product = queryParameters["ProductName"];
      string vendor = queryParameters["Vendor"];
      if (!UIGetSettingsPath(vendor, product, gSettingsPath)) {
        gSettingsPath.clear();
      }
    }

    if (gSettingsPath.empty() || !UIEnsurePathExists(gSettingsPath)) {
      UIError(gStrings[ST_ERROR_NOSETTINGSPATH]);
      return 0;
    }

    OpenLogFile();

#ifdef XP_WIN32
    static const wchar_t kEventsDirKey[] = L"MOZ_CRASHREPORTER_EVENTS_DIRECTORY";
    const wchar_t *eventsPath = _wgetenv(kEventsDirKey);
    if (eventsPath && *eventsPath) {
      gEventsPath = WideToUTF8(eventsPath);
    }
#else
    static const char kEventsDirKey[] = "MOZ_CRASHREPORTER_EVENTS_DIRECTORY";
    const char *eventsPath = getenv(kEventsDirKey);
    if (eventsPath && *eventsPath) {
      gEventsPath = eventsPath;
    }
#endif
    else {
      gEventsPath.clear();
    }

    if (!UIFileExists(gReporterDumpFile)) {
      UIError(gStrings[ST_ERROR_DUMPFILEEXISTS]);
      return 0;
    }

    string pendingDir = gSettingsPath + UI_DIR_SEPARATOR + "pending";
    if (!MoveCrashData(pendingDir, gReporterDumpFile, gExtraFile,
                       gMemoryFile)) {
      return 0;
    }

    string sendURL = queryParameters["ServerURL"];
    
    queryParameters.erase("ServerURL");

    queryParameters["Throttleable"] = "1";

    
    const char *appfile = getenv("MOZ_CRASHREPORTER_RESTART_XUL_APP_FILE");
    if (appfile && *appfile) {
      const char prefix[] = "XUL_APP_FILE=";
      char *env = (char*) malloc(strlen(appfile) + strlen(prefix) + 1);
      if (!env) {
        UIError("Out of memory");
        return 0;
      }
      strcpy(env, prefix);
      strcat(env, appfile);
      putenv(env);
      free(env);
    }

    vector<string> restartArgs;

    ostringstream paramName;
    int i = 0;
    paramName << "MOZ_CRASHREPORTER_RESTART_ARG_" << i++;
    const char *param = getenv(paramName.str().c_str());
    while (param && *param) {
      restartArgs.push_back(param);

      paramName.str("");
      paramName << "MOZ_CRASHREPORTER_RESTART_ARG_" << i++;
      param = getenv(paramName.str().c_str());
    };

    
    
    
    char* urlEnv = getenv("MOZ_CRASHREPORTER_URL");
    if (urlEnv && *urlEnv) {
      sendURL = urlEnv;
    }

     
     if (queryParameters.find("Version") != queryParameters.end() &&
         CheckEndOfLifed(queryParameters["Version"])) {
       UIError(gStrings[ST_ERROR_ENDOFLIFE]);
       DeleteDump();
       return 0;
     }

    StringTable files;
    files["upload_file_minidump"] = gReporterDumpFile;
    if (!gMemoryFile.empty()) {
      files["memory_report"] = gMemoryFile;
    }

    if (!UIShowCrashUI(files, queryParameters, sendURL, restartArgs))
      DeleteDump();
  }

  UIShutdown();

  return 0;
}

#if defined(XP_WIN) && !defined(__GNUC__)
#include <windows.h>



int WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR args, int )
{
  
  {
    HKEY hkApp;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Applications", 0,
                    nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                    &hkApp, nullptr);
    RegCloseKey(hkApp);
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Classes\\Applications\\crashreporter.exe",
                        0, nullptr, REG_OPTION_VOLATILE, KEY_SET_VALUE,
                        nullptr, &hkApp, nullptr) == ERROR_SUCCESS) {
      RegSetValueExW(hkApp, L"IsHostApp", 0, REG_NONE, 0, 0);
      RegSetValueExW(hkApp, L"NoOpenWith", 0, REG_NONE, 0, 0);
      RegSetValueExW(hkApp, L"NoStartPage", 0, REG_NONE, 0, 0);
      RegCloseKey(hkApp);
    }
  }

  char** argv = static_cast<char**>(malloc(__argc * sizeof(char*)));
  for (int i = 0; i < __argc; i++) {
    argv[i] = strdup(WideToUTF8(__wargv[i]).c_str());
  }

  
  return main(__argc, argv);
}
#endif
