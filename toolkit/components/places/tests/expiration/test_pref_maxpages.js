
















































let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

let gTests = [

  { desc: "Set max_pages to a negative value, with 1 page.",
    maxPages: -1,
    addPages: 1,
    expectedNotifications: 0, 
  },

  { desc: "Set max_pages to 0.",
    maxPages: 0,
    addPages: 1,
    expectedNotifications: 1,
  },

  { desc: "Set max_pages to 0, with 2 pages.",
    maxPages: 0,
    addPages: 2,
    expectedNotifications: 2, 
  },

  
  
  
  
  { desc: "Set max_pages to 1 with 2 pages.",
    maxPages: 1,
    addPages: 2,
    expectedNotifications: 2, 
  },

  { desc: "Set max_pages to 10, with 9 pages.",
    maxPages: 10,
    addPages: 9,
    expectedNotifications: 0, 
  },

  { desc: "Set max_pages to 10 with 10 pages.",
    maxPages: 10,
    addPages: 10,
    expectedNotifications: 0, 
  },
];

let gCurrentTest;
let gTestIndex = 0;

function run_test() {
  
  try {
    getMaxPages();
    do_throw("interval pref should not exist by default");
  }
  catch (ex) {}

  
  setInterval(3600); 

  do_test_pending();
  run_next_test();
}

function run_next_test() {
  if (gTests.length) {
    gCurrentTest = gTests.shift();
    gTestIndex++;
    print("\nTEST " + gTestIndex + ": " + gCurrentTest.desc);
    gCurrentTest.receivedNotifications = 0;

    
    let now = Date.now() * 1000;
    for (let i = 0; i < gCurrentTest.addPages; i++) {
      hs.addVisit(uri("http://" + gTestIndex + "." + i + ".mozilla.org/"), now++, null,
                  hs.TRANSITION_TYPED, false, 0);
    }

    
    historyObserver = {
      onBeginUpdateBatch: function PEX_onBeginUpdateBatch() {},
      onEndUpdateBatch: function PEX_onEndUpdateBatch() {},
      onClearHistory: function() {},
      onVisit: function() {},
      onTitleChanged: function() {},
      onBeforeDeleteURI: function() {},
      onDeleteURI: function(aURI) {
        print("onDeleteURI " + aURI.spec);
        gCurrentTest.receivedNotifications++;
      },
      onPageChanged: function() {},
      onDeleteVisits: function(aURI, aTime) {
        print("onDeleteVisits " + aURI.spec + " " + aTime);
      },
    };
    hs.addObserver(historyObserver, false);

    
    observer = {
      observe: function(aSubject, aTopic, aData) {
        os.removeObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
        hs.removeObserver(historyObserver, false);

        
        check_result();
      }
    };
    os.addObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

    setMaxPages(gCurrentTest.maxPages);
    
    force_expiration_step(-1);
  }
  else {
    clearMaxPages();
    waitForClearHistory(do_test_finished);
  }
}

function check_result() {

  do_check_eq(gCurrentTest.receivedNotifications,
              gCurrentTest.expectedNotifications);

  
  waitForClearHistory(run_next_test);
}
