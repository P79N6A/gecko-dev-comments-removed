






"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug_1050691_click_function_to_source.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();

  yield testWithoutDebuggerOpen(hud);

  
  let debuggerPanel = yield openDebugger();
  
  yield openConsole();
  yield testWithDebuggerOpen(hud, debuggerPanel);
});

function* testWithoutDebuggerOpen(hud) {
  let clickable = yield printFunction(hud);
  let onVariablesViewOpen = hud.jsterm.once("variablesview-fetched");
  synthesizeClick(clickable, hud);
  return onVariablesViewOpen;
}

function* testWithDebuggerOpen(hud, debuggerPanel) {
  let clickable = yield printFunction(hud);
  let panelWin = debuggerPanel.panelWin;
  let onEditorLocationSet = panelWin.once(panelWin.EVENTS.EDITOR_LOCATION_SET);
  synthesizeClick(clickable, hud);
  yield onEditorLocationSet;
  ok(isDebuggerCaretPos(debuggerPanel, 7),
    "Clicking on a function should go to its source in the debugger view");
}

function synthesizeClick(clickable, hud) {
  EventUtils.synthesizeMouse(clickable, 2, 2, {}, hud.iframeWindow);
}

let printFunction = Task.async(function* (hud) {
  hud.jsterm.clearOutput();
  content.wrappedJSObject.foo();
  let [result] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });
  let msg = [...result.matched][0];
  let clickable = msg.querySelector("a");
  ok(clickable, "clickable item for object should exist");
  return clickable;
});
