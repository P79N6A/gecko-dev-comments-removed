






"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();

  let [result] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "Dolske Digs Bacon",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  let msg = [...result.matched][0];
  let outputItem = msg.querySelector(".message-body");
  ok(outputItem, "found a logged message");

  let inputNode = hud.jsterm.inputNode;
  ok(inputNode.getAttribute("focused"), "input node is focused, first");

  let lostFocus = () => {
    inputNode.removeEventListener("blur", lostFocus);
    info("input node lost focus");
  };

  inputNode.addEventListener("blur", lostFocus);

  document.getElementById("urlbar").click();

  ok(!inputNode.getAttribute("focused"), "input node is not focused");

  EventUtils.sendMouseEvent({type: "click"}, hud.outputNode);

  ok(inputNode.getAttribute("focused"), "input node is focused, second time");

  
  EventUtils.sendMouseEvent({type: "mousedown", clientX: 3, clientY: 4},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", clientX: 15, clientY: 5},
    outputItem);

  todo(!inputNode.getAttribute("focused"), "input node is not focused after drag");
});

