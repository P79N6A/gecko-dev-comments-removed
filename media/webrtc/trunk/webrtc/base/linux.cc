









#if defined(WEBRTC_LINUX)
#include "webrtc/base/linux.h"

#include <ctype.h>

#include <errno.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <cstdio>
#include <set>

#include "webrtc/base/stringencode.h"

namespace rtc {

static const char kCpuInfoFile[] = "/proc/cpuinfo";
static const char kCpuMaxFreqFile[] =
    "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";

ProcCpuInfo::ProcCpuInfo() {
}

ProcCpuInfo::~ProcCpuInfo() {
}

bool ProcCpuInfo::LoadFromSystem() {
  ConfigParser procfs;
  if (!procfs.Open(kCpuInfoFile)) {
    return false;
  }
  return procfs.Parse(&sections_);
};

bool ProcCpuInfo::GetSectionCount(size_t* count) {
  if (sections_.empty()) {
    return false;
  }
  if (count) {
    *count = sections_.size();
  }
  return true;
}

bool ProcCpuInfo::GetNumCpus(int* num) {
  if (sections_.empty()) {
    return false;
  }
  int total_cpus = 0;
#if defined(__arm__)
  
  
  size_t section_count = sections_.size();
  for (size_t i = 0; i < section_count; ++i) {
    int processor_id;
    if (GetSectionIntValue(i, "processor", &processor_id)) {
      ++total_cpus;
    }
  }
  
  
  if (total_cpus == 0) {
    total_cpus = 1;
  }
#else
  
  total_cpus = static_cast<int>(sections_.size());
#endif
  if (num) {
    *num = total_cpus;
  }
  return true;
}

bool ProcCpuInfo::GetNumPhysicalCpus(int* num) {
  if (sections_.empty()) {
    return false;
  }
  
  
#if defined(__arm__)
  
  
  return GetNumCpus(num);
#else
  int total_cores = 0;
  std::set<int> physical_ids;
  size_t section_count = sections_.size();
  for (size_t i = 0; i < section_count; ++i) {
    int physical_id;
    int cores;
    
    if (GetSectionIntValue(i, "physical id", &physical_id) &&
        GetSectionIntValue(i, "cpu cores", &cores) &&
        physical_ids.find(physical_id) == physical_ids.end()) {
      physical_ids.insert(physical_id);
      total_cores += cores;
    }
  }

  if (num) {
    *num = total_cores;
  }
  return true;
#endif
}

bool ProcCpuInfo::GetCpuFamily(int* id) {
  int cpu_family = 0;

#if defined(__arm__)
  
  
  
  
  
  
  size_t section_count = sections_.size();
  for (size_t i = 0; i < section_count; ++i) {
    if (GetSectionIntValue(i, "CPU architecture", &cpu_family)) {
      
      break;
    };
  }
#else
  GetSectionIntValue(0, "cpu family", &cpu_family);
#endif
  if (id) {
    *id = cpu_family;
  }
  return true;
}

bool ProcCpuInfo::GetSectionStringValue(size_t section_num,
                                        const std::string& key,
                                        std::string* result) {
  if (section_num >= sections_.size()) {
    return false;
  }
  ConfigParser::SimpleMap::iterator iter = sections_[section_num].find(key);
  if (iter == sections_[section_num].end()) {
    return false;
  }
  *result = iter->second;
  return true;
}

bool ProcCpuInfo::GetSectionIntValue(size_t section_num,
                                     const std::string& key,
                                     int* result) {
  if (section_num >= sections_.size()) {
    return false;
  }
  ConfigParser::SimpleMap::iterator iter = sections_[section_num].find(key);
  if (iter == sections_[section_num].end()) {
    return false;
  }
  return FromString(iter->second, result);
}

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

bool ConfigParser::Open(const std::string& filename) {
  FileStream* fs = new FileStream();
  if (!fs->Open(filename, "r", NULL)) {
    return false;
  }
  instream_.reset(fs);
  return true;
}

void ConfigParser::Attach(StreamInterface* stream) {
  instream_.reset(stream);
}

bool ConfigParser::Parse(MapVector* key_val_pairs) {
  
  SimpleMap section;
  while (ParseSection(&section)) {
    key_val_pairs->push_back(section);
    section.clear();
  }
  return (!key_val_pairs->empty());
}

bool ConfigParser::ParseSection(SimpleMap* key_val_pair) {
  
  
  std::string key, value;
  while (ParseLine(&key, &value)) {
    (*key_val_pair)[key] = value;
  }
  return (!key_val_pair->empty());
}

bool ConfigParser::ParseLine(std::string* key, std::string* value) {
  
  
  std::string line;
  if ((instream_->ReadLine(&line)) == SR_EOS) {
    return false;
  }
  std::vector<std::string> tokens;
  if (2 != split(line, ':', &tokens)) {
    return false;
  }
  
  size_t pos = tokens[0].length() - 1;
  while ((pos > 0) && isspace(tokens[0][pos])) {
    pos--;
  }
  tokens[0].erase(pos + 1);
  
  pos = 0;
  while (pos < tokens[1].length() && isspace(tokens[1][pos])) {
    pos++;
  }
  tokens[1].erase(0, pos);
  *key = tokens[0];
  *value = tokens[1];
  return true;
}

#if !defined(WEBRTC_CHROMIUM_BUILD)
static bool ExpectLineFromStream(FileStream* stream,
                                 std::string* out) {
  StreamResult res = stream->ReadLine(out);
  if (res != SR_SUCCESS) {
    if (res != SR_EOS) {
      LOG(LS_ERROR) << "Error when reading from stream";
    } else {
      LOG(LS_ERROR) << "Incorrect number of lines in stream";
    }
    return false;
  }
  return true;
}

static void ExpectEofFromStream(FileStream* stream) {
  std::string unused;
  StreamResult res = stream->ReadLine(&unused);
  if (res == SR_SUCCESS) {
    LOG(LS_WARNING) << "Ignoring unexpected extra lines from stream";
  } else if (res != SR_EOS) {
    LOG(LS_WARNING) << "Error when checking for extra lines from stream";
  }
}



static std::string lsb_release_string;
static CriticalSection lsb_release_string_critsec;

std::string ReadLinuxLsbRelease() {
  CritScope cs(&lsb_release_string_critsec);
  if (!lsb_release_string.empty()) {
    
    return lsb_release_string;
  }
  
  POpenStream lsb_release_output;
  if (!lsb_release_output.Open("lsb_release -idrcs", "r", NULL)) {
    LOG_ERR(LS_ERROR) << "Can't run lsb_release";
    return lsb_release_string;  
  }
  
  std::ostringstream sstr;
  std::string line;
  int wait_status;

  if (!ExpectLineFromStream(&lsb_release_output, &line)) {
    return lsb_release_string;  
  }
  sstr << "DISTRIB_ID=" << line;

  if (!ExpectLineFromStream(&lsb_release_output, &line)) {
    return lsb_release_string;  
  }
  sstr << " DISTRIB_DESCRIPTION=\"" << line << '"';

  if (!ExpectLineFromStream(&lsb_release_output, &line)) {
    return lsb_release_string;  
  }
  sstr << " DISTRIB_RELEASE=" << line;

  if (!ExpectLineFromStream(&lsb_release_output, &line)) {
    return lsb_release_string;  
  }
  sstr << " DISTRIB_CODENAME=" << line;

  
  ExpectEofFromStream(&lsb_release_output);

  lsb_release_output.Close();
  wait_status = lsb_release_output.GetWaitStatus();
  if (wait_status == -1 ||
      !WIFEXITED(wait_status) ||
      WEXITSTATUS(wait_status) != 0) {
    LOG(LS_WARNING) << "Unexpected exit status from lsb_release";
  }

  lsb_release_string = sstr.str();

  return lsb_release_string;
}
#endif

std::string ReadLinuxUname() {
  struct utsname buf;
  if (uname(&buf) < 0) {
    LOG_ERR(LS_ERROR) << "Can't call uname()";
    return std::string();
  }
  std::ostringstream sstr;
  sstr << buf.sysname << " "
       << buf.release << " "
       << buf.version << " "
       << buf.machine;
  return sstr.str();
}

int ReadCpuMaxFreq() {
  FileStream fs;
  std::string str;
  int freq = -1;
  if (!fs.Open(kCpuMaxFreqFile, "r", NULL) ||
      SR_SUCCESS != fs.ReadLine(&str) ||
      !FromString(str, &freq)) {
    return -1;
  }
  return freq;
}

}  

#endif  
