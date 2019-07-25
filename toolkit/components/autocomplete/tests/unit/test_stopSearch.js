









Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");








function AutoCompleteInput(aSearches)
{
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  constructor: AutoCompleteInput,
  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "hello",
  disableAutoComplete: false, 
  completeDefaultIndex: false,
  set popupOpen(val) { return val; }, 
  get popupOpen() { return false; },
  get searchCount() { return this.searches.length; },
  getSearchAt: function(aIndex) { return this.searches[aIndex]; },
  onSearchBegin: function() {},
  onSearchComplete: function() {},
  onTextReverted: function () {},
  onTextEntered: function () {},
  popup: {
    selectBy: function() {},
    invalidate: function() {},
    set selectedIndex(val) { return val; }, 
    get selectedIndex() { return -1 },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput])
}





function AutoCompleteSearch(aName)
{
  this.name = aName;
}
AutoCompleteSearch.prototype = {
  constructor: AutoCompleteSearch,
  stopSearchInvoked: true,
  startSearch: function(aSearchString, aSearchParam, aPreviousResult, aListener)
  {
    print("Check stop search has been called");
    do_check_true(this.stopSearchInvoked);
    this.stopSearchInvoked = false;
  },
  stopSearch: function()
  {
    this.stopSearchInvoked = true;
  },
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFactory
  , Ci.nsIAutoCompleteSearch
  ]),
  createInstance: function(outer, iid)
  {
    return this.QueryInterface(iid);
  }
}






function registerAutoCompleteSearch(aSearch)
{
  let name = "@mozilla.org/autocomplete/search;1?name=" + aSearch.name;
  let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"].
                      getService(Ci.nsIUUIDGenerator);
  let cid = uuidGenerator.generateUUID();
  let desc = "Test AutoCompleteSearch";
  let componentManager = Components.manager
                                   .QueryInterface(Ci.nsIComponentRegistrar);
  componentManager.registerFactory(cid, desc, name, aSearch);
  
  aSearch.cid = cid; 
}





function unregisterAutoCompleteSearch(aSearch) {
  let componentManager = Components.manager
                                   .QueryInterface(Ci.nsIComponentRegistrar);  
  componentManager.unregisterFactory(aSearch.cid, aSearch);
}


let gTests = [
  function(controller) {
    print("handleText");
    controller.input.textValue = "hel";
    controller.handleText();
  },
  function(controller) {
    print("handleStartComposition");
    controller.handleStartComposition();
  },
  function(controller) {
    print("handleEndComposition");
    controller.handleEndComposition();
  },
  function(controller) {
    print("handleEscape");
    controller.handleEscape();
  },
  function(controller) {
    print("handleEnter");
    controller.handleEnter(false);
  },
  function(controller) {
    print("handleTab");
    controller.handleTab();
  },

  function(controller) {
    print("handleKeyNavigation");
    controller.handleKeyNavigation(Ci.nsIDOMKeyEvent.DOM_VK_UP);
  },
];


let gSearch;
let gCurrentTest;
function run_test() {
  
  gSearch = new AutoCompleteSearch("test");
  registerAutoCompleteSearch(gSearch);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);

  
  let input = new AutoCompleteInput([gSearch.name]);
  controller.input = input;

  input.onSearchBegin = function() {
    do_execute_soon(function() {
      gCurrentTest(controller);
    });
  };
  input.onSearchComplete = function() {
    run_next_test(controller);
  }

  
  do_test_pending();

  run_next_test(controller);
}

function run_next_test(controller) {
  if (gTests.length == 0) {
    unregisterAutoCompleteSearch(gSearch);
    controller.stopSearch();
    controller.input = null;
    do_test_finished();
    return;
  }

  gCurrentTest = gTests.shift();
  controller.startSearch("hello");
}
