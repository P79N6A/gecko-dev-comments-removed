









const TEST_URI = "http://example.com/";

"use strict";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  yield testSelectionWhenMovingBetweenBoxes(hud);
  performTestsAfterOutput(hud);
})

let testSelectionWhenMovingBetweenBoxes = Task.async(function *(aHud) {
  let hud = aHud;
  let jsterm = hud.jsterm;

  
  jsterm.clearOutput();
  yield jsterm.execute("1 + 2");
  yield jsterm.execute("3 + 4");
  yield jsterm.execute("5 + 6");

  return waitForMessages({
    webconsole: hud,
    messages: [{
      text: "3",
      category: CATEGORY_OUTPUT,
    },
    {
      text: "7",
      category: CATEGORY_OUTPUT,
    },
    {
      text: "11",
      category: CATEGORY_OUTPUT,
    }],
  });
});

function performTestsAfterOutput(aHud) {
  let hud = aHud;
  let outputNode = hud.outputNode;

  ok(outputNode.childNodes.length >= 3, "the output node has children after " +
     "executing some JavaScript");

  
  
  let commandController = hud.ui._commandController;
  ok(commandController != null, "the window has a command controller object");

  commandController.selectAll();

  let selectedCount = hud.ui.output.getSelectedMessages().length;
  is(selectedCount, outputNode.childNodes.length,
     "all console messages are selected after performing a regular browser " +
     "select-all operation");

  hud.iframeWindow.getSelection().removeAllRanges();

  
  
  let contextMenuId = outputNode.parentNode.getAttribute("context");
  let contextMenu = hud.ui.document.getElementById(contextMenuId);
  ok(contextMenu != null, "the output node has a context menu");

  let selectAllItem = contextMenu.querySelector("*[command='cmd_selectAll']");
  ok(selectAllItem != null,
     "the context menu on the output node has a \"Select All\" item");

  outputNode.focus();

  selectAllItem.doCommand();

  selectedCount = hud.ui.output.getSelectedMessages().length;
  is(selectedCount, outputNode.childNodes.length,
     "all console messages are selected after performing a select-all " +
     "operation from the context menu");

  hud.iframeWindow.getSelection().removeAllRanges();
}
