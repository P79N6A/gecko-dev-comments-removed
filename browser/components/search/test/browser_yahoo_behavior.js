






"use strict";

const BROWSER_SEARCH_PREF = "browser.search.";


function test() {
  let engine = Services.search.getEngineByName("Yahoo");
  ok(engine, "Yahoo is installed");

  let previouslySelectedEngine = Services.search.currentEngine;
  Services.search.currentEngine = engine;
  engine.alias = "y";

  let base = "https://search.yahoo.com/yhs/search?p=foo&ei=UTF-8&hspart=mozilla";
  let url;

  
  url = engine.getSubmission("foo").uri.spec;
  is(url, base, "Check search URL for 'foo'");

  waitForExplicitFinish();

  var gCurrTest;
  var gTests = [
    {
      name: "context menu search",
      searchURL: base + "&hsimp=yhs-005",
      run: function () {
        
        
        BrowserSearch.loadSearch("foo", false, "contextmenu");
      }
    },
    {
      name: "keyword search",
      searchURL: base + "&hsimp=yhs-002",
      run: function () {
        gURLBar.value = "? foo";
        gURLBar.focus();
        EventUtils.synthesizeKey("VK_RETURN", {});
      }
    },
    {
      name: "keyword search with alias",
      searchURL: base + "&hsimp=yhs-002",
      run: function () {
        gURLBar.value = "y foo";
        gURLBar.focus();
        EventUtils.synthesizeKey("VK_RETURN", {});
      }
    },
    {
      name: "search bar search",
      searchURL: base + "&hsimp=yhs-001",
      run: function () {
        let sb = BrowserSearch.searchBar;
        sb.focus();
        sb.value = "foo";
        registerCleanupFunction(function () {
          sb.value = "";
        });
        EventUtils.synthesizeKey("VK_RETURN", {});
      }
    },
    {
      name: "new tab search",
      searchURL: base + "&hsimp=yhs-004",
      run: function () {
        function doSearch(doc) {
          
          gBrowser.addProgressListener(listener);
          doc.getElementById("newtab-search-text").value = "foo";
          doc.getElementById("newtab-search-submit").click();
        }

        
        
        gBrowser.removeProgressListener(listener);
        gBrowser.loadURI("about:newtab");
        info("Waiting for about:newtab load");
        tab.linkedBrowser.addEventListener("load", function load(event) {
          if (event.originalTarget != tab.linkedBrowser.contentDocument ||
              event.target.location.href == "about:blank") {
            info("skipping spurious load event");
            return;
          }
          tab.linkedBrowser.removeEventListener("load", load, true);

          
          let win = gBrowser.contentWindow;
          if (win.gSearch.currentEngineName ==
              Services.search.currentEngine.name) {
            doSearch(win.document);
          }
          else {
            info("Waiting for newtab search init");
            win.addEventListener("ContentSearchService", function done(event) {
              info("Got newtab search event " + event.detail.type);
              if (event.detail.type == "State") {
                win.removeEventListener("ContentSearchService", done);
                
                executeSoon(() => doSearch(win.document));
              }
            });
          }
        }, true);
      }
    }
  ];

  function nextTest() {
    if (gTests.length) {
      gCurrTest = gTests.shift();
      info("Running : " + gCurrTest.name);
      executeSoon(gCurrTest.run);
    } else {
      finish();
    }
  }

  let tab = gBrowser.selectedTab = gBrowser.addTab();

  let listener = {
    onStateChange: function onStateChange(webProgress, req, flags, status) {
      info("onStateChange");
      
      let docStart = Ci.nsIWebProgressListener.STATE_IS_DOCUMENT |
                     Ci.nsIWebProgressListener.STATE_START;
      if (!(flags & docStart) || !webProgress.isTopLevel)
        return;

      info("received document start");

      ok(req instanceof Ci.nsIChannel, "req is a channel");
      is(req.originalURI.spec, gCurrTest.searchURL, "search URL was loaded");
      info("Actual URI: " + req.URI.spec);

      req.cancel(Components.results.NS_ERROR_FAILURE);

      executeSoon(nextTest);
    }
  }

  registerCleanupFunction(function () {
    engine.alias = undefined;
    gBrowser.removeProgressListener(listener);
    gBrowser.removeTab(tab);
    Services.search.currentEngine = previouslySelectedEngine;
  });

  tab.linkedBrowser.addEventListener("load", function load() {
    tab.linkedBrowser.removeEventListener("load", load, true);
    gBrowser.addProgressListener(listener);
    nextTest();
  }, true);
}
