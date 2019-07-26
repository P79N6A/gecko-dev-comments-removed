






function runTests() {
  
  let tab = gBrowser.addTab("http://127.0.0.1:1/");
  let browser = tab.linkedBrowser;

  yield browser.addEventListener("DOMContentLoaded", function onLoad() {
    browser.removeEventListener("DOMContentLoaded", onLoad, false);
    executeSoon(next);
  }, false);

  ok(!gBrowserThumbnails._shouldCapture(browser), "we're not going to capture an error page");
}
