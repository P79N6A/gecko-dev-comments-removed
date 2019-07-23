
















































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
let current_test = 0;

function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  timeout: 10,
  textValue: "",
  searches: null,
  searchParam: "",
  popupOpen: false,
  minResultsForPopup: 0,
  invalidate: function() {},
  disableAutoComplete: false,
  completeDefaultIndex: false,
  get popup() { return this; },
  onSearchBegin: function() {},
  onSearchComplete: function() {},
  setSelectedIndex: function() {},
  get searchCount() { return this.searches.length; },
  getSearchAt: function(aIndex) this.searches[aIndex],
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput, Ci.nsIAutoCompletePopup])
};

function ensure_results(aSearch, aExpected)
{
  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);

  
  
  let input = new AutoCompleteInput(["history"]);

  controller.input = input;

  let numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {
    do_check_eq(numSearchesStarted, 1);
    aExpected = aExpected.slice();

    
    for (let i = 0; i < controller.matchCount; i++) {
      let value = controller.getValueAt(i);
      let comment = controller.getCommentAt(i);

      print("Looking for an expected result of " + value + ", " + comment + "...");
      let j;
      for (j = 0; j < aExpected.length; j++) {
        let [uri, title, tags] = aExpected[j];

        
        if (uri == undefined) continue;

        
        uri = iosvc.newURI(kURIs[uri], null, null).spec;
        title = kTitles[title];
        if (tags)
          title += " \u2013 " + tags.map(function(aTag) kTitles[aTag]);

        
        if (uri == value && title == comment) {
          print("Got it at index " + j + "!!");
          
          aExpected[j] = [,];
          break;
        }
      }

      
      if (j == aExpected.length)
        do_throw("Didn't find the current result (" + value + ", " + comment + ") in expected: " + aExpected);
    }

    
    do_check_eq(controller.matchCount, aExpected.length);

    
    do_check_eq(controller.searchStatus, aExpected.length ?
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH :
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);

    
    if (++current_test < gTests.length)
      run_test();

    do_test_finished();
  };

  print("Searching for.. " + aSearch);
  controller.startSearch(aSearch);
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  var tagsvc = Cc["@mozilla.org/browser/tagging-service;1"].
               getService(Ci.nsITaggingService);
  var iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
} catch(ex) {
  do_throw("Could not get services\n");
}


let gDate = new Date(Date.now() - 1000 * 60 * 60) * 1000;

function addPageBook(aURI, aTitle, aBook, aTags, aKey)
{
  let uri = iosvc.newURI(kURIs[aURI], null, null);
  let title = kTitles[aTitle];

  let out = [aURI, aTitle, aBook, aTags, aKey];
  out.push("\nuri=" + kURIs[aURI]);
  out.push("\ntitle=" + title);

  
  bhist.addPageWithDetails(uri, title, gDate);

  
  if (aBook != undefined) {
    let book = kTitles[aBook];
    let bmid = bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, uri,
      bmsvc.DEFAULT_INDEX, book);
    out.push("\nbook=" + book);

    
    if (aKey != undefined)
      bmsvc.setKeywordForBookmark(bmid, aKey);

    
    if (aTags != undefined && aTags.length > 0) {
      
      let tags = aTags.map(function(aTag) kTitles[aTag]);
      tagsvc.tagURI(uri, tags);
      out.push("\ntags=" + tags);
    }
  }

  print("\nAdding page/book/tag: " + out.join(", "));
}

function run_test() {
  print("\n");
  
  do_test_pending();

  
  let [description, search, expected, func] = gTests[current_test];
  print(description);

  
  if (func)
    func();

  ensure_results(search, expected);
}





let theTag = "superTag";


let kURIs = [
  "http://escaped/ユニコード",
  "http://asciiescaped/blocking-firefox3%2B",
];
let kTitles = [
  "title",
  theTag,
];


addPageBook(0, 0, 0, [1]);
addPageBook(1, 0, 0, [1]);




let gTests = [
  ["0: Make sure tag matches return the right url as well as '+' remain escaped",
   theTag, [[0,0],[1,0]]],
];
