



#include "chrome/common/chrome_counters.h"

#include "base/stats_counters.h"

namespace chrome {












StatsCounter& Counters::ipc_send_counter() {
  static StatsCounter* ctr = new StatsCounter("IPC.SendMsgCount");
  return *ctr;
}

StatsCounterTimer& Counters::chrome_main() {
  static StatsCounterTimer* ctr = new StatsCounterTimer("Chrome.Init");
  return *ctr;
}

StatsCounterTimer& Counters::renderer_main() {
  static StatsCounterTimer* ctr = new StatsCounterTimer("Chrome.RendererInit");
  return *ctr;
}

StatsCounterTimer& Counters::spellcheck_init() {
  static StatsCounterTimer* ctr = new StatsCounterTimer("SpellCheck.Init");
  return *ctr;
}

StatsRate& Counters::spellcheck_lookup() {
  static StatsRate* ctr = new StatsRate("SpellCheck.Lookup");
  return *ctr;
}

StatsCounterTimer& Counters::plugin_load() {
  static StatsCounterTimer* ctr = new StatsCounterTimer("ChromePlugin.Load");
  return *ctr;
}

StatsRate& Counters::plugin_intercept() {
  static StatsRate* ctr = new StatsRate("ChromePlugin.Intercept");
  return *ctr;
}

}  
