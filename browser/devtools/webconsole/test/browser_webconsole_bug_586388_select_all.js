









const TEST_URI = "http://example.com/";

function test() {
  let hud;

  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testSelectionWhenMovingBetweenBoxes);
  }, true);

  function testSelectionWhenMovingBetweenBoxes(aHud) {
    hud = aHud;
    let jsterm = hud.jsterm;

    
    jsterm.clearOutput();
    jsterm.execute("1 + 2");
    jsterm.execute("3 + 4");
    jsterm.execute("5 + 6");

    waitForMessages({
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
    }).then(performTestsAfterOutput);
  }

  function performTestsAfterOutput() {
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

    finishTest();
  }
}
