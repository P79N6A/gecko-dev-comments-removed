




















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

  onSearchBegin: function() {},
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

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIAutoCompleteInput))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

function ensure_results(uris, searchTerm)
{
  PlacesTestUtils.promiseAsyncUpdates()
                 .then(() => ensure_results_internal(uris, searchTerm));
}

function ensure_results_internal(uris, searchTerm)
{
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  var numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {
    do_check_eq(numSearchesStarted, 1);
    do_check_eq(controller.searchStatus,
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    do_check_eq(controller.matchCount, uris.length);
    for (var i=0; i<controller.matchCount; i++) {
      do_check_eq(controller.getValueAt(i), uris[i].spec);
    }

    deferEnsureResults.resolve();
  };

  controller.startSearch(searchTerm);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
  var bmksvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

function task_setCountDate(aURI, aCount, aDate)
{
  
  let visits = [];
  for (let i = 0; i < aCount; i++) {
    visits.push({ uri: aURI, visitDate: aDate, transition: TRANSITION_TYPED });
  }
  yield PlacesTestUtils.addVisits(visits);
}

function setBookmark(aURI)
{
  bmksvc.insertBookmark(bmksvc.bookmarksMenuFolder, aURI, -1, "bleh");
}

function tagURI(aURI, aTags) {
  bmksvc.insertBookmark(bmksvc.unfiledBookmarksFolder, aURI,
                        bmksvc.DEFAULT_INDEX, "bleh");
  tagssvc.tagURI(aURI, aTags);
}

var uri1 = uri("http://site.tld/1");
var uri2 = uri("http://site.tld/2");
var uri3 = uri("http://aaaaaaaaaa/1");
var uri4 = uri("http://aaaaaaaaaa/2");



var d1 = new Date(Date.now() - 1000 * 60 * 60) * 1000;
var d2 = new Date(Date.now() - 1000 * 60 * 60 * 24 * 10) * 1000;

var c1 = 10;
var c2 = 1;

var tests = [

function() {
  print("TEST-INFO | Test 0: same count, different date");
  yield task_setCountDate(uri1, c1, d1);
  yield task_setCountDate(uri2, c1, d2);
  tagURI(uri1, ["site"]);
  ensure_results([uri1, uri2], "");
},
function() {
  print("TEST-INFO | Test 1: same count, different date");
  yield task_setCountDate(uri1, c1, d2);
  yield task_setCountDate(uri2, c1, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri2, uri1], "");
},
function() {
  print("TEST-INFO | Test 2: different count, same date");
  yield task_setCountDate(uri1, c1, d1);
  yield task_setCountDate(uri2, c2, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri1, uri2], "");
},
function() {
  print("TEST-INFO | Test 3: different count, same date");
  yield task_setCountDate(uri1, c2, d1);
  yield task_setCountDate(uri2, c1, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri2, uri1], "");
},


function() {
  print("TEST-INFO | Test 4: same count, different date");
  yield task_setCountDate(uri1, c1, d1);
  yield task_setCountDate(uri2, c1, d2);
  tagURI(uri1, ["site"]);
  ensure_results([uri1, uri2], "site");
},
function() {
  print("TEST-INFO | Test 5: same count, different date");
  yield task_setCountDate(uri1, c1, d2);
  yield task_setCountDate(uri2, c1, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri2, uri1], "site");
},
function() {
  print("TEST-INFO | Test 6: different count, same date");
  yield task_setCountDate(uri1, c1, d1);
  yield task_setCountDate(uri2, c2, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri1, uri2], "site");
},
function() {
  print("TEST-INFO | Test 7: different count, same date");
  yield task_setCountDate(uri1, c2, d1);
  yield task_setCountDate(uri2, c1, d1);
  tagURI(uri1, ["site"]);
  ensure_results([uri2, uri1], "site");
},


function() {
  print("TEST-INFO | Test 8.1a: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "a");
},
function() {
  print("TEST-INFO | Test 8.1b: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "aa");
},
function() {
  print("TEST-INFO | Test 8.2: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "aaa");
},
function() {
  print("TEST-INFO | Test 8.3: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "aaaa");
},
function() {
  print("TEST-INFO | Test 8.4: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "aaa");
},
function() {
  print("TEST-INFO | Test 8.5: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "aa");
},
function() {
  print("TEST-INFO | Test 8.6: same count, same date");
  setBookmark(uri3);
  setBookmark(uri4);
  ensure_results([uri4, uri3], "a");
}
];





let deferEnsureResults;




function run_test()
{
  run_next_test();
}

add_task(function test_frecency()
{
  
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);

  prefs.setBoolPref("browser.urlbar.suggest.history", true);
  prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  prefs.setBoolPref("browser.urlbar.suggest.openpage", false);
  for (let [, test] in Iterator(tests)) {
    yield PlacesUtils.bookmarks.eraseEverything();
    yield PlacesTestUtils.clearHistory();

    deferEnsureResults = Promise.defer();
    yield test();
    yield deferEnsureResults.promise;
  }
  for (let type of ["history", "bookmark", "openpage"]) {
    prefs.clearUserPref("browser.urlbar.suggest." + type);
  }
});
