












































let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

let gTests = [

  { desc: "Add 1 bookmarked page.",
    addPages: 1,
    addBookmarks: 1,
    expectedNotifications: 0, 
  },

  { desc: "Add 2 pages, 1 bookmarked.",
    addPages: 2,
    addBookmarks: 1,
    expectedNotifications: 1, 
  },

  { desc: "Add 10 pages, none bookmarked.",
    addPages: 10,
    addBookmarks: 0,
    expectedNotifications: 10, 
  },

  { desc: "Add 10 pages, all bookmarked.",
    addPages: 10,
    addBookmarks: 10,
    expectedNotifications: 0, 
  },

];

let gCurrentTest;
let gTestIndex = 0;

function run_test() {
  
  setInterval(3600); 

  
  setMaxPages(0);

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
      let page = "http://" + gTestIndex + "." + i + ".mozilla.org/";
      hs.addVisit(uri(page), now++, null, hs.TRANSITION_TYPED, false, 0);
    }

    
    gCurrentTest.bookmarks = [];
    for (let i = 0; i < gCurrentTest.addBookmarks; i++) {
      let page = "http://" + gTestIndex + "." + i + ".mozilla.org/";
      bs.insertBookmark(bs.unfiledBookmarksFolder, uri(page),
                        bs.DEFAULT_INDEX, null);
      gCurrentTest.bookmarks.push(page);
    }

    
    historyObserver = {
      onBeginUpdateBatch: function PEX_onBeginUpdateBatch() {},
      onEndUpdateBatch: function PEX_onEndUpdateBatch() {},
      onClearHistory: function() {},
      onVisit: function() {},
      onTitleChanged: function() {},
      onBeforeDeleteURI: function() {},
      onDeleteURI: function(aURI) {
        gCurrentTest.receivedNotifications++;
        
        do_check_eq(gCurrentTest.bookmarks.indexOf(aURI.spec), -1);
      },
      onPageChanged: function() {},
      onDeleteVisits: function(aURI, aTime) { },
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

    
    force_expiration_step(-1);
  }
  else {
    clearMaxPages();
    bs.removeFolderChildren(bs.unfiledBookmarksFolder);
    waitForClearHistory(do_test_finished);
  }
}

function check_result() {

  do_check_eq(gCurrentTest.receivedNotifications,
              gCurrentTest.expectedNotifications);

  
  bs.removeFolderChildren(bs.unfiledBookmarksFolder);
  waitForClearHistory(run_next_test);
}
