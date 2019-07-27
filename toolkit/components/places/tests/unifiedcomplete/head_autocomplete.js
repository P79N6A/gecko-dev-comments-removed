



const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



const TITLE_SEARCH_ENGINE_SEPARATOR = " \u00B7\u2013\u00B7 ";

function run_test() {
  run_next_test();
}

function* cleanup() {
  Services.prefs.clearUserPref("browser.urlbar.autocomplete.enabled");
  Services.prefs.clearUserPref("browser.urlbar.autoFill");
  Services.prefs.clearUserPref("browser.urlbar.autoFill.typed");
  Services.prefs.clearUserPref("browser.urlbar.autoFill.searchEngines");
  remove_all_bookmarks();
  yield promiseClearHistory();
}
do_register_cleanup(cleanup);





function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  popup: {
    selectedIndex: -1,
    invalidate: function () {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])
  },
  popupOpen: false,

  disableAutoComplete: false,
  completeDefaultIndex: true,
  completeSelectedIndex: true,
  forceComplete: false,

  minResultsForPopup: 0,
  maxRows: 0,

  showCommentColumn: false,
  showImageColumn: false,

  timeout: 10,
  searchParam: "",

  get searchCount() {
    return this.searches.length;
  },
  getSearchAt: function(aIndex) {
    return this.searches[aIndex];
  },

  textValue: "",
  
  _selStart: 0,
  _selEnd: 0,
  get selectionStart() {
    return this._selStart;
  },
  get selectionEnd() {
    return this._selEnd;
  },
  selectTextRange: function(aStart, aEnd) {
    this._selStart = aStart;
    this._selEnd = aEnd;
  },

  onSearchBegin: function () {},
  onSearchComplete: function () {},

  onTextEntered: function() false,
  onTextReverted: function() false,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput])
}

function* check_autocomplete(test) {
  
  
  
  
  yield promiseAsyncUpdates();

  
  let input = new AutoCompleteInput(["unifiedcomplete"]);
  input.textValue = test.search;

  if (test.searchParam)
    input.searchParam = test.searchParam;

  
  let strLen = test.search.length;
  input.selectTextRange(strLen, strLen);
  Assert.equal(input.selectionStart, strLen, "Selection starts at end");
  Assert.equal(input.selectionEnd, strLen, "Selection ends at the end");

  let controller = Cc["@mozilla.org/autocomplete/controller;1"]
                     .getService(Ci.nsIAutoCompleteController);
  controller.input = input;

  let numSearchesStarted = 0;
  input.onSearchBegin = () => {
    do_log_info("onSearchBegin received");
    numSearchesStarted++;
  };
  let deferred = Promise.defer();
  input.onSearchComplete = () => {
    do_log_info("onSearchComplete received");
    deferred.resolve();
  }

  let expectedSearches = 1;
  if (test.incompleteSearch) {
    controller.startSearch(test.incompleteSearch);
    expectedSearches++;
  }
  do_log_info("Searching for: '" + test.search + "'");
  controller.startSearch(test.search);
  yield deferred.promise;

  Assert.equal(numSearchesStarted, expectedSearches, "All searches started");

  
  if (test.matches) {
    
    let matches = test.matches.slice();

    for (let i = 0; i < controller.matchCount; i++) {
      let value = controller.getValueAt(i);
      let comment = controller.getCommentAt(i);
      do_log_info("Looking for '" + value + "', '" + comment + "' in expected results...");
      let j;
      for (j = 0; j < matches.length; j++) {
        
        if (matches[j] == undefined)
          continue;

        let { uri, title, tags, searchEngine } = matches[j];
        if (tags)
          title += " \u2013 " + tags.sort().join(", ");
        if (searchEngine)
          title += TITLE_SEARCH_ENGINE_SEPARATOR + searchEngine;

        do_log_info("Checking against expected '" + uri.spec + "', '" + title + "'...");
        
        if (stripPrefix(uri.spec) == stripPrefix(value) && title == comment) {
          do_log_info("Got a match at index " + j + "!");
          
          matches[j] = undefined;
          if (uri.spec.startsWith("moz-action:")) {
            let style = controller.getStyleAt(i);
            Assert.ok(style.contains("action"));
          }
          break;
        }
      }

      
      if (j == matches.length)
        do_throw("Didn't find the current result ('" + value + "', '" + comment + "') in matches");
    }

    Assert.equal(controller.matchCount, matches.length,
                 "Got as many results as expected");

    
    do_check_eq(controller.searchStatus, matches.length ?
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH :
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);
  }

  if (test.autofilled) {
    
    Assert.equal(input.textValue, test.autofilled,
                 "Autofilled value is correct");

    
    
    
    controller.handleEnter(false);
    Assert.equal(input.textValue, test.completed,
                 "Completed value is correct");
  }
}

function addBookmark(aBookmarkObj) {
  Assert.ok(!!aBookmarkObj.uri, "Bookmark object contains an uri");
  let parentId = aBookmarkObj.parentId ? aBookmarkObj.parentId
                                       : PlacesUtils.unfiledBookmarksFolderId;
  let itemId = PlacesUtils.bookmarks
                          .insertBookmark(parentId,
                                          aBookmarkObj.uri,
                                          PlacesUtils.bookmarks.DEFAULT_INDEX,
                                          aBookmarkObj.title || "A bookmark");
  if (aBookmarkObj.keyword) {
    PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aBookmarkObj.keyword);
  }

  if (aBookmarkObj.tags) {
    PlacesUtils.tagging.tagURI(aBookmarkObj.uri, aBookmarkObj.tags);
  }
}

function addOpenPages(aUri, aCount=1) {
  let ac = Cc["@mozilla.org/autocomplete/search;1?name=unifiedcomplete"]
             .getService(Ci.mozIPlacesAutoComplete);
  for (let i = 0; i < aCount; i++) {
    ac.registerOpenPage(aUri);
  }
}

function removeOpenPages(aUri, aCount=1) {
  let ac = Cc["@mozilla.org/autocomplete/search;1?name=unifiedcomplete"]
             .getService(Ci.mozIPlacesAutoComplete);
  for (let i = 0; i < aCount; i++) {
    ac.unregisterOpenPage(aUri);
  }
}

function changeRestrict(aType, aChar) {
  let branch = "browser.urlbar.";
  
  if (aType == "title" || aType == "url")
    branch += "match.";
  else
    branch += "restrict.";

  do_log_info("changing restrict for " + aType + " to '" + aChar + "'");
  Services.prefs.setCharPref(branch + aType, aChar);
}

function resetRestrict(aType) {
  let branch = "browser.urlbar.";
  
  if (aType == "title" || aType == "url")
    branch += "match.";
  else
    branch += "restrict.";

  Services.prefs.clearUserPref(branch + aType);
}








function stripPrefix(spec)
{
  ["http://", "https://", "ftp://"].some(scheme => {
    if (spec.startsWith(scheme)) {
      spec = spec.slice(scheme.length);
      return true;
    }
    return false;
  });

  if (spec.startsWith("www.")) {
    spec = spec.slice(4);
  }
  return spec;
}
