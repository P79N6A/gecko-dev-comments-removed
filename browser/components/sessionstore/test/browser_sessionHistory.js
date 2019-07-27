


"use strict";




add_task(function test_load_start() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  browser.loadURI("about:mozilla");
  yield promiseContentMessage(browser, "ss-test:OnHistoryReplaceEntry");
  gBrowser.removeTab(tab);

  
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  is(browser.currentURI.spec, "about:mozilla", "url is correct");

  
  gBrowser.removeTab(tab);
});




add_task(function test_purge() {
  
  let tab = gBrowser.addTab("about:mozilla");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  browser.loadURI("about:robots");
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 2, "there are two shistory entries");

  
  yield sendMessage(browser, "ss-test:purgeSessionHistory");

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 1, "there is one shistory entry");

  
  gBrowser.removeTab(tab);
});




add_task(function test_hashchange() {
  const URL = "data:text/html;charset=utf-8,<a id=a href=%23>clickme</a>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is one shistory entry");

  
  browser.messageManager.sendAsyncMessage("ss-test:click", {id: "a"});
  yield promiseContentMessage(browser, "ss-test:hashchange");

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there are two shistory entries");

  
  gBrowser.removeTab(tab);
});




add_task(function test_pageshow() {
  const URL = "data:text/html;charset=utf-8,<h1>first</h1>";
  const URL2 = "data:text/html;charset=utf-8,<h1>second</h1>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  browser.loadURI(URL2);
  yield promiseBrowserLoaded(browser);

  
  browser.goBack();
  yield promiseContentMessage(browser, "ss-test:onFrameTreeCollected");
  is(browser.currentURI.spec, URL, "correct url after going back");

  
  SyncHandlers.get(browser).flush();
  let {index} = JSON.parse(ss.getTabState(tab));
  is(index, 1, "first history entry is selected");

  
  gBrowser.removeTab(tab);
});




add_task(function test_subframes() {
  const URL = "data:text/html;charset=utf-8," +
              "<iframe src=http%3A//example.com/ name=t></iframe>" +
              "<a id=a1 href=http%3A//example.com/1 target=t>clickme</a>" +
              "<a id=a2 href=http%3A//example.com/%23 target=t>clickme</a>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is one shistory entry");
  is(entries[0].children.length, 1, "the entry has one child");

  
  browser.messageManager.sendAsyncMessage("ss-test:click", {id: "a1"});
  yield promiseBrowserLoaded(browser, false );

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there now are two shistory entries");
  is(entries[1].children.length, 1, "the second entry has one child");

  
  browser.goBack();
  yield promiseBrowserLoaded(browser, false );

  
  browser.messageManager.sendAsyncMessage("ss-test:click", {id: "a2"});
  yield promiseContentMessage(browser, "ss-test:hashchange");

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there now are two shistory entries");
  is(entries[1].children.length, 1, "the second entry has one child");

  
  gBrowser.removeTab(tab);
});




add_task(function test_about_page_navigate() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is one shistory entry");
  is(entries[0].url, "about:blank", "url is correct");

  browser.loadURI("about:robots");
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 1, "there is one shistory entry");
  is(entries[0].url, "about:robots", "url is correct");

  
  gBrowser.removeTab(tab);
});




add_task(function test_pushstate_replacestate() {
  
  let tab = gBrowser.addTab("http://example.com/1");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));
  is(entries.length, 1, "there is one shistory entry");
  is(entries[0].url, "http://example.com/1", "url is correct");

  browser.messageManager.
    sendAsyncMessage("ss-test:historyPushState", {url: 'test-entry/'});
  yield promiseContentMessage(browser, "ss-test:historyPushState");

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there is another shistory entry");
  is(entries[1].url, "http://example.com/test-entry/", "url is correct");

  browser.messageManager.
    sendAsyncMessage("ss-test:historyReplaceState", {url: 'test-entry2/'});
  yield promiseContentMessage(browser, "ss-test:historyReplaceState");

  
  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));
  is(entries.length, 2, "there is still two shistory entries");
  is(entries[1].url, "http://example.com/test-entry/test-entry2/", "url is correct");

  
  gBrowser.removeTab(tab);
});




add_task(function test_slow_subframe_load() {
  const SLOW_URL = "http://mochi.test:8888/browser/browser/components/" +
                   "sessionstore/test/browser_sessionHistory_slow.sjs";

  const URL = "data:text/html;charset=utf-8," +
              "<frameset cols=50%25,50%25>" +
              "<frame src='" + SLOW_URL + "'>" +
              "</frameset>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));

  
  is(entries.length, 1, "there is one root entry ...");
  is(entries[0].children.length, 1, "... with one child entries");

  
  ok(entries[0].url.startsWith("data:text/html"), "correct root url");
  is(entries[0].children[0].url, SLOW_URL, "correct url for subframe");

  
  gBrowser.removeTab(tab);
});
