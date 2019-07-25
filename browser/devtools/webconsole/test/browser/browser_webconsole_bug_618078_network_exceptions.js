



























const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-bug-618078-network-exceptions.html";

let testEnded = false;

let TestObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function test_observe(aSubject)
  {
    if (testEnded || !(aSubject instanceof Ci.nsIScriptError)) {
      return;
    }

    is(aSubject.category, "content javascript", "error category");

    if (aSubject.category == "content javascript") {
      executeSoon(checkOutput);
    }
    else {
      testEnd();
    }
  }
};

function checkOutput()
{
  if (testEnded) {
    return;
  }

  let textContent = hud.outputNode.textContent;
  isnot(textContent.indexOf("bug618078exception"), -1,
        "exception message");

  testEnd();
}

function testEnd()
{
  if (testEnded) {
    return;
  }

  testEnded = true;
  Services.console.unregisterListener(TestObserver);
  finishTest();
}

function test()
{
  addTab("data:text/html,Web Console test for bug 618078");

  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    openConsole();

    let hudId = HUDService.getHudIdByWindow(content);
    hud = HUDService.hudReferences[hudId];

    Services.console.registerListener(TestObserver);
    registerCleanupFunction(testEnd);

    executeSoon(function() {
      content.location = TEST_URI;
    });
  }, true);
}

