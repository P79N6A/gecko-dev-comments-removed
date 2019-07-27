





"use strict";




function test() {
  const TEST_URI = "data:text/html;charset=utf8,test autocompletion with $ or _";
  Task.spawn(runner).then(finishTest);

  function* runner() {
    function autocomplete(term) {
      let deferred = promise.defer();

      jsterm.setInputValue(term);
      jsterm.complete(jsterm.COMPLETE_HINT_ONLY, deferred.resolve);

      yield deferred.promise;

      ok(popup.itemCount > 0, "There's suggestions for '" + term + "'");
    }

    yield addTab(TEST_URI);
    let { jsterm } = yield openConsole(tab);
    let popup = jsterm.autocompletePopup;

    jsterm.execute("let testObject = {$$aaab: '', $$aaac: ''}");

    
    yield autocomplete("Object.__d");
    yield autocomplete("testObject.$$a");

    
    yield autocomplete("Object.__de");
    yield autocomplete("testObject.$$aa");
  }
}
