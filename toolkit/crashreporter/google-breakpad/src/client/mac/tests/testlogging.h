

#include <sstream>

namespace google_breakpad {
extern std::ostringstream info_log;
}

#define BPLOG_INFO_STREAM google_breakpad::info_log
