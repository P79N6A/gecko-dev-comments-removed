




const TEST_REPLACED_API_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-replaced-api.html";

function test() {
  waitForExplicitFinish();

  
  addTab("about:blank");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    testOpenWebConsole(false);
  }, true);
}

function testWarningPresent() {
  
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    testOpenWebConsole(true);
  }, true);
  browser.contentWindow.location = TEST_REPLACED_API_URI;
}

function testOpenWebConsole(shouldWarn) {
  openConsole(null, function(hud) {
    waitForSuccess({
      name: (shouldWarn ? "no " : "") + "API replacement warning",
      validatorFn: function()
      {
        let pos = hud.outputNode.textContent.indexOf("disabled by");
        return shouldWarn ? pos > -1 : pos == -1;
      },
      successFn: function() {
        if (shouldWarn) {
          finishTest();
        }
        else {
          closeConsole(null, testWarningPresent);
        }
      },
      failureFn: finishTest,
    });
  });
}
