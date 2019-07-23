


































#include "processor/pathname_stripper.h"

namespace google_breakpad {


string PathnameStripper::File(const string &path) {
  string::size_type slash = path.rfind('/');
  string::size_type backslash = path.rfind('\\');

  string::size_type file_start = 0;
  if (slash != string::npos &&
      (backslash == string::npos || slash > backslash)) {
    file_start = slash + 1;
  } else if (backslash != string::npos) {
    file_start = backslash + 1;
  }

  return path.substr(file_start);
}

}  
