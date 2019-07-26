



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

add_test(function test_handleEnter() {
  doSearch("", "mozilla.com", "http://www.mozilla.com", function(aController) {
    do_check_eq(aController.input.textValue, "");
    do_check_eq(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    aController.input.forceComplete = true;
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

  controller.input = input;
  controller.startSearch(aSearchString);

  input.onSearchComplete = function onSearchComplete() {
    aOnCompleteCallback(controller);

    
    unregisterAutoCompleteSearch(search);
    run_next_test();
  };
}
