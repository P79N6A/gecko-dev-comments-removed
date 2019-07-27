


"use strict";

const URL = ROOT + "browser_463205_sample.html";






add_task(function test_check_urls_before_restoring() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  ss.setTabState(tab, getState(URL));
  yield promiseTabRestored(tab);

  let value = yield getInputValue(browser, {id: "text"});
  is(value, "foobar", "value was restored");

  
  ss.setTabState(tab, getState("http://example.com/"));
  yield promiseTabRestored(tab);

  value = yield getInputValue(browser, {id: "text"});
  is(value, "", "value was not restored");

  
  gBrowser.removeTab(tab);
});

function getState(url) {
  return JSON.stringify({
    entries: [{url: URL}],
    formdata: {url: url, id: {text: "foobar"}}
  });
}
