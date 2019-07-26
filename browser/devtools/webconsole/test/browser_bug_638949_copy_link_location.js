




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
  "test/test-console.html?_date=" + Date.now();
const COMMAND_NAME = "consoleCmd_copyURL";
const CONTEXT_MENU_ID = "#menu_copyURL";

let HUD = null;
let output = null;
let menu = null;

function test() {
  registerCleanupFunction(() => {
    HUD = output = menu = null;
  });

  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    openConsole(null, function (aHud) {
      HUD = aHud;
      output = aHud.outputNode;
      menu = HUD.iframeWindow.document.getElementById("output-contextmenu");

      executeSoon(testWithoutNetActivity);
    });
  }, true);
}


function isEnabled() {
  let controller = top.document.commandDispatcher
                   .getControllerForCommand(COMMAND_NAME);
  return controller && controller.isCommandEnabled(COMMAND_NAME);
}

function testWithoutNetActivity() {
  HUD.jsterm.clearOutput();
  content.console.log("bug 638949");

  
  
  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: "bug 638949",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(onConsoleMessage);
}

function onConsoleMessage(aResults) {
  output.focus();
  let message = [...aResults[0].matched][0];

  goUpdateCommand(COMMAND_NAME);
  ok(!isEnabled(), COMMAND_NAME + "is disabled");

  
  
  waitForContextMenu(menu, message, () => {
    let isHidden = menu.querySelector(CONTEXT_MENU_ID).hidden;
    ok(isHidden, CONTEXT_MENU_ID + " is hidden");
  }, testWithNetActivity);
}

function testWithNetActivity() {
  HUD.jsterm.clearOutput();
  content.location.reload(); 

  
  
  
  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: "test-console.html",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    }],
  }).then(onNetworkMessage);
}

function onNetworkMessage(aResults) {
  output.focus();
  let message = [...aResults[0].matched][0];
  HUD.ui.output.selectMessage(message);

  goUpdateCommand(COMMAND_NAME);
  ok(isEnabled(), COMMAND_NAME + " is enabled");

  info("expected clipboard value: " + message.url);

  waitForClipboard((aData) => { return aData.trim() == message.url; },
    () => { goDoCommand(COMMAND_NAME) },
    testMenuWithNetActivity, testMenuWithNetActivity);

  function testMenuWithNetActivity() {
    
    
    waitForContextMenu(menu, message, () => {
      let isVisible = !menu.querySelector(CONTEXT_MENU_ID).hidden;
      ok(isVisible, CONTEXT_MENU_ID + " is visible");
    }, finishTest);
  }
}

