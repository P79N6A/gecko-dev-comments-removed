version(180);
















































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);

function add_visit(aURI, aVisitDate, aVisitType) {
  var isRedirect = aVisitType == histsvc.TRANSITION_REDIRECT_PERMANENT ||
                   aVisitType == histsvc.TRANSITION_REDIRECT_TEMPORARY;
  var placeID = histsvc.addVisit(aURI, aVisitDate, null,
                                 aVisitType, isRedirect, 0);
  do_check_true(placeID > 0);
  return placeID;
}


var searchTerm = "ユニコード";
var decoded = "http://www.foobar.com/" + searchTerm + "/";
var url = uri(decoded);
add_visit(url, Date.now(), Ci.nsINavHistoryService.TRANSITION_LINK);

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

function run_test() {
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

    
    do_check_eq(controller.matchCount, 1);

    
    do_check_eq(controller.getValueAt(0), decoded);

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}
