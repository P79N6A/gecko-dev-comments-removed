



const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


{
  let commonFile = do_get_file("../head_common.js", false);
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}









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
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteInput,
    Ci.nsIAutoCompletePopup,
  ])
};

function toURI(aSpec) {
  return uri(aSpec);
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

  if (typeof kSearchParam == "string")
    input.searchParam = kSearchParam;

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


let gDate = new Date(Date.now() - 1000 * 60 * 60) * 1000;

let gPages = [];


let gNextTestSetupTasks = [];






























function addPageBook(aURI, aTitle, aBook, aTags, aKey, aTransitionType, aNoVisit)
{
  gNextTestSetupTasks.push([task_addPageBook, arguments]);
}

function* task_addPageBook(aURI, aTitle, aBook, aTags, aKey, aTransitionType, aNoVisit)
{
  
  gPages[aURI] = [aURI, aBook != undefined ? aBook : aTitle, aTags];

  let uri = toURI(kURIs[aURI]);
  let title = kTitles[aTitle];

  let out = [aURI, aTitle, aBook, aTags, aKey];
  out.push("\nuri=" + kURIs[aURI]);
  out.push("\ntitle=" + title);

  
  if (!aNoVisit) {
    yield PlacesTestUtils.addVisits({
      uri: uri,
      transition: aTransitionType || TRANSITION_LINK,
      visitDate: gDate,
      title: title
    });
    out.push("\nwith visit");
  }

  
  if (aBook != undefined) {
    let book = kTitles[aBook];
    let bmid = bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, uri,
      bmsvc.DEFAULT_INDEX, book);
    out.push("\nbook=" + book);

    
    if (aKey != undefined)
      yield PlacesUtils.keywords.insert({url: uri.spec, keyword: aKey});

    
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
  
  prefs.setBoolPref("browser.urlbar.suggest.history", true);
  prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  prefs.setBoolPref("browser.urlbar.suggest.openpage", true);
  prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", false);

  
  do_test_pending();

  
  let [description, search, expected, func] = gTests[current_test];
  print(description);

  
  appendTags = true;

  
  if (func)
    func();

  Task.spawn(function () {
    
    for (let [, [fn, args]] in Iterator(gNextTestSetupTasks)) {
      yield fn.apply(this, args);
    };

    
    gNextTestSetupTasks = [];

    
    
    
    yield PlacesTestUtils.promiseAsyncUpdates();

  }).then(function () ensure_results(search, expected),
          do_report_unexpected_exception);
}


function removePages(aURIs)
{
  gNextTestSetupTasks.push([do_removePages, arguments]);
}

function do_removePages(aURIs)
{
  for each (let uri in aURIs)
    histsvc.removePage(toURI(kURIs[uri]));
}


function markTyped(aURIs, aTitle)
{
  gNextTestSetupTasks.push([task_markTyped, arguments]);
}

function task_markTyped(aURIs, aTitle)
{
  for (let uri of aURIs) {
    yield PlacesTestUtils.addVisits({
      uri: toURI(kURIs[uri]),
      transition: TRANSITION_TYPED,
      title: kTitles[aTitle]
    });
  }
}
