
































#include "utilities.h"

extern "C" {
#define UNW_LOCAL_ONLY
#include <libunwind.h>
}
#include "glog/raw_logging.h"
#include "stacktrace.h"

_START_GOOGLE_NAMESPACE_








static bool g_now_entering = false;


int GetStackTrace(void** result, int max_depth, int skip_count) {
  void *ip;
  int n = 0;
  unw_cursor_t cursor;
  unw_context_t uc;

  if (sync_val_compare_and_swap(&g_now_entering, false, true)) {
    return 0;
  }

  unw_getcontext(&uc);
  RAW_CHECK(unw_init_local(&cursor, &uc) >= 0, "unw_init_local failed");
  skip_count++;         

  while (n < max_depth) {
    int ret = unw_get_reg(&cursor, UNW_REG_IP, (unw_word_t *) &ip);
    if (ret < 0)
      break;
    if (skip_count > 0) {
      skip_count--;
    } else {
      result[n++] = ip;
    }
    ret = unw_step(&cursor);
    if (ret <= 0)
      break;
  }

  g_now_entering = false;
  return n;
}

_END_GOOGLE_NAMESPACE_
