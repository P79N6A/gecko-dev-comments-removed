





















































function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  constructor: AutoCompleteInput,

  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "",
  disableAutoComplete: false,
  completeDefaultIndex: false,

  get searchCount() this.searches.length,

  getSearchAt: function (aIndex) this.searches[aIndex],

  onSearchComplete: function() {},

  popupOpen: false,

  popup: {
    setSelectedIndex: function (aIndex) {},
    invalidate: function () {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])
  },

  onSearchBegin: function () {},

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

    next_test();
  };

  controller.startSearch(searchTerm);
}




function setCountRank(aURI, aCount, aRank, aSearch, aBookmark)
{
  
  for (let i = 0; i < aCount; i++) {
    PlacesUtils.history.addVisit(aURI, d1, null,
                                 PlacesUtils.history.TRANSITION_TYPED,
                                 false, 0);
  }

  
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

    
    if (aBookmark == "tag")
      PlacesUtils.tagging.tagURI(aURI, "test_tag");
  }
}




function doAdaptiveDecay()
{
  for (let i = 0; i < 10; i++) {
    PlacesUtils.history.QueryInterface(Ci.nsIObserver)
                       .observe(null, "idle-daily", null);
  }
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




function makeResult(aURI) {
  return {
    uri: aURI,
    style: "favicon",
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
    setCountRank(uri1, c1, c1, s2);
    setCountRank(uri2, c1, c2, s2);
  },
  function() {
    print("Test 1 same count, diff rank, same term; no search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c2, s2);
    setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 2 diff count, same rank, same term; no search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c1;
    setCountRank(uri1, c1, c1, s2);
    setCountRank(uri2, c2, c1, s2);
  },
  function() {
    print("Test 3 diff count, same rank, same term; no search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s0;
    observer.runCount = c1 + c1;
    setCountRank(uri1, c2, c1, s2);
    setCountRank(uri2, c1, c1, s2);
  },

  
  function() {
    print("Test 4 same count, same rank, diff term; one exact/one partial search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    setCountRank(uri1, c1, c1, s1);
    setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 5 same count, same rank, diff term; one exact/one partial search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    setCountRank(uri1, c1, c1, s2);
    setCountRank(uri2, c1, c1, s1);
  },

  
  function() {
    print("Test 6 same count, diff rank, same term; both exact search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c1, s1);
    setCountRank(uri2, c1, c2, s1);
  },
  function() {
    print("Test 7 same count, diff rank, same term; both exact search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c2, s1);
    setCountRank(uri2, c1, c1, s1);
  },

  
  function() {
    print("Test 8 same count, diff rank, same term; both partial search");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c1, s2);
    setCountRank(uri2, c1, c2, s2);
  },
  function() {
    print("Test 9 same count, diff rank, same term; both partial search");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c2, s2);
    setCountRank(uri2, c1, c1, s2);
  },
  function() {
    print("Test 10 same count, same rank, same term, decay first; exact match");
    observer.results = [
      makeResult(uri2),
      makeResult(uri1),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    setCountRank(uri1, c1, c1, s1);
    doAdaptiveDecay();
    setCountRank(uri2, c1, c1, s1);
  },
  function() {
    print("Test 11 same count, same rank, same term, decay second; exact match");
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s1;
    observer.runCount = c1 + c1;
    setCountRank(uri2, c1, c1, s1);
    doAdaptiveDecay();
    setCountRank(uri1, c1, c1, s1);
  },
  
  function() {
    print("Test 12 same count, diff rank, same term; no search; history only");
    Services.prefs.setIntPref("browser.urlbar.matchBehavior",
                              Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY);
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c1, s2, "bookmark");
    setCountRank(uri2, c1, c2, s2);
  },
  function() {
    print("Test 13 same count, diff rank, same term; no search; history only with tag");
    Services.prefs.setIntPref("browser.urlbar.matchBehavior",
                              Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY);
    observer.results = [
      makeResult(uri1),
      makeResult(uri2),
    ];
    observer.search = s0;
    observer.runCount = c1 + c2;
    setCountRank(uri1, c1, c1, s2, "tag");
    setCountRank(uri2, c1, c2, s2);
  },
];




function run_test() {
  do_test_pending();
  next_test();
}

function next_test() {
  if (tests.length) {
    
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.tagsFolderId);
    observer.runCount = -1;

    let test = tests.shift();
    waitForClearHistory(test);
  }
  else {
    Services.obs.removeObserver(observer, PlacesUtils.TOPIC_FEEDBACK_UPDATED);
    do_test_finished();
  }
}
