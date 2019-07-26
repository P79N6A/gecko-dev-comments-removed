











const TEST_HTTPS_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-bug-737873-mixedcontent.html";

function test() {
  addTab("data:text/html,Web Console basic network logging test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener("load", onLoad, true);
  openConsole(null, testMixedContent);
}

function testMixedContent(hud) {
  content.location = TEST_HTTPS_URI;
  var aOutputNode = hud.outputNode;

  waitForSuccess(
    {
      name: "mixed content warning displayed successfully",
      validatorFn: function() {
        return ( aOutputNode.querySelector(".webconsole-mixed-content") );
      },

      successFn: function() {

        
        let node = aOutputNode.querySelector(".webconsole-mixed-content");
        ok(testSeverity(node), "Severity type is SEVERITY_WARNING.");

        
        let warningNode = aOutputNode.querySelector(".webconsole-mixed-content-link");
        is(warningNode.value, "[Mixed Content]", "Message text is accurate." );
        testClickOpenNewTab(warningNode);

        finishTest();
      },

      failureFn: finishTest,
    }
  );

}

function testSeverity(node) {
  let linkNode = node.parentNode;
  let msgNode = linkNode.parentNode;
  let bodyNode = msgNode.parentNode;
  let finalNode = bodyNode.parentNode;

  return finalNode.classList.contains("webconsole-msg-warn");
}

function testClickOpenNewTab(warningNode) {
  
  let linkOpened = false;
  let oldOpenUILinkIn = window.openUILinkIn;

  window.openUILinkIn = function(aLink) {
   if (aLink == "https://developer.mozilla.org/en/Security/MixedContent");
     linkOpened = true;
  }

  EventUtils.synthesizeMouse(warningNode, 2, 2, {},
                             warningNode.ownerDocument.defaultView);

  ok(linkOpened, "Clicking the Mixed Content Warning node opens the desired page");

  window.openUILinkIn = oldOpenUILinkIn;
}
