












let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

let tests = [

  { desc: "Add 1 bookmarked page.",
    addPages: 1,
    visitsPerPage: 1,
    addBookmarks: 1,
    limitExpiration: -1,
    expectedNotifications: 1, 
  },

  { desc: "Add 2 pages, 1 bookmarked.",
    addPages: 2,
    visitsPerPage: 1,
    addBookmarks: 1,
    limitExpiration: -1,
    expectedNotifications: 1, 
  },

  { desc: "Add 10 pages, none bookmarked.",
    addPages: 10,
    visitsPerPage: 1,
    addBookmarks: 0,
    limitExpiration: -1,
    expectedNotifications: 0, 
  },

  { desc: "Add 10 pages, all bookmarked.",
    addPages: 10,
    visitsPerPage: 1,
    addBookmarks: 10,
    limitExpiration: -1,
    expectedNotifications: 10, 
  },

  { desc: "Add 10 pages with lot of visits, none bookmarked.",
    addPages: 10,
    visitsPerPage: 10,
    addBookmarks: 0,
    limitExpiration: 10,
    expectedNotifications: 10, 
  },                           

];

function run_test() {
  run_next_test();
}

add_task(function test_notifications_onDeleteVisits() {
  
  setInterval(3600); 

  
  setMaxPages(0);

  for (let testIndex = 1; testIndex <= tests.length; testIndex++) {
    let currentTest = tests[testIndex -1];
    print("\nTEST " + testIndex + ": " + currentTest.desc);
    currentTest.receivedNotifications = 0;

    
    let now = getExpirablePRTime();
    for (let j = 0; j < currentTest.visitsPerPage; j++) {
      for (let i = 0; i < currentTest.addPages; i++) {
        let page = "http://" + testIndex + "." + i + ".mozilla.org/";
        yield PlacesTestUtils.addVisits({ uri: uri(page), visitDate: now++ });
      }
    }

    
    currentTest.bookmarks = [];
    for (let i = 0; i < currentTest.addBookmarks; i++) {
      let page = "http://" + testIndex + "." + i + ".mozilla.org/";
      bs.insertBookmark(bs.unfiledBookmarksFolder, uri(page),
                        bs.DEFAULT_INDEX, null);
      currentTest.bookmarks.push(page);
    }

    
    historyObserver = {
      onBeginUpdateBatch: function PEX_onBeginUpdateBatch() {},
      onEndUpdateBatch: function PEX_onEndUpdateBatch() {},
      onClearHistory: function() {},
      onVisit: function() {},
      onTitleChanged: function() {},
      onDeleteURI: function(aURI, aGUID, aReason) {
        
        do_check_eq(currentTest.bookmarks.indexOf(aURI.spec), -1);
        do_check_valid_places_guid(aGUID);
        do_check_eq(aReason, Ci.nsINavHistoryObserver.REASON_EXPIRED);
      },
      onPageChanged: function() {},
      onDeleteVisits: function(aURI, aTime, aGUID, aReason) {
        currentTest.receivedNotifications++;
        do_check_guid_for_uri(aURI, aGUID);
        do_check_eq(aReason, Ci.nsINavHistoryObserver.REASON_EXPIRED);
      },
    };
    hs.addObserver(historyObserver, false);

    
    yield promiseForceExpirationStep(currentTest.limitExpiration);

    hs.removeObserver(historyObserver, false);

    do_check_eq(currentTest.receivedNotifications,
                currentTest.expectedNotifications);

    
    bs.removeFolderChildren(bs.unfiledBookmarksFolder);
    yield PlacesTestUtils.clearHistory();
  }

  clearMaxPages();
  bs.removeFolderChildren(bs.unfiledBookmarksFolder);
  yield PlacesTestUtils.clearHistory();
});
