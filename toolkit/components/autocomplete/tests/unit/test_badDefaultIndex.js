







function AutoCompleteNoMatchResult() {
  this.defaultIndex = 0;
}
AutoCompleteNoMatchResult.prototype = Object.create(AutoCompleteResultBase.prototype);





function AutoCompleteBadIndexResult(aValues, aDefaultIndex) {
  do_check_true(aValues.length <= aDefaultIndex);
  this._values = aValues;
  this.defaultIndex = aDefaultIndex;
}
AutoCompleteBadIndexResult.prototype = Object.create(AutoCompleteResultBase.prototype);

add_test(function autocomplete_noMatch_success() {
  const INPUT_STR = "moz";

  let searchNoMatch =
    new AutoCompleteSearchBase("searchNoMatch",
                               new AutoCompleteNoMatchResult());
  registerAutoCompleteSearch(searchNoMatch);

  
  let input = new AutoCompleteInputBase([searchNoMatch.name]);
  input.completeDefaultIndex = true;
  input.textValue = INPUT_STR;

  
  let strLen = INPUT_STR.length;
  input.selectTextRange(strLen, strLen);
  do_check_eq(input.selectionStart, strLen);
  do_check_eq(input.selectionEnd, strLen);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  controller.input = input;
  controller.startSearch(INPUT_STR);

  input.onSearchComplete = function () {
    
    do_check_eq(input.textValue, "moz");

    
    unregisterAutoCompleteSearch(searchNoMatch);
    run_next_test();
  };
});

add_test(function autocomplete_defaultIndex_exceeds_matchCount() {
  const INPUT_STR = "moz";

  
  let searchBadIndex =
    new AutoCompleteSearchBase("searchBadIndex",
                               new AutoCompleteBadIndexResult(["mozillaTest"], 1));
  registerAutoCompleteSearch(searchBadIndex);

  
  let input = new AutoCompleteInputBase([searchBadIndex.name]);
  input.completeDefaultIndex = true;
  input.textValue = INPUT_STR;

  
  let strLen = INPUT_STR.length;
  input.selectTextRange(strLen, strLen);
  do_check_eq(input.selectionStart, strLen);
  do_check_eq(input.selectionEnd, strLen);

  let controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  controller.input = input;
  controller.startSearch(INPUT_STR);

  input.onSearchComplete = function () {
    
    do_check_eq(input.textValue, "moz");

    
    unregisterAutoCompleteSearch(searchBadIndex);
    run_next_test();
  };
});

function run_test() {
  run_next_test();
}
