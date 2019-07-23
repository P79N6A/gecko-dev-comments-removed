















































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

  let search = kSearches[aSearch];
  print("Searching for.. " + search);
  controller.startSearch(search);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  var iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
} catch(ex) {
  do_throw("Could not get services\n");
} 


let gDate = new Date(Date.now() - 1000 * 60 * 60) * 1000;


let kURIs = [
  "http://abc/def",
  "http://def/ghi",
  "http://ghi/jkl",
  "http://jkl/mno",
];
let kTitles = [
  "foo bar",
  "bar baz",
];
let kSearches = [
  "c d",
  "b e",
  "b a z",
  "k f t",
  "d i g z",
  "m o z i",
];
  
function addPageBook(aURI, aTitle, aBook)
{
  let uri = iosvc.newURI(kURIs[aURI], null, null);
  let title = kTitles[aTitle];

  print("Adding page/book: " + [aURI, aTitle, aBook, kURIs[aURI], title].join(", "));
  
  bhist.addPageWithDetails(uri, title, gDate);

  
  if (aBook != undefined) {
    let book = kTitles[aBook];
    bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, uri, bmsvc.DEFAULT_INDEX, book);
  }
}

addPageBook(0, 0);
addPageBook(1, 1);
addPageBook(2, 0, 0);
addPageBook(3, 0, 1);




let gTests = [
  ["0: Match 2 terms all in url", [[0,0]]],
  ["1: Match 1 term in url and 1 term in title", [[0,0],[1,1]]],
  ["2: Match 3 terms all in title; display bookmark title if matched", [[1,1],[3,1]]],
  ["3: Match 2 terms in url and 1 in title; bookmark title didn't match", [[2,0],[3,0]]],
  ["4: Match 3 terms in url and 1 in title", [[1,1]]],
  ["5: Match nothing", []],
];

function run_test() {
  print("\n");
  
  do_test_pending();

  
  let [description, expected] = gTests[current_test];
  print(description);
  ensure_results(current_test, expected);
}
