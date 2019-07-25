


function AutoCompleteResult(aValues) {
  this._values = aValues;
  this.defaultIndex = -1;
  this._typeAheadResult = false;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function AutoCompleteTypeAheadResult(aValues) {
  this._values = aValues;
  this.defaultIndex = 0;
  this._typeAheadResult = true;
}
AutoCompleteTypeAheadResult.prototype = Object.create(AutoCompleteResultBase.prototype);






function run_test() {
  do_test_pending();

  var inputStr = "moz";

  
  var searchTypeAhead = new AutoCompleteSearchBase("search1",
                                                   new AutoCompleteTypeAheadResult(["mozillaTest1"]));
  
  var searchNormal = new AutoCompleteSearchBase("search2",
                                                new AutoCompleteResult(["mozillaTest2"]));
  
  
  registerAutoCompleteSearch(searchNormal);
  registerAutoCompleteSearch(searchTypeAhead);
  
  
  
  var input = new AutoCompleteInputBase([searchTypeAhead.name, searchNormal.name]);
  input.completeDefaultIndex = true;
  input.textValue = inputStr;

  
  
  var strLen = inputStr.length;
  input.selectTextRange(strLen, strLen);
  do_check_eq(input.selectionStart, strLen);
  do_check_eq(input.selectionEnd, strLen);

  var controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  

  controller.input = input;
  controller.startSearch(inputStr);

  input.onSearchComplete = function() {
    
    do_check_eq(input.textValue, "mozillaTest1");

    
    
    
    controller.handleEnter(true);
    do_check_eq(input.textValue, "mozillaTest2");

    
    do_check_eq(controller.matchCount, 1);

    
    unregisterAutoCompleteSearch(searchNormal);
    unregisterAutoCompleteSearch(searchTypeAhead);
    do_test_finished();
  };
}
