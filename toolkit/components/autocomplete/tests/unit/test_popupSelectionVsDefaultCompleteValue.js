



function AutoCompleteTypeAheadResult(aValues, aComments) {
  this._values = aValues;
  this._comments = aComments;
  this.defaultIndex = 0;
  this._typeAheadResult = true;
}
AutoCompleteTypeAheadResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function AutoCompleteResult(aValues, aComments) {
  this._values = aValues;
  this._comments = aComments;
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
  this.popupOpen = true;
  this.completeDefaultIndex = true;
  this.completeSelectedIndex = true;
}
AutoCompleteInput.prototype = Object.create(AutoCompleteInputBase.prototype);

function run_test() {
  run_next_test();
}

add_test(function test_handleEnter() {
  doSearch("moz", function(aController) {
    do_check_eq(aController.input.textValue, "mozilla.com");
    aController.handleEnter(true);
    do_check_eq(aController.input.textValue, "mozilla.org");
  });
});

function doSearch(aSearchString, aOnCompleteCallback) {
  let typeAheadSearch = new AutoCompleteSearchBase(
    "typeAheadSearch",
    new AutoCompleteTypeAheadResult([ "mozilla.com" ], [ "http://www.mozilla.com" ])
  );
  registerAutoCompleteSearch(typeAheadSearch);

  let search = new AutoCompleteSearchBase(
    "search",
    new AutoCompleteResult([ "mozilla.org" ], [ "http://www.mozilla.org" ])
  );
  registerAutoCompleteSearch(search);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);

  
  let input = new AutoCompleteInput([ typeAheadSearch.name, search.name ]);
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
