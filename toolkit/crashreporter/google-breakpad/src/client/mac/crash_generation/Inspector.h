































#import "common/mac/SimpleStringDictionary.h"

#import <Foundation/Foundation.h>
#include <mach/mach.h>

#import "client/mac/crash_generation/ConfigFile.h"
#import "client/mac/handler/minidump_generator.h"

extern bool gDebugLog;

#define DEBUGLOG if (gDebugLog) fprintf


enum {
  kMsgType_InspectorInitialInfo = 0,    
  kMsgType_InspectorKeyValuePair = 1,   
  kMsgType_InspectorAcknowledgement = 2 
};






struct InspectorInfo {
  int           exception_type;
  int           exception_code;
  int           exception_subcode;
  unsigned int  parameter_count;  
};


struct KeyValueMessageData {
 public:
  KeyValueMessageData() {}
  KeyValueMessageData(const google_breakpad::KeyValueEntry &inEntry) {
    strlcpy(key, inEntry.GetKey(), sizeof(key) );
    strlcpy(value, inEntry.GetValue(), sizeof(value) );
  }

  char key[google_breakpad::KeyValueEntry::MAX_STRING_STORAGE_SIZE];
  char value[google_breakpad::KeyValueEntry::MAX_STRING_STORAGE_SIZE];
};

using std::string;
using google_breakpad::MinidumpGenerator;

namespace google_breakpad {


class MinidumpLocation {
 public:
  MinidumpLocation(NSString *minidumpDir) {
    
    assert(minidumpDir);
    if (!EnsureDirectoryPathExists(minidumpDir)) {
      DEBUGLOG(stderr, "Unable to create: %s\n", [minidumpDir UTF8String]);
      minidumpDir = @"/tmp";
    }

    strlcpy(minidump_dir_path_, [minidumpDir fileSystemRepresentation],
            sizeof(minidump_dir_path_));

    
    string dump_path(minidump_dir_path_);
    string next_minidump_id;

    string next_minidump_path_ =
      (MinidumpGenerator::UniqueNameInDirectory(dump_path, &next_minidump_id));

    strlcpy(minidump_id_, next_minidump_id.c_str(), sizeof(minidump_id_));
  };

  const char *GetPath() { return minidump_dir_path_; }
  const char *GetID() { return minidump_id_; }

 private:
  char minidump_dir_path_[PATH_MAX];             
  char minidump_id_[128];
};


class Inspector {
 public:
  Inspector() {};

  
  
  
  void            Inspect(const char *receive_port_name);

 private:
  
  
  
  
  
  
  
  
  
  
  kern_return_t   ResetBootstrapPort();

  kern_return_t   ServiceCheckIn(const char *receive_port_name);
  kern_return_t   ServiceCheckOut(const char *receive_port_name);

  kern_return_t   ReadMessages();

  bool            InspectTask();
  kern_return_t   SendAcknowledgement();
  void            LaunchReporter(const char *inConfigFilePath);

  
  
  mach_port_t     bootstrap_subset_port_;

  mach_port_t     service_rcv_port_;

  int             exception_type_;
  int             exception_code_;
  int             exception_subcode_;
  mach_port_t     remote_task_;
  mach_port_t     crashing_thread_;
  mach_port_t     handler_thread_;
  mach_port_t     ack_port_;

  SimpleStringDictionary config_params_;

  ConfigFile      config_file_;
};


} 
