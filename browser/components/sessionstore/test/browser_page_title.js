"use strict";

const URL = "data:text/html,<title>initial title</title>";

add_task(function* () {
  
  let tab = gBrowser.addTab(URL);
  yield promiseBrowserLoaded(tab.linkedBrowser);

  
  yield promiseRemoveTab(tab);

  
  let [{state: {entries}}] = JSON.parse(ss.getClosedTabData(window));
  is(entries[0].title, "initial title", "correct title");
});

add_task(function* () {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("DOMTitleChanged", function onTitleChanged() {
        removeEventListener("DOMTitleChanged", onTitleChanged);
        resolve();
      });

      content.document.title = "new title";
    });
  });

  
  yield promiseRemoveTab(tab);

  
  let [{state: {entries}}] = JSON.parse(ss.getClosedTabData(window));
  is(entries[0].title, "new title", "correct title");
});
