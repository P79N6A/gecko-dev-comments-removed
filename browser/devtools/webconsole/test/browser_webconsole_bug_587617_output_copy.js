









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let HUD, outputNode;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(aHud) {
  HUD = aHud;

  
  outputNode = HUD.outputNode;

  HUD.jsterm.clearOutput();

  let controller = top.document.commandDispatcher.
                   getControllerForCommand("cmd_copy");
  is(controller.isCommandEnabled("cmd_copy"), false, "cmd_copy is disabled");

  content.console.log("Hello world! bug587617");

  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: "bug587617",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(([result]) => {
    let msg = [...result.matched][0];
    HUD.ui.output.selectMessage(msg);

    outputNode.focus();

    goUpdateCommand("cmd_copy");
    controller = top.document.commandDispatcher.getControllerForCommand("cmd_copy");
    is(controller.isCommandEnabled("cmd_copy"), true, "cmd_copy is enabled");

    let selection = HUD.iframeWindow.getSelection() + "";
    isnot(selection.indexOf("bug587617"), -1,
          "selection text includes 'bug587617'");

    waitForClipboard((str) => { return selection.trim() == str.trim(); },
      () => { goDoCommand("cmd_copy") },
      testContextMenuCopy, testContextMenuCopy);
  });
}



function testContextMenuCopy() {
  let contextMenuId = outputNode.parentNode.getAttribute("context");
  let contextMenu = HUD.ui.document.getElementById(contextMenuId);
  ok(contextMenu, "the output node has a context menu");

  let copyItem = contextMenu.querySelector("*[command='cmd_copy']");
  ok(copyItem, "the context menu on the output node has a \"Copy\" item");

  let selection = HUD.iframeWindow.getSelection() + "";

  copyItem.doCommand();

  waitForClipboard((str) => { return selection.trim() == str.trim(); },
    () => { goDoCommand("cmd_copy") },
    finishTest, finishTest);
  HUD = outputNode = null;
}

