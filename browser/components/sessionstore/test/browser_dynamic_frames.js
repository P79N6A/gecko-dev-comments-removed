


"use strict";





add_task(function () {
  
  
  
  
  const URL = "data:text/html;charset=utf-8," +
              "<frameset cols=50%25,50%25><frame src=about%3Amozilla>" +
              "<frame src=about%3Arobots></frameset>" +
              "<script>var i=document.createElement('iframe');" +
              "i.setAttribute('src', 'about%3Arights');" +
              "document.body.appendChild(i);</script>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));

  
  ok(entries[0].url.startsWith("data:text/html"), "correct root url");
  is(entries[0].children[0].url, "about:mozilla", "correct url for 1st frame");
  is(entries[0].children[1].url, "about:robots", "correct url for 2nd frame");

  
  is(entries.length, 1, "there is one root entry ...");
  is(entries[0].children.length, 2, "... with two child entries");

  
  gBrowser.removeTab(tab);
});






add_task(function () {
  
  
  
  const URL = "data:text/html;charset=utf-8," +
              "<iframe name=t src=about%3Amozilla></iframe>" +
              "<a id=lnk href=about%3Arobots target=t>clickme</a>" +
              "<script>var i=document.createElement('iframe');" +
              "i.setAttribute('src', 'about%3Arights');" +
              "document.body.appendChild(i);</script>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  SyncHandlers.get(browser).flush();
  let {entries} = JSON.parse(ss.getTabState(tab));

  
  ok(entries[0].url.startsWith("data:text/html"), "correct root url");
  is(entries[0].children[0].url, "about:mozilla", "correct url for static frame");

  
  is(entries.length, 1, "there is one root entry ...");
  is(entries[0].children.length, 1, "... with a single child entry");

  
  browser.messageManager.sendAsyncMessage("ss-test:click", {id: "lnk"});
  yield promiseBrowserLoaded(browser, false );

  SyncHandlers.get(browser).flush();
  ({entries} = JSON.parse(ss.getTabState(tab)));

  
  ok(entries[0].url.startsWith("data:text/html"), "correct 1st root url");
  ok(entries[1].url.startsWith("data:text/html"), "correct 2nd root url");
  is(entries[0].children[0].url, "about:mozilla", "correct url for 1st static frame");
  is(entries[1].children[0].url, "about:robots", "correct url for 2ns static frame");

  
  is(entries.length, 2, "there are two root entries ...");
  is(entries[0].children.length, 1, "... with a single child entry ...");
  is(entries[1].children.length, 1, "... each");

  
  gBrowser.removeTab(tab);
});
