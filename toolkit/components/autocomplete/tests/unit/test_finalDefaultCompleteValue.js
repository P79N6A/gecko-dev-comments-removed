



function AutoCompleteResult(aValues, aFinalCompleteValues) {
  this._values = aValues;
  this._finalCompleteValues = aFinalCompleteValues;
  this.defaultIndex = 0;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
  this.popup.selectedIndex = -1;
  this.completeDefaultIndex = true;
}
AutoCompleteInput.prototype = Object.create(AutoCompleteInputBase.prototype);

function run_test() {
  run_next_test();
}

add_test(function test_keyNavigation() {
  doSearch("moz", "mozilla.com", "http://www.mozilla.com", function(aController) {
    do_check_eq(aController.input.textValue, "mozilla.com");
    aController.handleKeyNavigation(Ci.nsIDOMKeyEvent.DOM_VK_RIGHT);
    do_check_eq(aController.input.textValue, "mozilla.com");
  });
});

add_test(function test_handleEnter() {
  doSearch("moz", "mozilla.com", "http://www.mozilla.com", function(aController) {
    do_check_eq(aController.input.textValue, "mozilla.com");
    aController.handleEnter(false);
    do_check_eq(aController.input.textValue, "http://www.mozilla.com");
  });
});

function doSearch(aSearchString, aResultValue, aFinalCompleteValue, aOnCompleteCallback) {
  let search = new AutoCompleteSearchBase(
    "search",
    new AutoCompleteResult([ aResultValue ], [ aFinalCompleteValue ])
  );
  registerAutoCompleteSearch(search);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  
  
  let input = new AutoCompleteInput([ search.name ]);
  input.textValue = aSearchString;

  
  let strLen = aSearchString.length;
  input.selectTextRange(strLen, strLen);
  controller.input = input;
  controller.startSearch(aSearchString);

  input.onSearchComplete = function onSearchComplete() {
    aOnCompleteCallback(controller);

    
    unregisterAutoCompleteSearch(search);
    run_next_test();
  };
}
