















let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

let tests = [

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

function run_test() {
  run_next_test();
}

add_task(function test_pref_maxpages() {
  
  try {
    getMaxPages();
    do_throw("interval pref should not exist by default");
  }
  catch (ex) {}

  
  setInterval(3600); 

  for (let testIndex = 1; testIndex <= tests.length; testIndex++) {
    let currentTest = tests[testIndex -1];
    print("\nTEST " + testIndex + ": " + currentTest.desc);
    currentTest.receivedNotifications = 0;

    
    let now = getExpirablePRTime();
    for (let i = 0; i < currentTest.addPages; i++) {
      let page = "http://" + testIndex + "." + i + ".mozilla.org/";
      yield PlacesTestUtils.addVisits({ uri: uri(page), visitDate: now++ });
    }

    
    historyObserver = {
      onBeginUpdateBatch: function PEX_onBeginUpdateBatch() {},
      onEndUpdateBatch: function PEX_onEndUpdateBatch() {},
      onClearHistory: function() {},
      onVisit: function() {},
      onTitleChanged: function() {},
      onDeleteURI: function(aURI) {
        print("onDeleteURI " + aURI.spec);
        currentTest.receivedNotifications++;
      },
      onPageChanged: function() {},
      onDeleteVisits: function(aURI, aTime) {
        print("onDeleteVisits " + aURI.spec + " " + aTime);
      },
    };
    hs.addObserver(historyObserver, false);

    setMaxPages(currentTest.maxPages);

    
    yield promiseForceExpirationStep(-1);

    hs.removeObserver(historyObserver, false);

    do_check_eq(currentTest.receivedNotifications,
                currentTest.expectedNotifications);

    
    yield PlacesTestUtils.clearHistory();
  }

  clearMaxPages();
  yield PlacesTestUtils.clearHistory();
});
