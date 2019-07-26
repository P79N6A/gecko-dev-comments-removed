


function test() {
  TestRunner.run();
}











const URL = "data:text/html,<h1>first</h1>";
const URL2 = "data:text/html,<h1>second</h1>";

function runTests() {
  
  
  
  let win = OpenBrowserWindow();
  yield waitForLoad(win);

  
  let tab = gBrowser.selectedTab = gBrowser.addTab("about:blank");
  yield loadURI(URL);
  yield loadURI(URL2);

  
  
  yield forceWriteState();

  
  
  waitForPageShow();
  yield gBrowser.selectedBrowser.goBack();
  is(tab.linkedBrowser.currentURI.spec, URL, "correct url after going back");

  
  
  
  let state = JSON.parse(ss.getBrowserState());
  is(state.windows[0].tabs[1].index, 1, "first history entry is selected");

  
  gBrowser.removeTab(tab);
  win.close();
}

function forceWriteState() {
  const PREF = "browser.sessionstore.interval";
  const TOPIC = "sessionstore-state-write";

  Services.obs.addObserver(function observe() {
    Services.obs.removeObserver(observe, TOPIC);
    Services.prefs.clearUserPref(PREF);
    executeSoon(next);
  }, TOPIC, false);

  Services.prefs.setIntPref(PREF, 0);
}

function loadURI(aURI) {
  let browser = gBrowser.selectedBrowser;
  waitForLoad(browser);
  browser.loadURI(aURI);
}

function waitForLoad(aElement) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(next);
  }, true);
}

function waitForPageShow() {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.addMessageListener("SessionStore:pageshow", function onPageShow() {
    mm.removeMessageListener("SessionStore:pageshow", onPageShow);
    executeSoon(next);
  });
}
