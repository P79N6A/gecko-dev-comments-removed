










const TEST_URI = "https://example.com/browser/browser/devtools/webconsole/test/test-mixedcontent-securityerrors.html";
const LEARN_MORE_URI = "https://developer.mozilla.org/docs/Security/MixedContent";

let test = asyncTest(function* () {
  yield pushPrefEnv();

  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  let results = yield waitForMessages({
    webconsole: hud,
    messages: [
      {
        name: "Logged mixed active content",
        text: "Loading mixed (insecure) active content \"http://example.com/\" on a secure page",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
      {
        name: "Logged mixed passive content - image",
        text: "Loading mixed (insecure) display content \"http://example.com/tests/image/test/mochitest/blue.png\" on a secure page",
        category: CATEGORY_SECURITY,
        severity: SEVERITY_WARNING,
        objects: true,
      },
    ],
  });

  yield testClickOpenNewTab(hud, results);
});

function pushPrefEnv()
{
  let deferred = promise.defer();
  let options = {"set":
      [["security.mixed_content.block_active_content", false],
       ["security.mixed_content.block_display_content", false]
  ]};
  SpecialPowers.pushPrefEnv(options, deferred.resolve);
  return deferred.promise;
}

function testClickOpenNewTab(hud, results) {
  let warningNode = results[0].clickableElements[0];
  ok(warningNode, "link element");
  ok(warningNode.classList.contains("learn-more-link"), "link class name");
  return simulateMessageLinkClick(warningNode, LEARN_MORE_URI);
}
