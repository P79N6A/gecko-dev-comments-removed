



















function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  constructor: AutoCompleteInput,

  get minResultsForPopup() 0,
  get timeout() 10,
  get searchParam() "",
  get textValue() "",
  get disableAutoComplete() false,
  get completeDefaultIndex() false,

  get searchCount() this.searches.length,
  getSearchAt: function (aIndex) this.searches[aIndex],

  onSearchBegin: function () {},
  onSearchComplete: function() {},

  get popupOpen() false,
  popup: {
    set selectedIndex(aIndex) aIndex,
    invalidate: function () {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput])
}




function ensure_results(expected, searchTerm)
{
  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);

  
  
  let input = new AutoCompleteInput(["history"]);

  controller.input = input;

  input.onSearchComplete = function() {
    do_check_eq(controller.searchStatus,
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    do_check_eq(controller.matchCount, expected.length);
    for (let i = 0; i < controller.matchCount; i++) {
      print("Testing for '" + expected[i].uri.spec + "' got '" + controller.getValueAt(i) + "'");
      do_check_eq(controller.getValueAt(i), expected[i].uri.spec);
      do_check_eq(controller.getStyleAt(i), expected[i].style);
    }

    deferEnsureResults.resolve();
  };

  controller.startSearch(searchTerm);
}




function task_setCountRank(aURI, aCount, aRank, aSearch, aBookmark)
{
  
  let visits = [];
  for (let i = 0; i < aCount; i++) {
    visits.push({ uri: aURI, visitDate: d1, transition: TRANSITION_TYPED });
  }
  yield PlacesTestUtils.addVisits(visits);

  
  let thing = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput,
                                           Ci.nsIAutoCompletePopup,
                                           Ci.nsIAutoCompleteController]),
    get popup() thing,
    get controller() thing,
    popupOpen: true,
    selectedIndex: 0,
    getValueAt: function() aURI.spec,
    searchString: aSearch
  };

  
  for (let i = 0; i < aRank; i++) {
    Services.obs.notifyObservers(thing, "autocomplete-will-enter-text", null);
  }

  
  if (aBookmark) {
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         aURI,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "test_book");

    
    if (aBookmark == "tag") {
      PlacesUtils.tagging.tagURI(aURI, ["test_tag"]);
    }
  }
}




function doAdaptiveDecay()
{
  PlacesUtils.history.runInBatchMode({
    runBatched: function() {
      for (let i = 0; i < 10; i++) {
        PlacesUtils.history.QueryInterface(Ci.nsIObserver)
                           .observe(null, "idle-daily", null);
      }
    }
  }, this);
}

let uri1 = uri("http://site.tld/1");
let uri2 = uri("http://site.tld/2");


let d1 = new Date(Date.now() - 1000 * 60 * 60) * 1000;

let c1 = 10;
let c2 = 1;

let s0 = "";
let s1 = "si";
let s2 = "site";

let observer = {
  results: null,
  search: null,
  runCount: -1,
  observe: function(aSubject, aTopic, aData)
  {
    if (--this.runCount > 0)
      return;
    ensure_results(this.results, this.search);
  }
};
Services.obs.addObserver(observer, PlacesUtils.TOPIC_FEEDBACK_UPDATED, false);




function makeResult(aURI, aStyle = "favicon") {
  return {
    uri: aURI,
    style: aStyle,
  };
}

let tests = [
  
  function() {
    print("Test 0 same count, diff rank, same term; no search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c1, s2);
    yield task_setCountRank(uri2, c1, c2, s2);
  },
  function() {
    print("Test 1 same count, diff rank, same term; no search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c2, s2);
    yield task_setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 2 diff count, same rank, same term; no search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri1, c1, c1, s2);
    yield task_setCountRank(uri2, c2, c1, s2);
  },
  function() {
    print("Test 3 diff count, same rank, same term; no search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s0;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri1, c2, c1, s2);
    yield task_setCountRank(uri2, c1, c1, s2);
  },

  
  function() {
    print("Test 4 same count, same rank, diff term; one exact/one partial search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri1, c1, c1, s1);
    yield task_setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 5 same count, same rank, diff term; one exact/one partial search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri1, c1, c1, s2);
    yield task_setCountRank(uri2, c1, c1, s1);
  },

  
  function() {
    print("Test 6 same count, diff rank, same term; both exact search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c1, s1);
    yield task_setCountRank(uri2, c1, c2, s1);
  },
  function() {
    print("Test 7 same count, diff rank, same term; both exact search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c2, s1);
    yield task_setCountRank(uri2, c1, c1, s1);
  },

  
  function() {
    print("Test 8 same count, diff rank, same term; both partial search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c1, s2);
    yield task_setCountRank(uri2, c1, c2, s2);
  },
  function() {
    print("Test 9 same count, diff rank, same term; both partial search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c2, s2);
    yield task_setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 10 same count, same rank, same term, decay first; exact match");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri1, c1, c1, s1);
    doAdaptiveDecay();
    yield task_setCountRank(uri2, c1, c1, s1);
  },
  function() {
    print("Test 11 same count, same rank, same term, decay second; exact match");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    yield task_setCountRank(uri2, c1, c1, s1);
    doAdaptiveDecay();
    yield task_setCountRank(uri1, c1, c1, s1);
  },
  
  function() {
    print("Test 12 same count, diff rank, same term; no search; history only");
    Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
    Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", false);
    Services.prefs.setBoolPref("browser.urlbar.suggest.openpage", false);
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c1, s2, "bookmark");
    yield task_setCountRank(uri2, c1, c2, s2);
  },
  
  function() {
    print("Test 13 same count, diff rank, same term; no search; history only with tag");
    Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
    Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", false);
    Services.prefs.setBoolPref("browser.urlbar.suggest.openpage", false);
    observer.results = [
      makeResult(uri1, "tag"),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    yield task_setCountRank(uri1, c1, c1, s2, "tag");
    yield task_setCountRank(uri2, c1, c2, s2);
  },
];





let deferEnsureResults;




function run_test()
{
  run_next_test();
}

add_task(function test_adaptive()
{
  for (let [, test] in Iterator(tests)) {
    
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.tagsFolderId);
    observer.runCount = -1;

    let types = ["history", "bookmark", "openpage"];
    for (let type of types) {
      Services.prefs.clearUserPref("browser.urlbar.suggest." + type);
    }

    yield PlacesTestUtils.clearHistory();

    deferEnsureResults = Promise.defer();
    yield test();
    yield deferEnsureResults.promise;
  }

  Services.obs.removeObserver(observer, PlacesUtils.TOPIC_FEEDBACK_UPDATED);
});
