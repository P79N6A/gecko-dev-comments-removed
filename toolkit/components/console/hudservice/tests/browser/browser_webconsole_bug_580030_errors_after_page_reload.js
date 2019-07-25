









































const TEST_ERROR_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-error.html";

function test() {
  return;
  

  addTab(TEST_ERROR_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}



function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);

  openConsole();

  browser.addEventListener("DOMContentLoaded", testErrorsAfterPageReload,
                           false);
  browser.contentWindow.wrappedJSObject.location.reload();
}

function testErrorsAfterPageReload() {
  browser.removeEventListener("DOMContentLoaded", testErrorsAfterPageReload,
                              false);

  
  

  var contentDocument = browser.contentDocument.wrappedJSObject;
  var button = contentDocument.getElementsByTagName("button")[0];
  var clickEvent = contentDocument.createEvent("MouseEvents");
  clickEvent.initMouseEvent("click", true, true,
    browser.contentWindow.wrappedJSObject, 0, 0, 0, 0, 0, false, false,
    false, false, 0, null);

  Services.console.registerListener(consoleObserver);
  button.dispatchEvent(clickEvent);
}

var consoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function (aMessage)
  {
    
    if (!(aMessage instanceof Ci.nsIScriptError) ||
      aMessage.category != "content javascript") {
      return;
    }

    Services.console.unregisterListener(this);

    const successMsg = "Found the error message after page reload";
    const errMsg = "Could not get the error message after page reload";

    const successMsgErrorLine = "Error line is correct";
    const errMsgErrorLine = "Error line is incorrect";

    var display = HUDService.getDisplayByURISpec(content.location.href);
    var outputNodes = display.querySelectorAll(".hud-msg-node");

    executeSoon(function () {
      executeSoon(function (){
        testLogEntry(outputNodes[1], "fooBazBaz",
                     { success: successMsg, err: errMsg });

        testLogEntry(outputNodes[1], "Line:",
                     { success: successMsgErrorLine, err: errMsgErrorLine });

        outputNodes = display = null;

        finishTest();
      });
    });
  }
};

