


XPCOMUtils.defineLazyModuleGetter(this, 'setTimeout',
  'resource://gre/modules/Timer.jsm');


Services.prefs.setCharPref("readinglist.log.appender.dump", "Trace");

let {createTestableScheduler} = Cu.import("resource:///modules/readinglist/Scheduler.jsm", {});
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Timer.jsm");


do_get_profile();

let prefs = new Preferences("readinglist.scheduler.");
prefs.set("enabled", true);

function promiseObserver(topic) {
  return new Promise(resolve => {
    let obs = (subject, topic, data) => {
      Services.obs.removeObserver(obs, topic);
      resolve(data);
    }
    Services.obs.addObserver(obs, topic, false);
  });
}

function ReadingListMock() {
  this.listener = null;
}

ReadingListMock.prototype = {
  addListener(listener) {
    ok(!this.listener, "mock only expects 1 listener");
    this.listener = listener;
  },
}

function createScheduler(options) {
  
  let allowedOptions = ["expectedDelay", "expectNewTimer", "syncFunction"];
  for (let key of Object.keys(options)) {
    if (allowedOptions.indexOf(key) == -1) {
      throw new Error("Invalid option " + key);
    }
  }
  let rlMock = new ReadingListMock();
  let scheduler = createTestableScheduler(rlMock);
  
  let syncFunction = options.syncFunction || Promise.resolve;
  scheduler._engine.start = syncFunction;
  
  
  
  let numCalls = 0;
  scheduler._setTimeout = function(delay) {
    ++numCalls;
    print("Test scheduler _setTimeout call number " + numCalls + " with delay=" + delay);
    switch (numCalls) {
      case 1:
        
        
        return setTimeout(() => scheduler._doSync(), 0);
        break;
      case 2:
        
        if (options.expectedDelay) {
          
          ok(Math.abs(options.expectedDelay * 1000 - delay) < 500, [options.expectedDelay * 1000, delay]);
        }
        
        return setTimeout(() => scheduler._doSync(), 10000000);
        break;
      default:
        
        ok(false, numCalls);
    }
  };
  
  
  scheduler._onAutoReschedule = () => {
    
    let expectNewTimer = options.expectNewTimer === undefined ? true : options.expectNewTimer;
    ok(expectNewTimer ? scheduler._timer : !scheduler._timer);
  }
  
  scheduler.init();
  return scheduler;
}

add_task(function* testSuccess() {
  
  let allNotifications = [
    promiseObserver("readinglist:sync:start"),
    promiseObserver("readinglist:sync:finish"),
  ];
  
  prefs.set("schedule", 100);
  let scheduler = createScheduler({expectedDelay: 100});
  yield Promise.all(allNotifications);
  scheduler.finalize();
});



add_task(function* testImmediateResyncWhenChangedDuringSync() {
  
  let allNotifications = [
    promiseObserver("readinglist:sync:start"),
    promiseObserver("readinglist:sync:finish"),
  ];
  prefs.set("schedule", 100);
  
  let scheduler = createScheduler({
    expectedDelay: 0,
    syncFunction: () => {
      
      scheduler.readingList.listener.onItemAdded();
      return Promise.resolve();
    }});
  yield Promise.all(allNotifications);
  scheduler.finalize();
});

add_task(function* testOffline() {
  let scheduler = createScheduler({expectNewTimer: false});
  Services.io.offline = true;
  ok(!scheduler._canSync(), "_canSync is false when offline.")
  ok(!scheduler._timer, "there is no current timer while offline.")
  Services.io.offline = false;
  ok(scheduler._canSync(), "_canSync is true when online.")
  ok(scheduler._timer, "there is a new timer when back online.")
  scheduler.finalize();
});

add_task(function* testRetryableError() {
  let allNotifications = [
    promiseObserver("readinglist:sync:start"),
    promiseObserver("readinglist:sync:error"),
  ];
  prefs.set("retry", 10);
  let scheduler = createScheduler({
    expectedDelay: 10,
    syncFunction: () => Promise.reject("transient"),
  });
  yield Promise.all(allNotifications);
  scheduler.finalize();
});

add_task(function* testAuthError() {
  prefs.set("retry", 10);
  
  
  
  
  let scheduler = createScheduler({
    expectedDelay: 10,
    syncFunction:  () => {
      return Promise.reject(ReadingListScheduler._engine.ERROR_AUTHENTICATION);
    },
    expectNewTimer: false
  });
  
  
  scheduler.finalize();
});

add_task(function* testBackoff() {
  let scheduler = createScheduler({expectedDelay: 1000});
  Services.obs.notifyObservers(null, "readinglist:backoff-requested", 1000);
  
  
  scheduler.finalize();
});

add_task(function testErrorBackoff() {
  
  
  let rlMock = new ReadingListMock();
  let scheduler = createTestableScheduler(rlMock);
  scheduler._setTimeout = function(delay) {
    
    return setTimeout(() => scheduler._doSync(), 0);
  }

  
  function checkBackoffs(expectedSequences) {
    let orig_maybeReschedule = scheduler._maybeReschedule;
    return new Promise(resolve => {
      let isSuccess = true; 
      let expected;
      function nextSequence() {
        if (expectedSequences.length == 0) {
          resolve();
          return true; 
        }
        
        expected = expectedSequences.shift()
        
        isSuccess = !isSuccess;
        if (isSuccess) {
          scheduler._engine.start = Promise.resolve;
        } else {
          scheduler._engine.start = () => {
            return Promise.reject(new Error("oh no"))
          }
        }
        return false; 
      };
      
      nextSequence();
      
      scheduler._maybeReschedule = function(nextDelay) {
        let thisExpected = expected.shift();
        equal(thisExpected * 1000, nextDelay);
        if (expected.length == 0) {
          if (nextSequence()) {
            
            return;
          }
        }
        
        return orig_maybeReschedule.call(scheduler, nextDelay);
      }
    });
  }

  prefs.set("schedule", 100);
  prefs.set("retry", 5);
  
  let backoffsChecked = checkBackoffs([
    
    [5, 10, 20, 40, 80, 100, 100],
    
    [100, 100],
    
    [5, 10],
    
    [100, 100],
  ]);

  
  scheduler.init();

  
  yield backoffsChecked;

  scheduler.finalize();
});

function run_test() {
  run_next_test();
}
