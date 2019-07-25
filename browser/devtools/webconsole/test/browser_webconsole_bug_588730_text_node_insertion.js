









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testTextNodeInsertion,
                           false);
}



function testTextNodeInsertion() {
  browser.removeEventListener("DOMContentLoaded", testTextNodeInsertion,
                              false);
  openConsole();

  let outputNode = HUDService.getHudByWindow(content).outputNode;

  let label = document.createElementNS(
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "label");
  outputNode.appendChild(label);

  let error = false;
  let listener = {
    observe: function(aMessage) {
      let messageText = aMessage.message;
      if (messageText.indexOf("JavaScript Warning") !== -1) {
        error = true;
      }
    }
  };

  let nsIConsoleServiceClass = Cc["@mozilla.org/consoleservice;1"];
  let nsIConsoleService =
    nsIConsoleServiceClass.getService(Ci.nsIConsoleService);
  nsIConsoleService.registerListener(listener);

  
  label.appendChild(document.createTextNode("foo"));

  executeSoon(function() {
    nsIConsoleService.unregisterListener(listener);
    ok(!error, "no error when adding text nodes as children of labels");

    finishTest();
  });
}

