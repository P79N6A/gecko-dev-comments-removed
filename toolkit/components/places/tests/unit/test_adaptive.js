





















































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





const MAX_POLLING_TIMEOUT = 3000;


let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
let db = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsPIPlacesDatabase).
         DBConnection;
let bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);

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
    else
      do_test_finished();
  };

  controller.startSearch(searchTerm);
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


  
  
  let targetUseCount = 0;
  for (let i = 0; i < aRank; i++) {
    obs.notifyObservers(thing, "autocomplete-will-enter-text", null);
    targetUseCount = (targetUseCount * 0.9) + 1;
  }

  return Math.floor(targetUseCount);
}

let uri1 = uri("http://site.tld/1");
let uri2 = uri("http://site.tld/2");


let d1 = new Date(Date.now() - 1000 * 60 * 60) * 1000;

let c1 = 10;
let c2 = 1;

let s0 = "";
let s1 = "si";
let s2 = "site";

let curTestItem1 = null;
let curTestItem2 = null;

let pollTime = Date.now();
let stmt = db.createStatement(
    "SELECT use_count " +
    "FROM moz_inputhistory " +
    "WHERE input = ?1 AND place_id = " +
    "(SELECT IFNULL((SELECT id FROM moz_places_temp WHERE url = ?2), " +
                   "(SELECT id FROM moz_places WHERE url = ?2)))");





function poll_database(aSearch) {
  if (Date.now() - pollTime > MAX_POLLING_TIMEOUT)
    do_throw("*** TIMEOUT ***: The test timed out while polling database.\n");
  stmt.bindUTF8StringParameter(0, curTestItem1.input);
  stmt.bindUTF8StringParameter(1, curTestItem1.uri.spec);
  if (!stmt.executeStep()) {
    stmt.reset();
    do_timeout(100, "poll_database('" + aSearch + "');");
    return;
  }
  let useCount1 = stmt.getInt64(0);
  stmt.reset();
  stmt.bindUTF8StringParameter(0, curTestItem2.input);
  stmt.bindUTF8StringParameter(1, curTestItem2.uri.spec);
  if (!stmt.executeStep()) {
    stmt.reset();
    do_timeout(100, "poll_database('" + aSearch + "');");
    return;
  }
  let useCount2 = stmt.getInt64(0);
  stmt.reset();
  if (useCount1 == curTestItem1.useCount && useCount2 == curTestItem2.useCount) {
    
    ensure_results([curTestItem1.uri, curTestItem2.uri], aSearch);
  }
  else {
    
    do_timeout(100, "poll_database('" + aSearch + "');");
  }
}




function prepTest(name) {
  print("Test " + name);
  pollTime = Date.now();
  bhist.removeAllPages();
}

let current_test = 0;

let tests = [

function() {
  prepTest("0 same count, diff rank, same term; no search");
  let uc1 = setCountRank(uri1, c1, c1, s2);
  let uc2 = setCountRank(uri2, c1, c2, s2);
  curTestItem1 = { uri: uri1, input: s2, useCount: uc1 };
  curTestItem2 = { uri: uri2, input: s2, useCount: uc2 };
  do_timeout(100, "poll_database('" + s0 + "');");
},
function() {
  prepTest("1 same count, diff rank, same term; no search");
  let uc1 = setCountRank(uri1, c1, c2, s2);
  let uc2 = setCountRank(uri2, c1, c1, s2);
  curTestItem2 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem1 = {uri: uri2, input: s2, useCount: uc2};
  do_timeout(100, "poll_database('" + s0 + "');");
},
function() {
  prepTest("2 diff count, same rank, same term; no search");
  let uc1 = setCountRank(uri1, c1, c1, s2);
  let uc2 = setCountRank(uri2, c2, c1, s2);
  curTestItem1 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem2 = {uri: uri2, input: s2, useCount: uc2};
  do_timeout(100, "poll_database('" + s0 + "');");
},
function() {
  prepTest("3 diff count, same rank, same term; no search");
  let uc1 = setCountRank(uri1, c2, c1, s2);
  let uc2 = setCountRank(uri2, c1, c1, s2);
  curTestItem2 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem1 = {uri: uri2, input: s2, useCount: uc2};  
  do_timeout(100, "poll_database('" + s0 + "');");
},


function() {
  prepTest("4 same count, same rank, diff term; one exact/one partial search");
  let uc1 = setCountRank(uri1, c1, c1, s1);
  let uc2 = setCountRank(uri2, c1, c1, s2);
  curTestItem1 = {uri: uri1, input: s1, useCount: uc1};
  curTestItem2 = {uri: uri2, input: s2, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
},
function() {
  prepTest("5 same count, same rank, diff term; one exact/one partial search");
  let uc1 = setCountRank(uri1, c1, c1, s2);
  let uc2 = setCountRank(uri2, c1, c1, s1);
  curTestItem2 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem1 = {uri: uri2, input: s1, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
},


function() {
  prepTest("6 same count, diff rank, same term; both exact search");
  let uc1 = setCountRank(uri1, c1, c1, s1);
  let uc2 = setCountRank(uri2, c1, c2, s1);
  curTestItem1 = {uri: uri1, input: s1, useCount: uc1};
  curTestItem2 = {uri: uri2, input: s1, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
},
function() {
  prepTest("7 same count, diff rank, same term; both exact search");
  let uc1 = setCountRank(uri1, c1, c2, s1);
  let uc2 = setCountRank(uri2, c1, c1, s1);
  curTestItem2 = {uri: uri1, input: s1, useCount: uc1};
  curTestItem1 = {uri: uri2, input: s1, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
},


function() {
  prepTest("8 same count, diff rank, same term; both partial search");
  let uc1 = setCountRank(uri1, c1, c1, s2);
  let uc2 = setCountRank(uri2, c1, c2, s2);
  curTestItem1 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem2 = {uri: uri2, input: s2, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
},
function() {
  prepTest("9 same count, diff rank, same term; both partial search");
  let uc1 = setCountRank(uri1, c1, c2, s2);
  let uc2 = setCountRank(uri2, c1, c1, s2);
  curTestItem2 = {uri: uri1, input: s2, useCount: uc1};
  curTestItem1 = {uri: uri2, input: s2, useCount: uc2};
  do_timeout(100, "poll_database('" + s1 + "');");
}
];




function run_test() {
  do_test_pending();
  tests[current_test]();
}
