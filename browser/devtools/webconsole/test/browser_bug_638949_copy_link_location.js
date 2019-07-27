







"use strict";

let test = asyncTest(function* () {
  const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
    "test/test-console.html?_date=" + Date.now();
  const COMMAND_NAME = "consoleCmd_copyURL";
  const CONTEXT_MENU_ID = "#menu_copyURL";

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("devtools.webconsole.filter.networkinfo");
  });

  Services.prefs.setBoolPref("devtools.webconsole.filter.networkinfo", true);

  yield loadTab(TEST_URI);
  let hud = yield openConsole();
  let output = hud.outputNode;
  let menu = hud.iframeWindow.document.getElementById("output-contextmenu");

  hud.jsterm.clearOutput();
  content.console.log("bug 638949");

  
  
  let [result] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug 638949",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  output.focus();
  let message = [...result.matched][0];

  goUpdateCommand(COMMAND_NAME);
  ok(!isEnabled(), COMMAND_NAME + " is disabled");

  
  
  message.scrollIntoView();

  yield waitForContextMenu(menu, message, () => {
    let isHidden = menu.querySelector(CONTEXT_MENU_ID).hidden;
    ok(isHidden, CONTEXT_MENU_ID + " is hidden");
  });

  hud.jsterm.clearOutput();
  
  content.location.reload();

  
  
  
  [result] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "test-console.html",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    }],
  });

  output.focus();
  message = [...result.matched][0];
  hud.ui.output.selectMessage(message);

  goUpdateCommand(COMMAND_NAME);
  ok(isEnabled(), COMMAND_NAME + " is enabled");

  info("expected clipboard value: " + message.url);

  let deferred = promise.defer();

  waitForClipboard((aData) => {
    return aData.trim() == message.url;
  }, () => {
    goDoCommand(COMMAND_NAME);
  }, () => {
    deferred.resolve(null);
  }, () => {
    deferred.reject(null);
  });

  yield deferred.promise;

  
  
  message.scrollIntoView();

  yield waitForContextMenu(menu, message, () => {
    let isVisible = !menu.querySelector(CONTEXT_MENU_ID).hidden;
    ok(isVisible, CONTEXT_MENU_ID + " is visible");
  });

  
  function isEnabled() {
    let controller = top.document.commandDispatcher
                     .getControllerForCommand(COMMAND_NAME);
    return controller && controller.isCommandEnabled(COMMAND_NAME);
  }
});
