













const TEST_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-mixedcontent-securityerrors.html";
const LEARN_MORE_URI = "https://developer.mozilla.org/docs/Security/MixedContent";


let test = asyncTest(function* () {
  yield pushPrefEnv();

  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();

  let results = yield waitForMessages({
    webconsole: hud,
    messages: [
      {
        name: "Logged blocking mixed active content",
        text: "Blocked loading mixed active content \"http://example.com/\"",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_ERROR,
        objects: true,
      },
      {
        name: "Logged blocking mixed passive content - image",
        text: "Blocked loading mixed active content \"http://example.com/\"",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_ERROR,
        objects: true,
      },
    ],
  });

  testClickOpenNewTab(hud, results[0]);

  let results2 = yield mixedContentOverrideTest2(hud, browser);

  testClickOpenNewTab(hud, results2[0]);
});

function pushPrefEnv()
{
  let deferred = promise.defer();
  let options = {"set": [["security.mixed_content.block_active_content", true],
                            ["security.mixed_content.block_display_content", true]]};
  SpecialPowers.pushPrefEnv(options, deferred.resolve);
  return deferred.promise;
}

function mixedContentOverrideTest2(hud, browser)
{
  var notification = PopupNotifications.getNotification("bad-content", browser);
  ok(notification, "Mixed Content Doorhanger did appear");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked");
  
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
  notification.remove();

  return waitForMessages({
    webconsole: hud,
    messages: [
      {
        name: "Logged blocking mixed active content",
        text: "Loading mixed (insecure) active content on a secure"+
          " page \"http://example.com/\"",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
      {
        name: "Logged blocking mixed passive content - image",
        text: "Loading mixed (insecure) display content on a secure page"+
          " \"http://example.com/tests/image/test/mochitest/blue.png\"",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
    ],
  });
}

function testClickOpenNewTab(hud, match) {
  let warningNode = match.clickableElements[0];
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
}
