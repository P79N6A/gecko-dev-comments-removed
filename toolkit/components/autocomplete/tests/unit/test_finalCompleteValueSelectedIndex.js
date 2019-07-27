function AutoCompleteResult(aResultValues) {
  this._values = aResultValues.map(x => x[0]);
  this._finalCompleteValues = aResultValues.map(x => x[1]);
}
AutoCompleteResult.prototype = Object.create(AutoCompleteResultBase.prototype);

let selectByWasCalled = false;
function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
  this.popup.selectedIndex = 0;
  this.popup.selectBy = function(reverse, page) {
    Assert.equal(selectByWasCalled, false);
    selectByWasCalled = true;
    Assert.equal(reverse, false);
    Assert.equal(page, false);
    this.selectedIndex += (reverse ? -1 : 1) * (page ? 100 : 1);
  };
  this.completeSelectedIndex = true;
}
AutoCompleteInput.prototype = Object.create(AutoCompleteInputBase.prototype);

function run_test() {
  run_next_test();
}

add_test(function test_handleEnter() {
  let results = [
    ["mozilla.com", "http://www.mozilla.com"],
    ["mozilla.org", "http://www.mozilla.org"],
  ];
  
  doSearch("moz", results, function(aController) {
    Assert.equal(aController.input.textValue, "moz");
    Assert.equal(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    Assert.equal(aController.getFinalCompleteValueAt(1), "http://www.mozilla.org");

    Assert.equal(aController.input.popup.selectedIndex, 0);
    aController.handleKeyNavigation(Ci.nsIDOMKeyEvent.DOM_VK_DOWN);
    Assert.equal(aController.input.popup.selectedIndex, 1);
    
    
    aController.input.popup.selectedIndex = 0;

    aController.handleEnter(false);
    
    
    Assert.equal(aController.input.textValue, "http://www.mozilla.org");
  });

  
  doSearch("moz", results, function(aController) {
    Assert.equal(aController.input.textValue, "moz");
    Assert.equal(aController.getFinalCompleteValueAt(0), "http://www.mozilla.com");
    Assert.equal(aController.getFinalCompleteValueAt(1), "http://www.mozilla.org");

    Assert.equal(aController.input.popup.selectedIndex, 0);
    aController.input.popupOpen = true;
    
    
    aController.input.popup.selectedIndex = 1;
    Assert.equal(selectByWasCalled, false);
    Assert.equal(aController.input.popup.selectedIndex, 1);

    aController.handleEnter(false);
    
    
    Assert.equal(aController.input.textValue, "moz");
  });
});

function doSearch(aSearchString, aResults, aOnCompleteCallback) {
  selectByWasCalled = false;
  let search = new AutoCompleteSearchBase(
    "search",
    new AutoCompleteResult(aResults)
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

