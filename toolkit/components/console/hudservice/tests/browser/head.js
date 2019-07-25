





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "HUDService", function () {
  Cu.import("resource:///modules/HUDService.jsm");
  try {
    return HUDService;
  }
  catch (ex) {
    dump(ex + "\n");
  }
});

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

let win = gBrowser.selectedBrowser;

function addTab(aURL)
{
  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;
  tab = gBrowser.selectedTab;
  browser = gBrowser.getBrowserForTab(tab);
}













function testLogEntry(aOutputNode, aMatchString, aSuccessErrObj, aOnlyVisible,
                      aFailIfFound)
{
  let found = true;
  let notfound = false;
  let foundMsg = aSuccessErrObj.success;
  let notfoundMsg = aSuccessErrObj.err;

  if (aFailIfFound) {
    found = false;
    notfound = true;
    foundMsg = aSuccessErrObj.success;
    notfoundMsg = aSuccessErrObj.err;
  }

  let selector = ".hud-msg-node";
  
  if (aOnlyVisible) {
    selector += ":not(.hud-filtered-by-type)";
  }

  let msgs = aOutputNode.querySelectorAll(selector);
  for (let i = 0, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(aMatchString);
    if (message > -1) {
      ok(found, foundMsg);
      return;
    }
  }
  ok(notfound, notfoundMsg);
}

function openConsole()
{
  HUDService.activateHUDForContext(tab);
}

function finishTest()
{
  finish();
}

function tearDown()
{
  try {
    HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  }
  catch (ex) {
    log(ex);
  }
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
  tab = browser = hudId = hud = filterBox = outputNode = cs = null;
}

registerCleanupFunction(tearDown);

waitForExplicitFinish();




