






function test() {
  TestRunner.run();
}

function runTests() {
  
  let testURL = "http://mochi.test:8888/browser/browser/components/sessionstore/test/browser_911547_sample.html";
  let tab = gBrowser.selectedTab = gBrowser.addTab(testURL);
  gBrowser.selectedTab = tab;

  let browser = tab.linkedBrowser;
  yield waitForLoad(browser);

  
  
  injectInlineScript(browser,'document.getElementById("test_id").value = "fail";');
  is(browser.contentDocument.getElementById("test_id").value, "ok",
     "CSP should block the inline script that modifies test_id");

  
  
  browser.contentDocument.getElementById("test_data_link").click();
  yield waitForLoad(browser);

  is(browser.contentDocument.getElementById("test_id2").value, "ok",
     "CSP should block the script loaded by the clicked data URI");

  
  gBrowser.removeTab(tab);

  
  tab = ss.undoCloseTab(window, 0);
  yield waitForTabRestored(tab);
  browser = tab.linkedBrowser;

  is(browser.contentDocument.getElementById("test_id2").value, "ok",
     "CSP should block the script loaded by the clicked data URI after restore");

  
  gBrowser.removeTab(tab);
}

function waitForLoad(aElement) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(next);
  }, true);
}

function waitForTabRestored(aElement) {
  aElement.addEventListener("SSTabRestored", function tabRestored(e) {
    aElement.removeEventListener("SSTabRestored", tabRestored, true);
    executeSoon(next);
  }, true);
}


function injectInlineScript(browser, scriptText) {
  let scriptElt = browser.contentDocument.createElement("script");
  scriptElt.type = 'text/javascript';
  scriptElt.text = scriptText;
  browser.contentDocument.body.appendChild(scriptElt);
}
