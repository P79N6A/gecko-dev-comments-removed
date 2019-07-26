













const TEST_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-mixedcontent-securityerrors.html";
const LEARN_MORE_URI = "https://developer.mozilla.org/Security/MixedContent";

function test()
{
  SpecialPowers.pushPrefEnv({"set": [["security.mixed_content.block_active_content", true],
                            ["security.mixed_content.block_display_content", true]]}, blockMixedContentTest1);
}

function blockMixedContentTest1()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad(aEvent) {
    browser.removeEventListener(aEvent.type, onLoad, true);
    openConsole(null, function testSecurityErrorLogged (hud) {
      waitForMessages({
        webconsole: hud,
        messages: [
          {
            name: "Logged blocking mixed active content",
            text: "Blocked loading mixed active content \"http://example.com/\"",
            category: CATEGORY_SECURITY,
            severity: SEVERITY_ERROR
          },
          {
            name: "Logged blocking mixed passive content - image",
            text: "Blocked loading mixed active content \"http://example.com/\"",
            category: CATEGORY_SECURITY,
            severity: SEVERITY_ERROR
          },
        ],
      }).then(() => {
        testClickOpenNewTab(hud);
        
        mixedContentOverrideTest2(hud);
      });
    });
  }, true);
}

function mixedContentOverrideTest2(hud)
{
  var notification = PopupNotifications.getNotification("mixed-content-blocked", browser);
  ok(notification, "Mixed Content Doorhanger didn't appear");
  
  notification.secondaryActions[0].callback();

  waitForMessages({
    webconsole: hud,
    messages: [
      {
      name: "Logged blocking mixed active content",
      text: "Loading mixed (insecure) active content on a secure"+
        " page \"http://example.com/\"",
      category: CATEGORY_SECURITY,
      severity: SEVERITY_WARNING
    },
    {
      name: "Logged blocking mixed passive content - image",
      text: "Loading mixed (insecure) display content on a secure page"+
        " \"http://example.com/tests/image/test/mochitest/blue.png\"",
      category: CATEGORY_SECURITY,
      severity: SEVERITY_WARNING
    },
    ],
  }).then(() => {
    testClickOpenNewTab(hud);
    finishTest();
  });
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

}
