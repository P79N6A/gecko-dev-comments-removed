













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

  yield testClickOpenNewTab(hud, results[0]);

  let results2 = yield mixedContentOverrideTest2(hud, browser);

  yield testClickOpenNewTab(hud, results2[0]);
});

function pushPrefEnv()
{
  let deferred = promise.defer();
  let options = {"set": [["security.mixed_content.block_active_content", true],
                            ["security.mixed_content.block_display_content", true]]};
  SpecialPowers.pushPrefEnv(options, deferred.resolve);
  return deferred.promise;
}

function waitForNotificationShown(notification, callback)
{
  if (PopupNotifications.panel.state == "open") {
    executeSoon(callback);
    return;
  }
  PopupNotifications.panel.addEventListener("popupshown", function onShown(e) {
    PopupNotifications.panel.removeEventListener("popupshown", onShown);
    callback();
  }, false);
  notification.reshow();
}

function mixedContentOverrideTest2(hud, browser)
{
  var notification = PopupNotifications.getNotification("bad-content", browser);
  ok(notification, "Mixed Content Doorhanger did appear");
  let deferred = promise.defer();
  waitForNotificationShown(notification, () => {
    afterNotificationShown(hud, notification, deferred);
  });
  return deferred.promise;
}

function afterNotificationShown(hud, notification, deferred)
{
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked");
  
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
  notification.remove();

  waitForMessages({
    webconsole: hud,
    messages: [
      {
        name: "Logged blocking mixed active content",
        text: "Loading mixed (insecure) active content \"http://example.com/\"" +
          " on a secure page",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
      {
        name: "Logged blocking mixed passive content - image",
        text: "Loading mixed (insecure) display content" +
          " \"http://example.com/tests/image/test/mochitest/blue.png\"" +
          " on a secure page",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
    ],
  }).then(msgs => deferred.resolve(msgs), Cu.reportError);
}

function testClickOpenNewTab(hud, match) {
  let warningNode = match.clickableElements[0];
  ok(warningNode, "link element");
  ok(warningNode.classList.contains("learn-more-link"), "link class name");
  return simulateMessageLinkClick(warningNode, LEARN_MORE_URI);
}
