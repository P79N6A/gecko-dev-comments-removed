



#include "base/logging.h"



extern "C"
void bz_internal_error(int errcode) {
  CHECK(false) << "bzip2 internal error: " << errcode;
}
