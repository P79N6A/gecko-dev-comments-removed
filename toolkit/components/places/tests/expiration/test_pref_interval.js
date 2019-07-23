
















































const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";
const MAX_WAIT_SECONDS = 4;
const INTERVAL_CUSHION = 2;

let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

let gTests = [

  
  
  { desc: "Set interval to 1s.",
    interval: 1,
    expectedNotification: true,
  },

  { desc: "Set interval to a negative value.",
    interval: -1,
    expectedNotification: false, 
  },

  { desc: "Set interval to 0.",
    interval: 0,
    expectedNotification: false, 
  },

  { desc: "Set interval to a large value.",
    interval: 100,
    expectedNotification: false, 
  },
];

let gCurrentTest;

function run_test() {
  
  try {
    getInterval();
    do_throw("interval pref should not exist by default");
  }
  catch (ex) {}

  
  force_expiration_start();

  run_next_test();
  do_test_pending();
}

function run_next_test() {
  if (gTests.length) {
    gCurrentTest = gTests.shift();
    print(gCurrentTest.desc);
    gCurrentTest.receivedNotification = false;
    gCurrentTest.observer = {
      observe: function(aSubject, aTopic, aData) {
        gCurrentTest.receivedNotification = true;
      }
    };
    os.addObserver(gCurrentTest.observer, TOPIC_EXPIRATION_FINISHED, false);
    setInterval(gCurrentTest.interval);
    let waitSeconds = Math.min(MAX_WAIT_SECONDS,
                               gCurrentTest.interval + INTERVAL_CUSHION);
    do_timeout(waitSeconds * 1000, check_result);
  }
  else {
    clearInterval();
    do_test_finished();
  }
}

function check_result() {
  os.removeObserver(gCurrentTest.observer, TOPIC_EXPIRATION_FINISHED);

  do_check_eq(gCurrentTest.receivedNotification,
              gCurrentTest.expectedNotification);

  run_next_test();
}
