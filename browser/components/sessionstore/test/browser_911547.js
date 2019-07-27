






add_task(function* test() {
  
  let testURL = "http://mochi.test:8888/browser/browser/components/sessionstore/test/browser_911547_sample.html";
  let tab = gBrowser.selectedTab = gBrowser.addTab(testURL);
  gBrowser.selectedTab = tab;

  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  
  injectInlineScript(browser,'document.getElementById("test_id").value = "fail";');
  is(browser.contentDocument.getElementById("test_id").value, "ok",
     "CSP should block the inline script that modifies test_id");

  
  
  browser.contentDocument.getElementById("test_data_link").click();
  yield promiseBrowserLoaded(browser);

  is(browser.contentDocument.getElementById("test_id2").value, "ok",
     "CSP should block the script loaded by the clicked data URI");

  
  yield promiseRemoveTab(tab);

  
  tab = ss.undoCloseTab(window, 0);
  yield promiseTabRestored(tab);
  browser = tab.linkedBrowser;

  is(browser.contentDocument.getElementById("test_id2").value, "ok",
     "CSP should block the script loaded by the clicked data URI after restore");

  
  gBrowser.removeTab(tab);
});


function injectInlineScript(browser, scriptText) {
  let scriptElt = browser.contentDocument.createElement("script");
  scriptElt.type = 'text/javascript';
  scriptElt.text = scriptText;
  browser.contentDocument.body.appendChild(scriptElt);
}
