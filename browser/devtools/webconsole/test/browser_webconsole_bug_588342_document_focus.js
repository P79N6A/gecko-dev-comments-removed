









const TEST_URI = "data:text/html;charset=utf-8,Web Console test for bug 588342";

let fm = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);

"use strict";

let test = asyncTest(function* () {
  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();
  yield consoleOpened(hud);

  is(fm.focusedWindow, browser.contentWindow,
     "content document has focus");

  fm = null;
});

function consoleOpened(hud) {
  let deferred = promise.defer();
  waitForFocus(function() {
    is(hud.jsterm.inputNode.getAttribute("focused"), "true",
       "jsterm input is focused on web console open");
    isnot(fm.focusedWindow, content, "content document has no focus");
    closeConsole(null).then(deferred.resolve);
  }, hud.iframeWindow);

  return deferred.promise;
}
