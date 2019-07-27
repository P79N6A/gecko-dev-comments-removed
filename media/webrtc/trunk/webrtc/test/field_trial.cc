









#include "webrtc/test/field_trial.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include "webrtc/system_wrappers/interface/field_trial.h"

namespace webrtc {
namespace {



bool field_trials_initiated_ = false;
std::map<std::string, std::string> field_trials_;
}  

namespace field_trial {
std::string FindFullName(const std::string& trial_name) {
  assert(field_trials_initiated_);
  std::map<std::string, std::string>::const_iterator it =
      field_trials_.find(trial_name);
  if (it == field_trials_.end())
    return std::string();
  return it->second;
}
}  

namespace test {


void InitFieldTrialsFromString(const std::string& trials_string) {
  static const char kPersistentStringSeparator = '/';

  
  assert(field_trials_initiated_ == false);
  field_trials_initiated_ = true;

  if (trials_string.empty()) return;

  size_t next_item = 0;
  while (next_item < trials_string.length()) {
    size_t name_end = trials_string.find(kPersistentStringSeparator, next_item);
    if (name_end == trials_string.npos || next_item == name_end)
      break;
    size_t group_name_end = trials_string.find(kPersistentStringSeparator,
                                               name_end + 1);
    if (group_name_end == trials_string.npos || name_end + 1 == group_name_end)
      break;
    std::string name(trials_string, next_item, name_end - next_item);
    std::string group_name(trials_string, name_end + 1,
                           group_name_end - name_end - 1);
    next_item = group_name_end + 1;

    
    if (field_trials_.find(name) != field_trials_.end() &&
        field_trials_.find(name)->second != group_name)
      break;

    field_trials_[name] = group_name;

    
    if (next_item == trials_string.length())
      return;
  }
  
  fprintf(stderr, "Invalid field trials string.\n");

  
  abort();
}
}  
}  
