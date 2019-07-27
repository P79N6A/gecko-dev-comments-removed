



function AutoCompleteResult(aValues, aFinalCompleteValues) {
  this._values = aValues;
  this._finalCompleteValues = aFinalCompleteValues;
  this.defaultIndex = 0;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
  this.popup.selectedIndex = -1;
}
AutoCompleteInput.prototype = Object.create(AutoCompleteInputBase.prototype);

function run_test() {
  run_next_test();
}

add_test(function test_handleEnterWithDirectMatchCompleteSelectedIndex() {
  doSearch("moz", "mozilla.com", "http://www.mozilla.com",
    { forceComplete: true, completeSelectedIndex: true }, function(aController) {
    do_check_eq(aController.input.textValue, "moz");
    do_check_eq(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    aController.handleEnter(false);
    
    do_check_eq(aController.input.textValue, "http://www.mozilla.com");
  });
});

add_test(function test_handleEnterWithDirectMatch() {
  doSearch("mozilla", "mozilla.com", "http://www.mozilla.com",
    { forceComplete: true, completeDefaultIndex: true }, function(aController) {
    
    do_check_eq(aController.input.textValue, "mozilla.com");
    do_check_eq(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    aController.handleEnter(false);
    
    do_check_eq(aController.input.textValue, "http://www.mozilla.com");
  });
});

add_test(function test_handleEnterWithNoMatch() {
  doSearch("mozilla", "mozilla.com", "http://www.mozilla.com",
    { forceComplete: true, completeDefaultIndex: true }, function(aController) {
    
    do_check_eq(aController.input.textValue, "mozilla.com");
    do_check_eq(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    
    aController.input.textValue = "mozillax";
    
    
    aController.handleEnter(false);
    do_check_eq(aController.input.textValue, "mozillax");
  });
});

add_test(function test_handleEnterWithIndirectMatch() {
  doSearch("com", "mozilla.com", "http://www.mozilla.com",
    { forceComplete: true, completeDefaultIndex: true }, function(aController) {
    
    do_check_eq(aController.input.textValue, "com >> mozilla.com");
    do_check_eq(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    aController.handleEnter(false);
    
    
    do_check_eq(aController.input.textValue, "http://www.mozilla.com");
  });
});

function doSearch(aSearchString, aResultValue, aFinalCompleteValue,
                  aInputProps, aOnCompleteCallback) {
  let search = new AutoCompleteSearchBase(
    "search",
    new AutoCompleteResult([ aResultValue ], [ aFinalCompleteValue ])
  );
  registerAutoCompleteSearch(search);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  
  
  let input = new AutoCompleteInput([ search.name ]);
  for (var p in aInputProps) {
    input[p] = aInputProps[p];
  }
  input.textValue = aSearchString;
  
  
  input.selectTextRange(aSearchString.length, aSearchString.length);

  controller.input = input;
  controller.startSearch(aSearchString);

  input.onSearchComplete = function onSearchComplete() {
    aOnCompleteCallback(controller);

    
    unregisterAutoCompleteSearch(search);
    run_next_test();
  };
}
