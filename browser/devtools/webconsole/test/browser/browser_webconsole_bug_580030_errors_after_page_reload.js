









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-error.html";

function test() {
  expectUncaughtException();
  addTab(TEST_URI);
  browser.addEventListener("load", onLoad, true);
}



function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  browser.addEventListener("load", testErrorsAfterPageReload, true);
  executeSoon(function() {
    content.location.reload();
  });
}

function testErrorsAfterPageReload(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  
  

  Services.console.registerListener(consoleObserver);

  var button = content.document.querySelector("button").wrappedJSObject;
  var clickEvent = content.document.createEvent("MouseEvents");
  clickEvent.initMouseEvent("click", true, true,
    content, 0, 0, 0, 0, 0, false, false,
    false, false, 0, null);

  executeSoon(function() {
    button.dispatchEvent(clickEvent);
  });
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

    executeSoon(function() {
      let msg = "Found the error message after page reload";
      testLogEntry(outputNode, "fooBazBaz", msg);
      finishTest();
    });
  }
};

