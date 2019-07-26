




let tempScope = {};
Cu.import("resource:///modules/HUDService.jsm", tempScope);
let HUDService = tempScope.HUDService;
Cu.import("resource://gre/modules/devtools/WebConsoleUtils.jsm", tempScope);
let WebConsoleUtils = tempScope.WebConsoleUtils;
Cu.import("resource:///modules/devtools/gDevTools.jsm", tempScope);
let gDevTools = tempScope.gDevTools;
Cu.import("resource:///modules/devtools/Target.jsm", tempScope);
let TargetFactory = tempScope.TargetFactory;
Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
let console = tempScope.console;

const WEBCONSOLE_STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let WCU_l10n = new WebConsoleUtils.l10n(WEBCONSOLE_STRINGS_URI);

function log(aMsg)
{
  dump("*** WebConsoleTest: " + aMsg + "\n");
}

function pprint(aObj)
{
  for (let prop in aObj) {
    if (typeof aObj[prop] == "function") {
      log("function " + prop);
    }
    else {
      log(prop + ": " + aObj[prop]);
    }
  }
}

let tab, browser, hudId, hud, hudBox, filterBox, outputNode, cs;

function addTab(aURL)
{
  gBrowser.selectedTab = gBrowser.addTab(aURL);
  tab = gBrowser.selectedTab;
  browser = gBrowser.getBrowserForTab(tab);
}

function afterAllTabsLoaded(callback, win) {
  win = win || window;

  let stillToLoad = 0;

  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let browser = win.gBrowser.tabs[a].linkedBrowser;
    if (browser.contentDocument.readyState != "complete") {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad)
    callback();
}

















function testLogEntry(aOutputNode, aMatchString, aMsg, aOnlyVisible,
                      aFailIfFound, aClass)
{
  let selector = ".hud-msg-node";
  
  if (aOnlyVisible) {
    selector += ":not(.hud-filtered-by-type)";
  }
  if (aClass) {
    selector += "." + aClass;
  }

  let msgs = aOutputNode.querySelectorAll(selector);
  let found = false;
  for (let i = 0, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(aMatchString);
    if (message > -1) {
      found = true;
      break;
    }

    
    let labels = msgs[i].querySelectorAll("label");
    for (let j = 0; j < labels.length; j++) {
      if (labels[j].getAttribute("value").indexOf(aMatchString) > -1) {
        found = true;
        break;
      }
    }
  }

  is(found, !aFailIfFound, aMsg);
}







function findLogEntry(aString)
{
  testLogEntry(outputNode, aString, "found " + aString);
}











function openConsole(aTab, aCallback = function() { })
{
  let target = TargetFactory.forTab(aTab || tab);
  gDevTools.showToolbox(target, "webconsole").then(function(toolbox) {
    aCallback(toolbox.getCurrentPanel().hud);
  });
}











function closeConsole(aTab, aCallback = function() { })
{
  let target = TargetFactory.forTab(aTab || tab);
  let toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    let panel = toolbox.getPanel("webconsole");
    if (panel) {
      let hudId = panel.hud.hudId;
      toolbox.destroy().then(function() {
        executeSoon(aCallback.bind(null, hudId));
      }).then(null, console.error);
    }
    else {
      toolbox.destroy().then(aCallback.bind(null));
    }
  }
  else {
    aCallback();
  }
}
















function waitForOpenContextMenu(aContextMenu, aOptions) {
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;
  let targetElement = aOptions.target;

  if (!aContextMenu) {
    ok(false, "Can't get a context menu.");
    aOptions.failureFn();
    return;
  }
  if (!targetElement) {
    ok(false, "Can't get a target element.");
    aOptions.failureFn();
    return;
  }

  function onPopupShown() {
    aContextMenu.removeEventListener("popupshown", onPopupShown);
    clearTimeout(onTimeout);
    aOptions.successFn();
  }


  aContextMenu.addEventListener("popupshown", onPopupShown);

  let onTimeout = setTimeout(function(){
    aContextMenu.removeEventListener("popupshown", onPopupShown);
    aOptions.failureFn();
  }, timeout);

  
  let eventDetails = { type : "contextmenu", button : 2};
  EventUtils.synthesizeMouse(targetElement, 2, 2,
                             eventDetails, targetElement.ownerDocument.defaultView);
}

function finishTest()
{
  browser = hudId = hud = filterBox = outputNode = cs = null;

  let hud = HUDService.getHudByWindow(content);
  if (!hud) {
    finish();
    return;
  }
  if (hud.jsterm) {
    hud.jsterm.clearOutput(true);
  }

  closeConsole(hud.target.tab, finish);

  hud = null;
}

function tearDown()
{
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.closeToolbox(target);
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
  WCU_l10n = tab = browser = hudId = hud = filterBox = outputNode = cs = null;
}

registerCleanupFunction(tearDown);

waitForExplicitFinish();






















function waitForSuccess(aOptions)
{
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;

  function wait(validatorFn, successFn, failureFn)
  {
    if ((Date.now() - start) > timeout) {
      
      ok(false, "Timed out while waiting for: " + aOptions.name);
      failureFn(aOptions);
      return;
    }

    if (validatorFn(aOptions)) {
      ok(true, aOptions.name);
      successFn();
    }
    else {
      setTimeout(function() wait(validatorFn, successFn, failureFn), 100);
    }
  }

  wait(aOptions.validatorFn, aOptions.successFn, aOptions.failureFn);
}

function openInspector(aCallback, aTab = gBrowser.selectedTab)
{
  let target = TargetFactory.forTab(aTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    aCallback(toolbox.getCurrentPanel());
  });
}
