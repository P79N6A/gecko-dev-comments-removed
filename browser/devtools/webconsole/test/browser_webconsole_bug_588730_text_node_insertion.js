






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testTextNodeInsertion);
  }, true);
}



function testTextNodeInsertion(hud) {
  let outputNode = hud.outputNode;

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

  Services.console.registerListener(listener);

  
  label.appendChild(document.createTextNode("foo"));

  executeSoon(function() {
    Services.console.unregisterListener(listener);
    ok(!error, "no error when adding text nodes as children of labels");

    finishTest();
  });
}

