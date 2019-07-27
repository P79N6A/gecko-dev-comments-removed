


"use strict";

const URL = ROOT + "browser_463205_sample.html";






add_task(function test_check_urls_before_restoring() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield promiseTabState(tab, getState(URL));

  let value = yield getInputValue(browser, {id: "text"});
  is(value, "foobar", "value was restored");

  
  yield promiseTabState(tab, getState("http://example.com/"));

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
