


"use strict";



const TESTCASE_URI = TEST_BASE_HTTP + "four.html";

add_task(function* () {
  let { panel, ui } = yield openStyleEditorForURL(TESTCASE_URI);

  info("Waiting for source editor to load.");
  yield ui.editors[0].getSourceEditor();

  let selected = ui.once("editor-selected");

  info("Testing keyboard navigation on the sheet list.");
  testKeyboardNavigation(ui.editors[0], panel);

  info("Waiting for editor #2 to be selected due to keyboard navigation.");
  yield selected;

  ok(ui.editors[2].sourceEditor.hasFocus(), "Editor #2 has focus.");
});

function getStylesheetNameLinkFor(aEditor)
{
  return aEditor.summary.querySelector(".stylesheet-name");
}

function testKeyboardNavigation(aEditor, panel)
{
  let panelWindow = panel.panelWindow;
  let ui = panel.UI;
  waitForFocus(function () {
    let summary = aEditor.summary;
    EventUtils.synthesizeMouseAtCenter(summary, {}, panelWindow);

    let item = getStylesheetNameLinkFor(ui.editors[0]);
    is(panelWindow.document.activeElement, item,
       "editor 0 item is the active element");

    EventUtils.synthesizeKey("VK_DOWN", {}, panelWindow);
    item = getStylesheetNameLinkFor(ui.editors[1]);
    is(panelWindow.document.activeElement, item,
       "editor 1 item is the active element");

    EventUtils.synthesizeKey("VK_HOME", {}, panelWindow);
    item = getStylesheetNameLinkFor(ui.editors[0]);
    is(panelWindow.document.activeElement, item,
       "fist editor item is the active element");

    EventUtils.synthesizeKey("VK_END", {}, panelWindow);
    item = getStylesheetNameLinkFor(ui.editors[3]);
    is(panelWindow.document.activeElement, item,
       "last editor item is the active element");

    EventUtils.synthesizeKey("VK_UP", {}, panelWindow);
    item = getStylesheetNameLinkFor(ui.editors[2]);
    is(panelWindow.document.activeElement, item,
       "editor 2 item is the active element");

    EventUtils.synthesizeKey("VK_RETURN", {}, panelWindow);
    
  }, panelWindow);
}
