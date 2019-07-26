






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-error.html";

function test() {
  expectUncaughtException();
  addTab(TEST_URI);
  browser.addEventListener("load", onLoad, true);
}



function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, onLoad, true);

  openConsole(null, function(hud) {
    hud.jsterm.clearOutput();
    browser.addEventListener("load", testErrorsAfterPageReload, true);
    content.location.reload();
  });
}

function testErrorsAfterPageReload(aEvent) {
  browser.removeEventListener(aEvent.type, testErrorsAfterPageReload, true);

  
  

  Services.console.registerListener(consoleObserver);

  let button = content.document.querySelector("button").wrappedJSObject;
  ok(button, "button found");
  EventUtils.sendMouseEvent({type: "click"}, button, content.wrappedJSObject);
}

var consoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function test_observe(aMessage)
  {
    
    if (!(aMessage instanceof Ci.nsIScriptError) ||
      aMessage.category != "content javascript") {
      return;
    }

    Services.console.unregisterListener(this);

    let outputNode = HUDService.getHudByWindow(content).outputNode;

    waitForSuccess({
      name: "error message after page reload",
      validatorFn: function()
      {
        return outputNode.textContent.indexOf("fooBazBaz") > -1;
      },
      successFn: finishTest,
      failureFn: finishTest,
    });
  }
};

