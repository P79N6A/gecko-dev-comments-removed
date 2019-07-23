









































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const TRANSITION_LINK = Ci.nsINavHistoryService.TRANSITION_LINK;
const TRANSITION_TYPED = Ci.nsINavHistoryService.TRANSITION_TYPED;
const TRANSITION_BOOKMARK = Ci.nsINavHistoryService.TRANSITION_BOOKMARK;
const TRANSITION_EMBED = Ci.nsINavHistoryService.TRANSITION_EMBED;
const TRANSITION_REDIRECT_PERMANENT = Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT;
const TRANSITION_REDIRECT_TEMPORARY = Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY;
const TRANSITION_DOWNLOAD = Ci.nsINavHistoryService.TRANSITION_DOWNLOAD;

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

function toURI(aSpec)
{
  return iosvc.newURI(aSpec, null, null);
}

let appendTags = true;

function ignoreTags()
{
  print("Ignoring tags from results");
  appendTags = false;
}

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

      print("Looking for '" + value + "', '" + comment + "' in expected results...");
      let j;
      for (j = 0; j < aExpected.length; j++) {
        
        if (aExpected[j] == undefined)
          continue;

        let [uri, title, tags] = gPages[aExpected[j]];

        
        uri = toURI(kURIs[uri]).spec;
        title = kTitles[title];
        if (tags && appendTags)
          title += " \u2013 " + tags.map(function(aTag) kTitles[aTag]);
        print("Checking against expected '" + uri + "', '" + title + "'...");

        
        if (uri == value && title == comment) {
          print("Got it at index " + j + "!!");
          
          aExpected[j] = undefined;
          break;
        }
      }

      
      if (j == aExpected.length)
        do_throw("Didn't find the current result ('" + value + "', '" + comment + "') in expected: " + aExpected);
    }

    
    print("Expecting " + aExpected.length + " results; got " +
          controller.matchCount + " results");
    do_check_eq(controller.matchCount, aExpected.length);

    
    do_check_eq(controller.searchStatus, aExpected.length ?
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH :
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);

    
    if (++current_test < gTests.length)
      run_test();

    do_test_finished();
  };

  print("Searching for.. '" + aSearch + "'");
  controller.startSearch(aSearch);
}


var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var tagsvc = Cc["@mozilla.org/browser/tagging-service;1"].
             getService(Ci.nsITaggingService);
var iosvc = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);
var lmsvc = Cc["@mozilla.org/browser/livemark-service;2"].
            getService(Ci.nsILivemarkService);


let gDate = new Date(Date.now() - 1000 * 60 * 60) * 1000;

let gPages = [];





function DBConn()
{
  let db = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsPIPlacesDatabase).
           DBConnection;
  if (db.connectionReady)
    return db;

  
  let file = dirSvc.get('ProfD', Ci.nsIFile);
  file.append("places.sqlite");
  let storageService = Cc["@mozilla.org/storage/service;1"].
                       getService(Ci.mozIStorageService);
  try {
    var dbConn = storageService.openDatabase(file);
  } catch (ex) {
    return null;
  }
  return dbConn;
}














function setPageTitle(aURI, aTitle) {
  let dbConn = DBConn();
  
  let stmt = dbConn.createStatement(
    "SELECT id FROM moz_places_view WHERE url = :url");
  stmt.params.url = aURI.spec;
  try {
    if (!stmt.executeStep()) {
      do_throw("Unable to find page " + aURIString);
      return;
    }
  }
  finally {
    stmt.finalize();
  }

  
  stmt = dbConn.createStatement(
    "UPDATE moz_places_view SET title = :title WHERE url = :url");
  stmt.params.title = aTitle;
  stmt.params.url = aURI.spec;
  try {
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }
}



























function addLivemark(aContainerSiteURI, aContainerFeedURI, aContainerTitle,
                     aChildURI, aChildTitle, aTransitionType, aNoChildVisit)
{
  
  gPages[aChildURI] = [aChildURI, aChildTitle, null];

  let out = [aChildURI, aChildTitle];
  out.push("\nchild uri=" + kURIs[aChildURI]);
  out.push("\nchild title=" + kTitles[aChildTitle]);

  
  let containerSiteURI = toURI(kURIs[aContainerSiteURI]);
  let containerFeedURI = toURI(kURIs[aContainerFeedURI]);
  let containerTitle = kTitles[aContainerTitle];
  let containerId = lmsvc.createLivemarkFolderOnly(bmsvc.unfiledBookmarksFolder,
                                                   containerTitle,
                                                   containerSiteURI,
                                                   containerFeedURI,
                                                   bmsvc.DEFAULT_INDEX);
  
  let childURI = toURI(kURIs[aChildURI]);
  let childTitle = kTitles[aChildTitle];
  bmsvc.insertBookmark(containerId, childURI, bmsvc.DEFAULT_INDEX, childTitle);

  
  if (!aNoChildVisit) {
    let tt = aTransitionType || TRANSITION_LINK;
    let isRedirect = tt == TRANSITION_REDIRECT_PERMANENT ||
                     tt == TRANSITION_REDIRECT_TEMPORARY;
    histsvc.addVisit(childURI, gDate, null, tt, isRedirect, 0);
    out.push("\nwith visit");
  }

  print("\nAdding livemark: " + out.join(", "));
}






























function addPageBook(aURI, aTitle, aBook, aTags, aKey, aTransitionType, aNoVisit)
{
  
  gPages[aURI] = [aURI, aBook != undefined ? aBook : aTitle, aTags];

  let uri = toURI(kURIs[aURI]);
  let title = kTitles[aTitle];

  let out = [aURI, aTitle, aBook, aTags, aKey];
  out.push("\nuri=" + kURIs[aURI]);
  out.push("\ntitle=" + title);

  
  if (!aNoVisit) {
    let tt = aTransitionType || TRANSITION_LINK;
    let isRedirect = tt == TRANSITION_REDIRECT_PERMANENT ||
                     tt == TRANSITION_REDIRECT_TEMPORARY;
    histsvc.addVisit(uri, gDate, null, tt, isRedirect, 0);
    setPageTitle(uri, title);
    out.push("\nwith visit");
  }

  
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
  
  prefs.setIntPref("browser.urlbar.search.sources", 3);
  prefs.setIntPref("browser.urlbar.default.behavior", 0);

  
  do_test_pending();

  
  let [description, search, expected, func] = gTests[current_test];
  print(description);

  
  appendTags = true;

  
  if (func)
    func();

  ensure_results(search, expected);
}


function removePages(aURIs)
{
  for each (let uri in aURIs)
    histsvc.removePage(toURI(kURIs[uri]));
}


function markTyped(aURIs)
{
  for each (let uri in aURIs)
    histsvc.addVisit(toURI(kURIs[uri]), Date.now() * 1000, null,
      histsvc.TRANSITION_TYPED, false, 0);
}

