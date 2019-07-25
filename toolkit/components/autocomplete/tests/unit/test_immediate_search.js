




function AutoCompleteImmediateSearch(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteImmediateSearch.prototype = Object.create(AutoCompleteSearchBase.prototype);
AutoCompleteImmediateSearch.prototype.searchType =
  Ci.nsIAutoCompleteSearchDescriptor.SEARCH_TYPE_IMMEDIATE;
AutoCompleteImmediateSearch.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.nsIFactory,
                         Ci.nsIAutoCompleteSearch,
                         Ci.nsIAutoCompleteSearchDescriptor]);

function AutoCompleteDelayedSearch(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteDelayedSearch.prototype = Object.create(AutoCompleteSearchBase.prototype);

function AutoCompleteResult(aValues, aDefaultIndex) {
  this._values = aValues;
  this.defaultIndex = aDefaultIndex;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function run_test() {
  run_next_test();
}




add_test(function test_immediate_search() {
  let immediateResults = ["mozillaTest"];
  let inputStr = "moz";

  let immediateSearch = new AutoCompleteImmediateSearch(
    "immediate", new AutoCompleteResult(["moz-immediate"], 0));
  registerAutoCompleteSearch(immediateSearch);
  let delayedSearch = new AutoCompleteDelayedSearch(
    "delayed", new AutoCompleteResult(["moz-delayed"], 0));
  registerAutoCompleteSearch(delayedSearch);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  

  let input = new AutoCompleteInputBase([delayedSearch.name,
                                         immediateSearch.name]);
  input.completeDefaultIndex = true;
  input.textValue = inputStr;

  
  
  let strLen = inputStr.length;
  input.selectTextRange(strLen, strLen);

  controller.input = input;
  controller.startSearch(inputStr);

  
  do_check_eq(input.textValue, "moz-immediate");

  
  input.onSearchComplete = function() {
    
    do_check_eq(input.textValue, "moz-immediate");

    unregisterAutoCompleteSearch(immediateSearch);
    unregisterAutoCompleteSearch(delayedSearch);
    run_next_test();
  };
});




add_test(function test_immediate_search_notimeout() {
  let immediateResults = ["mozillaTest"];
  let inputStr = "moz";

  let immediateSearch = new AutoCompleteImmediateSearch(
    "immediate", new AutoCompleteResult(["moz-immediate"], 0));
  registerAutoCompleteSearch(immediateSearch);

  let delayedSearch = new AutoCompleteDelayedSearch(
    "delayed", new AutoCompleteResult(["moz-delayed"], 0));
  registerAutoCompleteSearch(delayedSearch);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  

  let input = new AutoCompleteInputBase([delayedSearch.name,
                                         immediateSearch.name]);
  input.completeDefaultIndex = true;
  input.textValue = inputStr;
  input.timeout = 0;

  
  
  let strLen = inputStr.length;
  input.selectTextRange(strLen, strLen);

  controller.input = input;
  let complete = false;
  input.onSearchComplete = function() {
    complete = true;
  };
  controller.startSearch(inputStr);
  do_check_true(complete);

  
  do_check_eq(input.textValue, "moz-immediate");

  unregisterAutoCompleteSearch(immediateSearch);
  unregisterAutoCompleteSearch(delayedSearch);
  run_next_test();
});




add_test(function test_delayed_search_notimeout() {
  let immediateResults = ["mozillaTest"];
  let inputStr = "moz";

  let delayedSearch = new AutoCompleteDelayedSearch(
    "delayed", new AutoCompleteResult(["moz-delayed"], 0));
  registerAutoCompleteSearch(delayedSearch);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  

  let input = new AutoCompleteInputBase([delayedSearch.name]);
  input.completeDefaultIndex = true;
  input.textValue = inputStr;
  input.timeout = 0;

  
  
  let strLen = inputStr.length;
  input.selectTextRange(strLen, strLen);

  controller.input = input;
  let complete = false;
  input.onSearchComplete = function() {
    complete = true;
  };
  controller.startSearch(inputStr);
  do_check_true(complete);

  
  do_check_eq(input.textValue, "moz-delayed");

  unregisterAutoCompleteSearch(delayedSearch);
  run_next_test();
});
