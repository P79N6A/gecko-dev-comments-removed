






function AutoCompleteAsyncSearch(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteAsyncSearch.prototype = Object.create(AutoCompleteSearchBase.prototype);
AutoCompleteAsyncSearch.prototype.startSearch = function(aSearchString, 
                                                         aSearchParam, 
                                                         aPreviousResult, 
                                                         aListener) {
  setTimeout(this._returnResults.bind(this), 500, aListener);
};

AutoCompleteAsyncSearch.prototype._returnResults = function(aListener) {
  var result = this._result;

  result.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
  aListener.onSearchResult(this, result);
};




function AutoCompleteSyncSearch(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteSyncSearch.prototype = Object.create(AutoCompleteAsyncSearch.prototype);
AutoCompleteSyncSearch.prototype.startSearch = function(aSearchString, 
                                                        aSearchParam, 
                                                        aPreviousResult, 
                                                        aListener) {
  this._returnResults(aListener);
};




function AutoCompleteResult(aValues, aDefaultIndex) {
  this._values = aValues;
  this.defaultIndex = aDefaultIndex;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);






function run_test() {
  do_test_pending();

  var results = ["mozillaTest"];
  var inputStr = "moz";

  
  var asyncSearch = new AutoCompleteAsyncSearch("Async", 
                                                new AutoCompleteResult(results, -1));
  
  var syncSearch = new AutoCompleteSyncSearch("Sync",
                                              new AutoCompleteResult(results, 0));
  
  
  registerAutoCompleteSearch(asyncSearch);
  registerAutoCompleteSearch(syncSearch);
    
  var controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  
  
  
  
  var input = new AutoCompleteInputBase([asyncSearch.name, syncSearch.name]);
  input.completeDefaultIndex = true;
  input.textValue = inputStr;

  
  
  var strLen = inputStr.length;
  input.selectTextRange(strLen, strLen);

  controller.input = input;
  controller.startSearch(inputStr);

  input.onSearchComplete = function() {
    do_check_eq(input.textValue, results[0]);

    
    unregisterAutoCompleteSearch(asyncSearch);
    unregisterAutoCompleteSearch(syncSearch);
    do_test_finished();
  };
}
