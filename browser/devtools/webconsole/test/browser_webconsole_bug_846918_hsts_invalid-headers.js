 



const TEST_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-bug-846918-hsts-invalid-headers.html";
const HSTS_INVALID_HEADER_MSG = "The site specified an invalid Strict-Transport-Security header.";
const LEARN_MORE_URI = "https://developer.mozilla.org/docs/Security/HTTP_Strict_Transport_Security";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad(aEvent) {
    browser.removeEventListener(aEvent.type, onLoad, true);
    openConsole(null, function testHSTSErrorLogged (hud) {
      waitForMessages({
        webconsole: hud,
        messages: [
          {
            name: "Invalid HSTS header error displayed successfully",
            text: HSTS_INVALID_HEADER_MSG,
            category: CATEGORY_SECURITY,
            severity: SEVERITY_WARNING,
            objects: true,
          },
        ],
      }).then((results) => testClickOpenNewTab(hud, results));
    });
  }, true);
}

function testClickOpenNewTab(hud, results) {
  let warningNode = results[0].clickableElements[0];
  ok(warningNode, "link element");
  ok(warningNode.classList.contains("learn-more-link"), "link class name");

  
  
  let linkOpened = false;
  let oldOpenUILinkIn = window.openUILinkIn;
  window.openUILinkIn = function(aLink) {
    if (aLink == LEARN_MORE_URI) {
      linkOpened = true;
    }
  }

  EventUtils.synthesizeMouse(warningNode, 2, 2, {},
                             warningNode.ownerDocument.defaultView);
  ok(linkOpened, "Clicking the Learn More Warning node opens the desired page");
  window.openUILinkIn = oldOpenUILinkIn;

  finishTest();
}
