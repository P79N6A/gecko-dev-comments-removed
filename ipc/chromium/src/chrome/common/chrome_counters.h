





#ifndef CHROME_COMMON_CHROME_COUNTERS_H_
#define CHROME_COMMON_CHROME_COUNTERS_H_

class StatsCounter;
class StatsCounterTimer;
class StatsRate;

namespace chrome {

class Counters {
 public:
  
  static StatsCounter& ipc_send_counter();

  
  static StatsCounterTimer& chrome_main();

  
  static StatsCounterTimer& renderer_main();

  
  static StatsCounterTimer& spellcheck_init();

  
  static StatsRate& spellcheck_lookup();

  
  static StatsCounterTimer& plugin_load();

  
  static StatsRate& plugin_intercept();
};

}  

#endif  
