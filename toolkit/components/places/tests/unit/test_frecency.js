













































var current_test = 0;

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
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  do_test_pending();

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

    if (current_test < (tests.length - 1)) {
      current_test++;
      tests[current_test]();
    }

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

function setCountDate(aURI, aCount, aDate)
{
  
  histsvc.setPageDetails(aURI, aURI, aCount, false, true);
  
  for (let i = 0; i < aCount; i++)
    histsvc.addVisit(aURI, aDate, null, histsvc.TRANSITION_TYPED, false, 0);
}

var uri1 = uri("http://site.tld/1");
var uri2 = uri("http://site.tld/2");



var d1 = new Date(Date.now() - 1000 * 60 * 60) * 1000;
var d2 = new Date(Date.now() - 1000 * 60 * 60 * 24 * 10) * 1000;

var c1 = 10;
var c2 = 1;

function prepTest(desc) {
  print("Test " + desc);
  bhist.removeAllPages();
}

var tests = [

function() {
  prepTest("0: same count, different date");
  setCountDate(uri1, c1, d1);
  setCountDate(uri2, c1, d2);
  ensure_results([uri1, uri2], "");
},
function() {
  prepTest("1: same count, different date");
  setCountDate(uri1, c1, d2);
  setCountDate(uri2, c1, d1);
  ensure_results([uri2, uri1], "");
},
function() {
  prepTest("2: different count, same date");
  setCountDate(uri1, c1, d1);
  setCountDate(uri2, c2, d1);
  ensure_results([uri1, uri2], "");
},
function() {
  prepTest("3: different count, same date");
  setCountDate(uri1, c2, d1);
  setCountDate(uri2, c1, d1);
  ensure_results([uri2, uri1], "");
},


function() {
  prepTest("4: same count, different date");
  setCountDate(uri1, c1, d1);
  setCountDate(uri2, c1, d2);
  ensure_results([uri1, uri2], "site");
},
function() {
  prepTest("5: same count, different date");
  setCountDate(uri1, c1, d2);
  setCountDate(uri2, c1, d1);
  ensure_results([uri2, uri1], "site");
},
function() {
  prepTest("6: different count, same date");
  setCountDate(uri1, c1, d1);
  setCountDate(uri2, c2, d1);
  ensure_results([uri1, uri2], "site");
},
function() {
  prepTest("7: different count, same date");
  setCountDate(uri1, c2, d1);
  setCountDate(uri2, c1, d1);
  ensure_results([uri2, uri1], "site");
}
];




function run_test() {
  tests[0]();
}
