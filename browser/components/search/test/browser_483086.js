


































let gSS = Cc["@mozilla.org/browser/search-service;1"].
           getService(Ci.nsIBrowserSearchService);
let gObs = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

function test() {
  waitForExplicitFinish();

  let observer = {
    observe: function(aSubject, aTopic, aData) {
      switch (aData) {
        case "engine-added":
          let engine = gSS.getEngineByName("483086a");
          ok(engine, "Test engine 1 installed");
          isnot(engine.searchForm, "foo://example.com",
                "Invalid SearchForm URL dropped");
          gSS.removeEngine(engine);
          break;
        case "engine-removed":
          gObs.removeObserver(this, "browser-search-engine-modified");
          test2();
          break;
      }
    }
  };

  gObs.addObserver(observer, "browser-search-engine-modified", false);
  gSS.addEngine("http://localhost:8888/browser/browser/components/search/test/483086-1.xml",
                Ci.nsISearchEngine.DATA_XML, "data:image/x-icon;%00",
                false);
}

function test2() {
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      switch (aData) {
        case "engine-added":
          let engine = gSS.getEngineByName("483086b");
          ok(engine, "Test engine 2 installed");
          is(engine.searchForm, "http://example.com", "SearchForm is correct");
          gSS.removeEngine(engine);
          break;
        case "engine-removed":  
          gObs.removeObserver(this, "browser-search-engine-modified");
          finish();
          break;
      }
    }
  };

  gObs.addObserver(observer, "browser-search-engine-modified", false);
  gSS.addEngine("http://localhost:8888/browser/browser/components/search/test/483086-2.xml",
                Ci.nsISearchEngine.DATA_XML, "data:image/x-icon;%00",
                false);
}
