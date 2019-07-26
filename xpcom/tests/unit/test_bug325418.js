const Cc = Components.classes;
const Ci = Components.interfaces;


const kExpectedDelay1 = 5;

const kExpectedDelay2 = 1;

var gStartTime1;
var gStartTime2;
var timer;

var observer1 = {
  observe: function observeTC1(subject, topic, data) {
    if (topic == "timer-callback") {
      
      timer.cancel();

      
      do_check_eq(Math.round((Date.now() - gStartTime1) / 1000),
                  kExpectedDelay1);

      timer = null;

      do_print("1st timer triggered (before being cancelled). Should not have happened!");
      do_check_true(false);
    }
  }
};

var observer2 = {
  observe: function observeTC2(subject, topic, data) {
    if (topic == "timer-callback") {
      
      timer.cancel();

      
      do_check_eq(Math.round((Date.now() - gStartTime2) / 1000),
                  kExpectedDelay2);

      timer = null;

      do_test_finished();
    }
  }
};

function run_test() {
  do_test_pending();

  timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

  
  gStartTime1 = Date.now();
  timer.init(observer1, kExpectedDelay1 * 1000, timer.TYPE_REPEATING_PRECISE);
  timer.cancel();

  
  gStartTime2 = Date.now();
  timer.init(observer2, kExpectedDelay2 * 1000, timer.TYPE_REPEATING_PRECISE);
}
