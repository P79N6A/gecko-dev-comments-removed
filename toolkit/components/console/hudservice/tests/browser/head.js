





































Cu.import("resource://gre/modules/HUDService.jsm");

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

function openConsole()
{
  HUDService.activateHUDForContext(tab);
}

function closeConsole()
{
  HUDService.deactivateHUDForContext(tab);
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




