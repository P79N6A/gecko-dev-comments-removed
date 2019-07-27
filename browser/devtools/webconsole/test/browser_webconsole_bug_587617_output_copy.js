









"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let HUD, outputNode;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  yield consoleOpened(hud);
  yield testContextMenuCopy();

  HUD = outputNode = null;
});

function consoleOpened(aHud) {
  HUD = aHud;

  let deferred = promise.defer();

  
  outputNode = HUD.outputNode;

  HUD.jsterm.clearOutput();

  let controller = top.document.commandDispatcher
                               .getControllerForCommand("cmd_copy");
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
    controller = top.document.commandDispatcher
                             .getControllerForCommand("cmd_copy");
    is(controller.isCommandEnabled("cmd_copy"), true, "cmd_copy is enabled");

    
    
    let selection = (HUD.iframeWindow.getSelection() + "")
      .replace(/\r?\n|\r/g, " ");
    isnot(selection.indexOf("bug587617"), -1,
          "selection text includes 'bug587617'");

    waitForClipboard((str) => {
      return selection.trim() == str.trim();
    }, () => {
      goDoCommand("cmd_copy");
    }, deferred.resolve, deferred.resolve);
  });
  return deferred.promise;
}



function testContextMenuCopy() {
  let deferred = promise.defer();

  let contextMenuId = outputNode.parentNode.getAttribute("context");
  let contextMenu = HUD.ui.document.getElementById(contextMenuId);
  ok(contextMenu, "the output node has a context menu");

  let copyItem = contextMenu.querySelector("*[command='cmd_copy']");
  ok(copyItem, "the context menu on the output node has a \"Copy\" item");

  
  
  let selection = (HUD.iframeWindow.getSelection() + "")
    .replace(/\r?\n|\r/g, " ");

  copyItem.doCommand();

  waitForClipboard((str) => {
    return selection.trim() == str.trim();
  }, () => {
    goDoCommand("cmd_copy");
  }, deferred.resolve, deferred.resolve);
  HUD = outputNode = null;

  return deferred.promise;
}
