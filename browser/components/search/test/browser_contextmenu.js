





function test() {
  waitForExplicitFinish();

  const ss = Services.search;
  const ENGINE_NAME = "Foo";
  var contextMenu;

  function observer(aSub, aTopic, aData) {
    switch (aData) {
      case "engine-added":
        var engine = ss.getEngineByName(ENGINE_NAME);
        ok(engine, "Engine was added.");
        
        
        break;
      case "engine-current":
        is(ss.currentEngine.name, ENGINE_NAME, "currentEngine set");
        startTest();
        break;
      case "engine-removed":
        Services.obs.removeObserver(observer, "browser-search-engine-modified");
        finish();
        break;
    }
  }

  Services.obs.addObserver(observer, "browser-search-engine-modified", false);
  ss.addEngine("http://mochi.test:8888/browser/browser/components/search/test/testEngine_mozsearch.xml",
               Ci.nsISearchEngine.DATA_XML, "data:image/x-icon,%00",
               false);

  function startTest() {
    contextMenu = document.getElementById("contentAreaContextMenu");
    ok(contextMenu, "Got context menu XUL");

    doOnloadOnce(testContextMenu);
    var tab = gBrowser.addTab("data:text/plain;charset=utf8,test%20search");
    gBrowser.selectedTab = tab;
  }

  function testContextMenu() {
    function rightClickOnDocument() {
      waitForBrowserContextMenu(checkContextMenu);
      var clickTarget = content.document.body;
      var eventDetails = { type: "contextmenu", button: 2 };
      EventUtils.synthesizeMouseAtCenter(clickTarget, eventDetails, content);
    }

    
    function checkContextMenu() {
      var searchItem = contextMenu.getElementsByAttribute("id", "context-searchselect")[0];
      ok(searchItem, "Got search context menu item");
      is(searchItem.label, 'Search ' + ENGINE_NAME + ' for "test search"', "Check context menu label");
      is(searchItem.disabled, false, "Check that search context menu item is enabled");
      searchItem.click();
      contextMenu.hidePopup();
    }

    function checkSearchURL(event){
      is(event.originalTarget.URL,
         "http://mochi.test:8888/browser/browser/components/search/test/?test=test+search&ie=utf-8&client=app&channel=contextsearch",
         "Checking context menu search URL");
      finalize();
    }

    doOnloadOnce(checkSearchURL);

    
    var selectAllItem = contextMenu.getElementsByAttribute("id", "context-selectall")[0];
    ok(selectAllItem, "Got select all context menu item");
    selectAllItem.click();

    
    SimpleTest.executeSoon(rightClickOnDocument);
  }

  function finalize() {
    while (gBrowser.tabs.length != 1) {
      gBrowser.removeTab(gBrowser.tabs[0]);
    }
    content.location.href = "about:blank";
    var engine = ss.getEngineByName(ENGINE_NAME);
    ss.removeEngine(engine);
  }

  function doOnloadOnce(callback) {
    gBrowser.addEventListener("DOMContentLoaded", function handleLoad(event) {
      gBrowser.removeEventListener("DOMContentLoaded", handleLoad, true);
      callback(event);
    }, true);
  }
}
