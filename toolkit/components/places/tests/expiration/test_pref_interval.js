


















































const DEFAULT_TIMER_DELAY_SECONDS = 3 * 60;


const EXPIRE_AGGRESSIVITY_MULTIPLIER = 3;



const Cm = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
const TIMER_CONTRACT_ID = "@mozilla.org/timer;1";
const kMockCID = Components.ID("52bdf457-4de3-48ae-bbbb-f3cbcbcad05f");


let gOriginalCID = Cm.contractIDToCID(TIMER_CONTRACT_ID);


let gMockTimerFactory = {
  createInstance: function MTF_createInstance(aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return mockTimerImpl.QueryInterface(aIID);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFactory,
  ])
}

let mockTimerImpl = {
  initWithCallback: function MTI_initWithCallback(aCallback, aDelay, aType) {
    print("Checking timer delay equals expected interval value");
    if (!gCurrentTest)
      return;
    
    do_check_eq(aDelay, gCurrentTest.expectedTimerDelay * 1000 * EXPIRE_AGGRESSIVITY_MULTIPLIER)

    do_execute_soon(run_next_test);
  },

  cancel: function() {},
  initWithFuncCallback: function() {},
  init: function() {},

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITimer,
  ])
}

function replace_timer_factory() {
  Cm.registerFactory(kMockCID,
                     "Mock timer",
                     TIMER_CONTRACT_ID,
                     gMockTimerFactory);
}

do_register_cleanup(function() {
  
  
  shutdownExpiration();

  
  Cm.unregisterFactory(kMockCID,
                       gMockTimerFactory);
  Cm.registerFactory(gOriginalCID,
                     "",
                     TIMER_CONTRACT_ID,
                     null);
});


let gTests = [

  
  
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

let gCurrentTest;

function run_test() {
  
  try {
    getInterval();
    do_throw("interval pref should not exist by default");
  }
  catch (ex) {}

  
  replace_timer_factory();

  
  force_expiration_start();

  run_next_test();
  do_test_pending();
}

function run_next_test() {
  if (gTests.length) {
    gCurrentTest = gTests.shift();
    print(gCurrentTest.desc);
    setInterval(gCurrentTest.interval);
  }
  else {
    clearInterval();
    do_test_finished();
  }
}
