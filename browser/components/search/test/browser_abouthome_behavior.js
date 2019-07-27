






"use strict";

const BROWSER_SEARCH_PREF      = "browser.search.";

const MOZ_PARAM_LOCALE_REGEX   = /\{moz:locale\}/g;
const MOZ_PARAM_DIST_ID_REGEX  = /\{moz:distributionID\}/g;
const MOZ_PARAM_OFFICIAL_REGEX = /\{moz:official\}/g;


const MOZ_OFFICIAL = Services.appinfo.isOfficialBranding ? "official" : "unofficial";

XPCOMUtils.defineLazyGetter(this, "distributionID", () => {
  try {
    return Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "distributionID");
  } catch (ex) {
    return Services.appinfo.distributionID;
  }
});

XPCOMUtils.defineLazyGetter(this, "GOOGLE_CLIENT", () => {
  switch (Services.appinfo.defaultUpdateChannel) {
    case "beta":
      return "firefox-beta";
    case "aurora":
      return "firefox-aurora";
    case "nightly":
      return "firefox-nightly";
    default:
      return "firefox-a";
  }
});

function test() {
  
  ignoreAllUncaughtExceptions(true);

  function replaceUrl(base) {
    return base.replace(MOZ_PARAM_LOCALE_REGEX, getLocale())
               .replace(MOZ_PARAM_DIST_ID_REGEX, distributionID)
               .replace(MOZ_PARAM_OFFICIAL_REGEX, MOZ_OFFICIAL);
  }

  let gMutationObserver = null;

  function verify_about_home_search(engine_name) {
    let engine = Services.search.getEngineByName(engine_name);
    ok(engine, engine_name + " is installed");

    let previouslySelectedEngine = Services.search.currentEngine;
    Services.search.currentEngine = engine;

    
    
    gBrowser.removeProgressListener(listener);
    gBrowser.loadURI("about:home");
    info("Waiting for about:home load");
    tab.linkedBrowser.addEventListener("load", function load(event) {
      if (event.originalTarget != tab.linkedBrowser.contentDocument ||
          event.target.location.href == "about:blank") {
        info("skipping spurious load event");
        return;
      }
      tab.linkedBrowser.removeEventListener("load", load, true);

      
      let doc = gBrowser.contentDocument;
      gMutationObserver = new MutationObserver(function (mutations) {
        for (let mutation of mutations) {
          if (mutation.attributeName == "searchEngineName") {
            
            gBrowser.addProgressListener(listener);
            gMutationObserver.disconnect()
            gMutationObserver = null;
            executeSoon(function() {
              doc.getElementById("searchText").value = "foo";
              doc.getElementById("searchSubmit").click();
            });
          }
        }
      });
      gMutationObserver.observe(doc.documentElement, { attributes: true });
    }, true);
  }
  waitForExplicitFinish();

  let gCurrTest;
  let gTests = [
    {
      name: "Search with Bing from about:home",
      searchURL: replaceUrl("http://www.bing.com/search?q=foo&pc=MOZI&form=MOZSPG"),
      run: function () {
        verify_about_home_search("Bing");
      }
    },
    {
      name: "Search with Yahoo from about:home",
      searchURL: replaceUrl("https://search.yahoo.com/search?p=foo&ei=UTF-8&fr=moz35"),
      run: function () {
        verify_about_home_search("Yahoo");
      }
    },
    {
      name: "Search with eBay from about:home",
      searchURL: replaceUrl("http://rover.ebay.com/rover/1/711-47294-18009-3/4?mfe=search&mpre=http://www.ebay.com/sch/i.html?_nkw=foo"),
      run: function () {
        verify_about_home_search("eBay");
      }
    },
    {
      name: "Search with Google from about:home",
      searchURL: replaceUrl("https://www.google.com/search?q=foo&ie=utf-8&oe=utf-8&aq=t&rls={moz:distributionID}:{moz:locale}:{moz:official}&client=" + GOOGLE_CLIENT + "&channel=np&source=hp"),
      run: function () {
        verify_about_home_search("Google");
      }
    },
    {
      name: "Search with Amazon.com from about:home",
      searchURL: replaceUrl("http://www.amazon.com/exec/obidos/external-search/?field-keywords=foo&mode=blended&tag=mozilla-20&sourceid=Mozilla-search"),
      run: function () {
        verify_about_home_search("Amazon.com");
      }
    }
  ];

  function nextTest() {
    if (gTests.length) {
      gCurrTest = gTests.shift();
      info("Running : " + gCurrTest.name);
      executeSoon(gCurrTest.run);
    } else {
      
      executeSoon(finish);
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
    gBrowser.removeProgressListener(listener);
    gBrowser.removeTab(tab);
    if (gMutationObserver)
      gMutationObserver.disconnect();
  });

  tab.linkedBrowser.addEventListener("load", function load() {
    tab.linkedBrowser.removeEventListener("load", load, true);
    gBrowser.addProgressListener(listener);
    nextTest();
  }, true);
}
