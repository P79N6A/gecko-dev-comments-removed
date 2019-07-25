



function AutoCompleteResult(aValues) {
  this._values = aValues;
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
  doSearch("MOZ", "mozilla", function(aController) {
    do_check_eq(aController.input.textValue, "MOZilla");
    aController.handleKeyNavigation(Ci.nsIDOMKeyEvent.DOM_VK_RIGHT);
    do_check_eq(aController.input.textValue, "mozilla");
  });
});

add_test(function test_handleEnter() {
  doSearch("MOZ", "mozilla", function(aController) {
    do_check_eq(aController.input.textValue, "MOZilla");
    aController.handleEnter(false);
    do_check_eq(aController.input.textValue, "mozilla");
  });
});

function doSearch(aSearchString, aResultValue, aOnCompleteCallback) {
  let search = new AutoCompleteSearchBase("search",
                                          new AutoCompleteResult([ "mozilla", "toolkit" ], 0));
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
