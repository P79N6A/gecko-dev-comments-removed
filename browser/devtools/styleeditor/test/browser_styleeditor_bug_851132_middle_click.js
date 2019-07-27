

"use strict";




const TESTCASE_URI = TEST_BASE_HTTP + "four.html";

add_task(function* () {
  let { ui } = yield openStyleEditorForURL(TESTCASE_URI);
  gBrowser.tabContainer.addEventListener("TabOpen", onTabAdded, false);

  yield ui.editors[0].getSourceEditor();
  info("first editor selected");

  info("Left-clicking on the second editor link.");
  yield clickOnStyleSheetLink(ui.editors[1], 0);

  info("Waiting for the second editor to be selected.");
  let editor = yield ui.once("editor-selected");

  ok(editor.sourceEditor.hasFocus(),
     "Left mouse click gave second editor focus.");

  
  info("Middle clicking on the third editor link.");
  yield clickOnStyleSheetLink(ui.editors[2], 1);
});









function* clickOnStyleSheetLink(editor, button) {
  let window = editor._window;
  let link = editor.summary.querySelector(".stylesheet-name");

  info("Waiting for focus.");
  yield SimpleTest.promiseFocus(window);

  info("Pressing button " + button + " on style sheet name link.");
  EventUtils.synthesizeMouseAtCenter(link, { button }, window);
}

function onTabAdded() {
  ok(false, "middle mouse click has opened a new tab");
}

registerCleanupFunction(function () {
  gBrowser.tabContainer.removeEventListener("TabOpen", onTabAdded, false);
});
