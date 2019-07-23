










































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

function ensure_results(aSearch, aExpected)
{
  let controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  let input = new AutoCompleteInput(["history"]);

  controller.input = input;

  var numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {
    do_check_eq(numSearchesStarted, 1);
    
    do_check_eq(controller.searchStatus, aExpected.length ?
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH :
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);

    
    do_check_eq(controller.matchCount, aExpected.length);

    
    for (let i = 0; i < controller.matchCount; i++) {
      let value = controller.getValueAt(i);
      let comment = controller.getCommentAt(i);

      print("Looking for an expected result of " + value + ", " + comment + "...");
      let j;
      for (j = 0; j < aExpected.length; j++) {
        let [uri, title] = aExpected[j];
        
        if (uri == undefined) continue;
        
        [uri, title] = [kURIs[uri], kTitles[title]];

        
        if (uri == value && title == comment) {
          print("Got it at index " + j + "!!");
          
          aExpected[j] = [,];
          break;
        }
      }

      
      if (j == aExpected.length)
        do_throw("Didn't find the current result (" + value + ", " + comment + ") in expected: " + aExpected);
    }

    
    if (++current_test < gTests.length)
      run_test();

    do_test_finished();
  };

  print("Searching for.. " + aSearch);
  controller.startSearch(aSearch);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
  var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
} catch(ex) {
  do_throw("Could not get services\n");
}


let gDate = new Date(Date.now() - 1000 * 60 * 60) * 1000;

function addPageBook(aURI, aTitle, aBook)
{
  let uri = iosvc.newURI(kURIs[aURI], null, null);
  let title = kTitles[aTitle];

  print("Adding page/book: " + [aURI, aTitle, aBook, kURIs[aURI], title].join(", "));
  
  histsvc.setPageDetails(uri, title, 1, false, true);
  histsvc.addVisit(uri, gDate, null, histsvc.TRANSITION_TYPED, false, 0);

  
  if (aBook != undefined) {
    let book = kTitles[aBook];
    bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, uri, bmsvc.DEFAULT_INDEX, book);
  }
}


let kURIs = [
  "http://abc/def",
  "javascript:5",
];
let kTitles = [
  "Title with javascript:",
];

let kPages = [[0,0], [1,0]];
for each (let [uri, title, book] in kPages)
  addPageBook(uri, title, book);




let gTests = [
  ["0: Match non-javascript: with plain search",
   "a", [[0,0]]],
  ["1: Match non-javascript: with almost javascript:",
   "javascript", [[0,0]]],
  ["2: Match javascript:",
   "javascript:", [[0,0],[1,0]]],
  ["3: Match nothing with non-first javascript:",
   "5 javascript:", []],
  ["4: Match javascript: with multi-word search",
   "javascript: 5", [[1,0]]],
];

function run_test() {
  print("\n");
  
  do_test_pending();

  
  let [description, search, expected] = gTests[current_test];
  print(description);
  ensure_results(search, expected);
}
