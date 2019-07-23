



#ifndef CHROME_COMMON_LOGGING_CHROME_H__
#define CHROME_COMMON_LOGGING_CHROME_H__

#include <string>
#include <vector>

#include "base/logging.h"

class CommandLine;

namespace logging {














void InitChromeLogging(const CommandLine& command_line,
                       OldFileDeletionState delete_old_log_file);


void CleanupChromeLogging();


std::wstring GetLogFileName();



bool DialogsAreSuppressed();

typedef std::vector<std::wstring> AssertionList;









size_t GetFatalAssertions(AssertionList* assertions);

} 

#endif
