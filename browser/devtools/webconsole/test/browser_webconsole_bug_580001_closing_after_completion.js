










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("devtools.gcli.enable");
});

function test() {
  Services.prefs.setBoolPref("devtools.gcli.enable", false);
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testClosingAfterCompletion,
                           false);
}

function testClosingAfterCompletion() {
  browser.removeEventListener("DOMContentLoaded",
                              testClosingAfterCompletion, false);

  openConsole();

  let inputNode = HUDService.getHudByWindow(content).jsterm.inputNode;

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

