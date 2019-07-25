



const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



XPCOMUtils.defineLazyServiceGetter(this, "gHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");

function VisitInfo(aTransitionType, aVisitTime)
{
  this.transitionType =
    aTransitionType === undefined ? TRANSITION_LINK : aTransitionType;
  this.visitDate = aVisitTime || Date.now() * 1000;
}

function addVisits(aUrls)
{
  let places = [];
  aUrls.forEach(function(url) {
    places.push({
                  uri: url.url,
                  title: "test for " + url.url,
                  visits: [
                    new VisitInfo(url.transition),
                  ],
    });
  });

  gHistory.updatePlaces(places);
}





function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  searches: null,
  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "",
  disableAutoComplete: false,

  completeDefaultIndex: true,
  defaultIndex: 0,

  
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

  onTextEntered: function() false,
  onTextReverted: function() false,

  get searchCount() {
    return this.searches.length;
  },
  getSearchAt: function(aIndex) {
    return this.searches[aIndex];
  },

  onSearchBegin: function () {},
  onSearchComplete: function () {},

  popupOpen: false,

  popup: {
    selectedIndex: -1,
    invalidate: function () {},

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput])
}














function ensure_results(aSearchString, aExpectedValue) {
  let autoFilledValue, completedValue;
  if (typeof(aExpectedValue) == "string") {
    autoFilledValue = aExpectedValue;
  }
  else {
    autoFilledValue = aExpectedValue.autoFilled;
    completedValue = aExpectedValue.completed;
  }

  
  let input = new AutoCompleteInput(["urlinline"]);
  input.textValue = aSearchString;

  
  let strLen = aSearchString.length;
  input.selectTextRange(strLen, strLen);
  do_check_eq(input.selectionStart, strLen);
  do_check_eq(input.selectionEnd, strLen);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);
  controller.input = input;

  let numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {
    
    do_check_eq(numSearchesStarted, 1);

    
    do_check_eq(input.textValue, autoFilledValue);

    if (completedValue) {
      
      
      
      controller.handleEnter(false);
      do_check_eq(input.textValue, completedValue);
    }

    waitForCleanup(run_next_test);
  };

  do_log_info("Searching for: '" + aSearchString + "'");
  controller.startSearch(aSearchString);
}

function run_test() {
  Services.prefs.setBoolPref("browser.urlbar.autoFill", true);
  do_register_cleanup(function () {
    Services.prefs.clearUserPref("browser.urlbar.autoFill");
  });

  gAutoCompleteTests.forEach(function (testData) {
    let [description, searchString, expectedValue, setupFunc] = testData;
    add_test(function () {
      do_log_info(description);
      if (setupFunc) {
        setupFunc();
      }

      
      
      
      
      waitForAsyncUpdates(ensure_results, this, [searchString, expectedValue]);
    })
  }, this);

  run_next_test();
}

let gAutoCompleteTests = [];
function add_autocomplete_test(aTestData) {
  gAutoCompleteTests.push(aTestData);
}

function waitForCleanup(aCallback) {
  remove_all_bookmarks();
  waitForClearHistory(aCallback);
}

function addBookmark(aBookmarkObj) {
  do_check_true(!!aBookmarkObj.url);
  let parentId = aBookmarkObj.parentId ? aBookmarkObj.parentId
                                       : PlacesUtils.unfiledBookmarksFolderId;
  let itemId = PlacesUtils.bookmarks
                          .insertBookmark(parentId,
                                          NetUtil.newURI(aBookmarkObj.url),
                                          PlacesUtils.bookmarks.DEFAULT_INDEX,
                                          "A bookmark");
  if (aBookmarkObj.keyword) {
    PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aBookmarkObj.keyword);
  }
}
