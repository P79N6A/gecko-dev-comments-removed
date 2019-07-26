




let tempScope = {};
Cu.import("resource:///modules/HUDService.jsm", tempScope);
let HUDService = tempScope.HUDService;
Cu.import("resource:///modules/WebConsoleUtils.jsm", tempScope);
let WebConsoleUtils = tempScope.WebConsoleUtils;

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
  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;
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











function openConsole(aTab, aCallback)
{
  function onWebConsoleOpen(aSubject, aTopic)
  {
    if (aTopic == "web-console-created") {
      Services.obs.removeObserver(onWebConsoleOpen, "web-console-created");
      aSubject.QueryInterface(Ci.nsISupportsString);
      let hud = HUDService.getHudReferenceById(aSubject.data);
      executeSoon(aCallback.bind(null, hud));
    }
  }

  if (aCallback) {
    Services.obs.addObserver(onWebConsoleOpen, "web-console-created", false);
  }

  HUDService.activateHUDForContext(aTab || tab);
}











function closeConsole(aTab, aCallback)
{
  function onWebConsoleClose(aSubject, aTopic)
  {
    if (aTopic == "web-console-destroyed") {
      Services.obs.removeObserver(onWebConsoleClose, "web-console-destroyed");
      aSubject.QueryInterface(Ci.nsISupportsString);
      let hudId = aSubject.data;
      executeSoon(aCallback.bind(null, hudId));
    }
  }

  if (aCallback) {
    Services.obs.addObserver(onWebConsoleClose, "web-console-destroyed", false);
  }

  HUDService.deactivateHUDForContext(aTab || tab);
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
  hud.jsterm.clearOutput(true);

  closeConsole(hud.tab, finish);

  hud = null;
}

function tearDown()
{
  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
  tab = browser = hudId = hud = filterBox = outputNode = cs = null;
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
