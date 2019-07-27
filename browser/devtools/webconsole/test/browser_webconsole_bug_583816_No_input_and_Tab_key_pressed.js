




"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/browser/test-console.html";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  testCompletion(hud);
});

function testCompletion(hud) {
  let jsterm = hud.jsterm;
  let input = jsterm.inputNode;

  jsterm.setInputValue("");
  EventUtils.synthesizeKey("VK_TAB", {});
  is(jsterm.completeNode.value, "<- no result", "<- no result - matched");
  is(input.value, "", "inputnode is empty - matched");
  is(input.getAttribute("focused"), "true", "input is still focused");

  
  jsterm.setInputValue("window.Bug583816");
  EventUtils.synthesizeKey("VK_TAB", {});
  is(jsterm.completeNode.value, "                <- no result",
     "completenode content - matched");
  is(input.value, "window.Bug583816", "inputnode content - matched");
  is(input.getAttribute("focused"), "true", "input is still focused");
}
