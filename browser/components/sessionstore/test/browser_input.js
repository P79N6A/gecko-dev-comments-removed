


const URL = "http://mochi.test:8888/browser/" +
            "browser/components/sessionstore/test/browser_input_sample.html";

function test() {
  TestRunner.run();
}







function runTests() {
  
  
  
  let win = OpenBrowserWindow();
  yield waitForLoad(win);

  
  let tab = gBrowser.selectedTab = gBrowser.addTab(URL);
  let browser = gBrowser.selectedBrowser;
  yield waitForLoad(browser);

  
  
  yield forceWriteState();

  
  let chk = browser.contentDocument.getElementById("chk");
  EventUtils.sendMouseEvent({type: "click"}, chk);
  yield waitForInput();

  
  let {entries: [{formdata}]} = JSON.parse(ss.getTabState(tab));
  is(formdata.id.chk, true, "chk's value is correct");

  
  yield forceWriteState();

  
  browser.contentDocument.getElementById("txt").focus();
  EventUtils.synthesizeKey("m", {});
  yield waitForInput();

  
  let {entries: [{formdata}]} = JSON.parse(ss.getTabState(tab));
  is(formdata.id.chk, true, "chk's value is correct");
  is(formdata.id.txt, "m", "txt's value is correct");

  
  let {entries: [{formdata}]} = JSON.parse(ss.getTabState(tab));
  is(formdata.id.chk, true, "chk's cached value is correct");
  is(formdata.id.txt, "m", "txt's cached value is correct");

  
  yield forceWriteState();

  
  let cdoc = browser.contentDocument.getElementById("ifr").contentDocument;
  EventUtils.sendMouseEvent({type: "click"}, cdoc.getElementById("chk"));
  yield waitForInput();

  
  cdoc.getElementById("txt").focus();
  EventUtils.synthesizeKey("m", {});
  yield waitForInput();

  
  let {entries: [{children: [{formdata}]}]} = JSON.parse(ss.getTabState(tab));
  is(formdata.id.chk, true, "iframe chk's value is correct");
  is(formdata.id.txt, "m", "iframe txt's value is correct");

  
  let {entries: [{children: [{formdata}]}]} = JSON.parse(ss.getTabState(tab));
  is(formdata.id.chk, true, "iframe chk's value is correct");
  is(formdata.id.txt, "m", "iframe txt's value is correct");

  
  yield forceWriteState();

  
  browser.contentDocument.getElementById("ced").focus();
  EventUtils.synthesizeKey("m", {});
  yield waitForInput();

  
  let {entries: [{children: [, {innerHTML}]}]} = JSON.parse(ss.getTabState(tab));
  is(innerHTML, "m", "content editable's value is correct");

  
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

function waitForLoad(aElement) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(next);
  }, true);
}

function waitForInput() {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.addMessageListener("SessionStore:input", function onPageShow() {
    mm.removeMessageListener("SessionStore:input", onPageShow);
    executeSoon(next);
  });
}
