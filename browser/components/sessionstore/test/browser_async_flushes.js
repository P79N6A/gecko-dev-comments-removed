"use strict";

const URL = "data:text/html;charset=utf-8,<a href=%23>clickme</a>";

add_task(function* test_flush() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is a single history entry");

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("hashchange", function onHashChange() {
        removeEventListener("hashchange", onHashChange);
        resolve();
      });

      
      content.document.querySelector("a").click();
    });
  });

  
  yield TabStateFlusher.flush(browser);

  
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there are two shistory entries");

  
  gBrowser.removeTab(tab);
});

add_task(function* test_crash() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is a single history entry");

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("hashchange", function onHashChange() {
        removeEventListener("hashchange", onHashChange);
        resolve();
      });

      
      content.document.querySelector("a").click();
    });
  });

  
  
  
  
  let promise1 = crashBrowser(browser);
  let promise2 = TabStateFlusher.flush(browser);
  yield Promise.all([promise1, promise2]);

  
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 1, "still only one history entry");

  
  gBrowser.removeTab(tab);
});

add_task(function* test_remove() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is a single history entry");

  
  yield ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      addEventListener("hashchange", function onHashChange() {
        removeEventListener("hashchange", onHashChange);
        resolve();
      });

      
      content.document.querySelector("a").click();
    });
  });

  
  yield Promise.all([TabStateFlusher.flush(browser), promiseRemoveTab(tab)]);
})
