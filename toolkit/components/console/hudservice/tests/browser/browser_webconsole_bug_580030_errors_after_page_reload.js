









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-error.html";

function test() {
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

    const successMsg = "Found the error message after page reload";
    const errMsg = "Could not get the error message after page reload";

    var display = HUDService.getDisplayByURISpec(content.location.href);
    var outputNode = display.querySelector(".hud-output-node");

    executeSoon(function() {
      testLogEntry(outputNode, "fooBazBaz",
                   { success: successMsg, err: errMsg });

      finishTest();
    });
  }
};

