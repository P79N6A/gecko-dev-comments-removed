










const TEST_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-mixedcontent-securityerrors.html";
const LEARN_MORE_URI = "https://developer.mozilla.org/Security/MixedContent";

function test()
{
  SpecialPowers.pushPrefEnv({"set":
      [["security.mixed_content.block_active_content", false],
       ["security.mixed_content.block_display_content", false]
  ]}, loadingMixedContentTest);
}

function loadingMixedContentTest()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad(aEvent) {
    browser.removeEventListener(aEvent.type, onLoad, true);
    openConsole(null, function testSecurityErrorLogged (hud) {
      waitForMessages({
        webconsole: hud,
        messages: [
          {
          name: "Logged mixed active content",
          text: "Loading mixed (insecure) active content on a secure page \"http://example.com/\"",
          category: CATEGORY_SECURITY,
          severity: SEVERITY_WARNING
        },
        {
          name: "Logged mixed passive content - image",
          text: "Loading mixed (insecure) display content on a secure page \"http://example.com/tests/image/test/mochitest/blue.png\"",
          category: CATEGORY_SECURITY,
          severity: SEVERITY_WARNING
        },
        ],
      }).then(() => testClickOpenNewTab(hud));
    });
  }, true);
}

function testClickOpenNewTab(hud) {
  let warningNode = hud.outputNode.querySelector(".webconsole-learn-more-link");

    
    
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
