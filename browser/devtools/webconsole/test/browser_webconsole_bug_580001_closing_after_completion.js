







const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testClosingAfterCompletion);
  }, true);
}

function testClosingAfterCompletion(hud) {
  let inputNode = hud.jsterm.inputNode;

  let errorWhileClosing = false;
  function errorListener(evt) {
    errorWhileClosing = true;
  }

  browser.addEventListener("error", errorListener, false);

  
  inputNode.focus();
  EventUtils.synthesizeKey("k", { accelKey: true, shiftKey: true });

  
  
  executeSoon(function() {
    browser.removeEventListener("error", errorListener, false);
    is(errorWhileClosing, false, "no error while closing the WebConsole");
    finishTest();
  });
}

