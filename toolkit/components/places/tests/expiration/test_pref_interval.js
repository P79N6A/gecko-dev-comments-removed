

















const DEFAULT_TIMER_DELAY_SECONDS = 3 * 60;


const EXPIRE_AGGRESSIVITY_MULTIPLIER = 3;

Cu.import("resource://testing-common/MockRegistrar.jsm");


const TIMER_CONTRACT_ID = "@mozilla.org/timer;1";
let mockCID;

let mockTimerImpl = {
  initWithCallback: function MTI_initWithCallback(aCallback, aDelay, aType) {
    print("Checking timer delay equals expected interval value");
    if (!currentTest)
      return;
    
    do_check_eq(aDelay, currentTest.expectedTimerDelay * 1000 * EXPIRE_AGGRESSIVITY_MULTIPLIER)

    do_execute_soon(runNextTest);
  },

  cancel: function() {},
  initWithFuncCallback: function() {},
  init: function() {},

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITimer,
  ])
}

function replace_timer_factory() {
  mockCID = MockRegistrar.register(TIMER_CONTRACT_ID, mockTimerImpl);
}

do_register_cleanup(function() {
  
  
  shutdownExpiration();

  
  MockRegistrar.unregister(mockCID);
});


let tests = [

  
  
  { desc: "Set interval to 1s.",
    interval: 1,
    expectedTimerDelay: 1
  },

  { desc: "Set interval to a negative value.",
    interval: -1,
    expectedTimerDelay: DEFAULT_TIMER_DELAY_SECONDS
  },

  { desc: "Set interval to 0.",
    interval: 0,
    expectedTimerDelay: DEFAULT_TIMER_DELAY_SECONDS
  },

  { desc: "Set interval to a large value.",
    interval: 100,
    expectedTimerDelay: 100
  },

];

let currentTest;

function run_test() {
  
  try {
    getInterval();
    do_throw("interval pref should not exist by default");
  }
  catch (ex) {}

  
  replace_timer_factory();

  
  force_expiration_start();

  runNextTest();
  do_test_pending();
}

function runNextTest() {
  if (tests.length) {
    currentTest = tests.shift();
    print(currentTest.desc);
    setInterval(currentTest.interval);
  }
  else {
    clearInterval();
    do_test_finished();
  }
}
