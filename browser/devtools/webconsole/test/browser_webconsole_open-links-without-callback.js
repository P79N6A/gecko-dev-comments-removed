






"use strict";

function test() {
  function* runner() {
    const TEST_EVAL_STRING = "document";
    const TEST_PAGE_URI = "http://example.com/browser/browser/devtools/" +
                          "webconsole/test/test-console.html";
    const {tab} = yield loadTab(TEST_PAGE_URI);
    const hud = yield openConsole(tab);

    hud.jsterm.execute(TEST_EVAL_STRING);

    const EXPECTED_OUTPUT = new RegExp("HTMLDocument \.+");

    let messages = yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "JS eval output",
        text: EXPECTED_OUTPUT,
        category: CATEGORY_OUTPUT,
      }],
    });

    let messageNode = messages[0].matched.values().next().value;

    
    
    
    let urlNode = messageNode.querySelector("a:not(.cm-variable)");

    let linkOpened = false;
    let oldOpenUILinkIn = window.openUILinkIn;
    window.openUILinkIn = function(aLink) {
      if (aLink == TEST_PAGE_URI) {
        linkOpened = true;
      }
    };

    EventUtils.synthesizeMouseAtCenter(urlNode, {}, hud.iframeWindow);

    ok(linkOpened, "Clicking the URL opens the desired page");
    window.openUILinkIn = oldOpenUILinkIn;
  }

  Task.spawn(runner).then(finishTest);
}
