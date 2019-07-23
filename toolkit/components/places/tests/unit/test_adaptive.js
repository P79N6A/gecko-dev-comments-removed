





















































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
let current_test = 0;

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

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIAutoCompleteInput))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

function ensure_results(uris, searchTerm)
{
  let controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  let input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  do_test_pending();

  input.onSearchComplete = function() {
    do_check_eq(controller.searchStatus,
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    do_check_eq(controller.matchCount, uris.length);
    for (let i = 0; i < controller.matchCount; i++) {
      do_check_eq(controller.getValueAt(i), uris[i].spec);
    }

    if (current_test < (tests.length - 1)) {
      current_test++;
      tests[current_test]();
    }

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

function setCountRank(aURI, aCount, aRank, aSearch)
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

  
  for (let i = 0; i < aRank; i++)
    obs.notifyObservers(thing, "autocomplete-will-enter-text", null);
}

let uri1 = uri("http://site.tld/1");
let uri2 = uri("http://site.tld/2");


let d1 = new Date(Date.now() - 1000 * 60 * 60) * 1000;

let c1 = 10;
let c2 = 1;

let s0 = "";
let s1 = "si";
let s2 = "site";

function prepTest(name) {
  print("Test " + name);
  bhist.removeAllPages();
}

let tests = [

function() {
  prepTest("0 same count, diff rank, same term; no search");
  setCountRank(uri1, c1, c1, s2);
  setCountRank(uri2, c1, c2, s2);
  ensure_results([uri1, uri2], s0);
},
function() {
  prepTest("1 same count, diff rank, same term; no search");
  setCountRank(uri1, c1, c2, s2);
  setCountRank(uri2, c1, c1, s2);
  ensure_results([uri2, uri1], s0);
},
function() {
  prepTest("2 diff count, same rank, same term; no search");
  setCountRank(uri1, c1, c1, s2);
  setCountRank(uri2, c2, c1, s2);
  ensure_results([uri1, uri2], s0);
},
function() {
  prepTest("3 diff count, same rank, same term; no search");
  setCountRank(uri1, c2, c1, s2);
  setCountRank(uri2, c1, c1, s2);
  ensure_results([uri2, uri1], s0);
},


function() {
  prepTest("4 same count, same rank, diff term; one exact/one partial search");
  setCountRank(uri1, c1, c1, s1);
  setCountRank(uri2, c1, c1, s2);
  ensure_results([uri1, uri2], s1);
},
function() {
  prepTest("5 same count, same rank, diff term; one exact/one partial search");
  setCountRank(uri1, c1, c1, s2);
  setCountRank(uri2, c1, c1, s1);
  ensure_results([uri2, uri1], s1);
},


function() {
  prepTest("6 same count, diff rank, same term; both exact search");
  setCountRank(uri1, c1, c1, s1);
  setCountRank(uri2, c1, c2, s1);
  ensure_results([uri1, uri2], s1);
},
function() {
  prepTest("7 same count, diff rank, same term; both exact search");
  setCountRank(uri1, c1, c2, s1);
  setCountRank(uri2, c1, c1, s1);
  ensure_results([uri2, uri1], s1);
},


function() {
  prepTest("8 same count, diff rank, same term; both partial search");
  setCountRank(uri1, c1, c1, s2);
  setCountRank(uri2, c1, c2, s2);
  ensure_results([uri1, uri2], s1);
},
function() {
  prepTest("9 same count, diff rank, same term; both partial search");
  setCountRank(uri1, c1, c2, s2);
  setCountRank(uri2, c1, c1, s2);
  ensure_results([uri2, uri1], s1);
},
];




function run_test() {
  tests[0]();
}
