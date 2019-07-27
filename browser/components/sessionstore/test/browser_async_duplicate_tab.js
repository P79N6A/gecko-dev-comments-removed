"use strict";

const URL = "data:text/html;charset=utf-8,<a href=%23>clickme</a>";

add_task(function* test_duplicate() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("hashchange", function onHashChange() {
        removeEventListener("hashchange", onHashChange);
        resolve();
      });

      
      content.document.querySelector("a").click();
    });
  });

  
  let tab2 = ss.duplicateTab(window, tab);

  
  yield promiseTabRestored(tab2);
  yield TabStateFlusher.flush(tab2.linkedBrowser);

  
  let {entries} = JSON.parse(ss.getTabState(tab2));
  is(entries.length, 2, "there are two shistory entries");

  
  yield promiseRemoveTab(tab2);
  yield promiseRemoveTab(tab);
});

add_task(function* test_duplicate_remove() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("hashchange", function onHashChange() {
        removeEventListener("hashchange", onHashChange);
        resolve();
      });

      
      content.document.querySelector("a").click();
    });
  });

  
  let tab2 = ss.duplicateTab(window, tab);

  
  yield Promise.all([promiseRemoveTab(tab), promiseTabRestored(tab2)]);
  yield TabStateFlusher.flush(tab2.linkedBrowser);

  
  let {entries} = JSON.parse(ss.getTabState(tab2));
  is(entries.length, 2, "there are two shistory entries");

  
  yield promiseRemoveTab(tab2);
});
