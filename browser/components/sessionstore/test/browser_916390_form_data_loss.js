


const URL = "http://mochi.test:8888/browser/" +
            "browser/components/sessionstore/test/browser_916390_sample.html";

function test() {
  TestRunner.run();
}

function runTests() {
  
  let tab = gBrowser.selectedTab = gBrowser.addTab(URL);
  let browser = gBrowser.selectedBrowser;
  yield waitForLoad(browser);

  
  browser.contentDocument.getElementById("txt").focus();
  EventUtils.synthesizeKey("m", {});
  yield waitForInput();

  
  let state = JSON.parse(ss.getBrowserState());
  let {formdata} = state.windows[0].tabs[1].entries[0];
  is(formdata.id.txt, "m", "txt's value is correct");

  
  
  browser.loadURI(URL + "#");
  browser.contentWindow.sessionStorage.foo = "bar";
  yield waitForStorageChange();

  
  let state = JSON.parse(ss.getBrowserState());
  let {formdata} = state.windows[0].tabs[1].entries[1];
  is(formdata.id.txt, "m", "txt's value is correct");

  
  gBrowser.removeTab(tab);
}

function waitForLoad(aElement) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(next);
  }, true);
}

function waitForInput() {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.addMessageListener("SessionStore:input", function onInput() {
    mm.removeMessageListener("SessionStore:input", onInput);
    executeSoon(next);
  });
}

function waitForStorageChange() {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.addMessageListener("SessionStore:MozStorageChanged", function onChanged() {
    mm.removeMessageListener("SessionStore:MozStorageChanged", onChanged);
    executeSoon(next);
  });
}
