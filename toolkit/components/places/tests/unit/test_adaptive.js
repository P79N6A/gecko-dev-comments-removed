





















































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
let bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
let bsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
let tsvc = Cc["@mozilla.org/browser/tagging-service;1"].
           getService(Ci.nsITaggingService);
let obs = Cc["@mozilla.org/observer-service;1"].
          getService(Ci.nsIObserverService);
let prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);


const PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC = "places-autocomplete-feedback-updated";

function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  constructor: AutoCompleteInput,

  searches: null,

  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "",
  disableAutoComplete: false,
  completeDefaultIndex: false,

  get searchCount() {
    return this.searches.length;
  },

  getSearchAt: function(aIndex) {
    return this.searches[aIndex];
  },

  onSearchComplete: function() {},

  popupOpen: false,

  popup: {
    setSelectedIndex: function(aIndex) {},
    invalidate: function() {},

    
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsISupports) ||
          iid.equals(Ci.nsIAutoCompletePopup))
        return this;

      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  },

  onSearchBegin: function() {},

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIAutoCompleteInput))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}




function ensure_results(expected, searchTerm)
{
  let controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
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

    if (tests.length)
      (tests.shift())();
    else {
      obs.removeObserver(observer, PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC);
      do_test_finished();
    }
  };

  controller.startSearch(searchTerm);
}




function setCountRank(aURI, aCount, aRank, aSearch, aBookmark)
{
  
  for (let i = 0; i < aCount; i++)
    histsvc.addVisit(aURI, d1, null, histsvc.TRANSITION_TYPED, false, 0);

  
  let thing = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput,
                                           Ci.nsIAutoCompletePopup,
                                           Ci.nsIAutoCompleteController]),
    get popup() { return thing; },
    get controller() { return thing; },
    popupOpen: true,
    selectedIndex: 0,
    getValueAt: function() aURI.spec,
    searchString: aSearch
  };

  
  for (let i = 0; i < aRank; i++) {
    obs.notifyObservers(thing, "autocomplete-will-enter-text", null);
  }

  
  if (aBookmark) {
    bsvc.insertBookmark(bsvc.unfiledBookmarksFolder, aURI, bsvc.DEFAULT_INDEX,
                        "test_book");

    
    if (aBookmark == "tag")
      tsvc.tagURI(aURI, "test_tag");
  }
}




function doAdaptiveDecay()
{
  for (let i = 0; i < 10; i++)
    obs.notifyObservers(null, "idle-daily", null);
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
    if (PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC == aTopic &&
        !(--this.runCount)) {
      ensure_results(this.results, this.search);
    }
  }
};
obs.addObserver(observer, PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC, false);




function prepTest(name) {
  print("Test " + name);
  bhist.removeAllPages();
  observer.runCount = -1;

  
  bsvc.removeFolderChildren(bsvc.unfiledBookmarksFolder);
  bsvc.removeFolderChildren(bsvc.tagsFolder);
}




function makeResult(aURI) {
  return {
    uri: aURI,
    style: "favicon",
  };
}

let tests = [
  
  function() {
    prepTest("0 same count, diff rank, same term; no search");
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
    prepTest("1 same count, diff rank, same term; no search");
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
    prepTest("2 diff count, same rank, same term; no search");
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
    prepTest("3 diff count, same rank, same term; no search");
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
    prepTest("4 same count, same rank, diff term; one exact/one partial search");
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
    prepTest("5 same count, same rank, diff term; one exact/one partial search");
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
    prepTest("6 same count, diff rank, same term; both exact search");
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
    prepTest("7 same count, diff rank, same term; both exact search");
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
    prepTest("8 same count, diff rank, same term; both partial search");
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
    prepTest("9 same count, diff rank, same term; both partial search");
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
    prepTest("10 same count, same rank, same term, decay first; exact match");
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
    prepTest("11 same count, same rank, same term, decay second; exact match");
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
    prepTest("12 same count, diff rank, same term; no search; history only");
    prefs.setIntPref("browser.urlbar.matchBehavior",
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
    prepTest("13 same count, diff rank, same term; no search; history only with tag");
    prefs.setIntPref("browser.urlbar.matchBehavior",
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
  (tests.shift())();
}
