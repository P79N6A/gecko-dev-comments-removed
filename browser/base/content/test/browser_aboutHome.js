



XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AboutHomeUtils",
  "resource:///modules/AboutHomeUtils.jsm");

let gRightsVersion = Services.prefs.getIntPref("browser.rights.version");

registerCleanupFunction(function() {
  
  Services.prefs.clearUserPref("network.cookies.cookieBehavior");
  Services.prefs.clearUserPref("network.cookie.lifetimePolicy");
  Services.prefs.clearUserPref("browser.rights.override");
  Services.prefs.clearUserPref("browser.rights." + gRightsVersion + ".shown");
});

let gTests = [

{
  desc: "Check that clearing cookies does not clear storage",
  setup: function ()
  {
    Cc["@mozilla.org/observer-service;1"]
      .getService(Ci.nsIObserverService)
      .notifyObservers(null, "cookie-changed", "cleared");
  },
  run: function (aSnippetsMap)
  {
    isnot(aSnippetsMap.get("snippets-last-update"), null,
          "snippets-last-update should have a value");
  }
},

{
  desc: "Check default snippets are shown",
  setup: function () { },
  run: function ()
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;
    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element")
    is(snippetsElt.getElementsByTagName("span").length, 1,
       "A default snippet is present.");
  }
},

{
  desc: "Check default snippets are shown if snippets are invalid xml",
  setup: function (aSnippetsMap)
  {
    
    aSnippetsMap.set("snippets", "<p><b></p></b>");
  },
  run: function (aSnippetsMap)
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    is(snippetsElt.getElementsByTagName("span").length, 1,
       "A default snippet is present.");

    aSnippetsMap.delete("snippets");
  }
},

{
  desc: "Check that search engine logo has alt text",
  setup: function () { },
  run: function ()
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let searchEngineLogoElt = doc.getElementById("searchEngineLogo");
    ok(searchEngineLogoElt, "Found search engine logo");

    let altText = searchEngineLogoElt.alt;
    ok(typeof altText == "string" && altText.length > 0,
       "Search engine logo's alt text is a nonempty string");

    isnot(altText, "undefined",
          "Search engine logo's alt text shouldn't be the string 'undefined'");
  }
},

{
  desc: "Check that performing a search fires a search event and records to " +
        "Firefox Health Report.",
  setup: function () { },
  run: function () {
    try {
      let cm = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
      cm.getCategoryEntry("healthreport-js-provider-default", "SearchesProvider");
    } catch (ex) {
      
      return Promise.resolve();
    }

    let numSearchesBefore = 0;
    let deferred = Promise.defer();
    let doc = gBrowser.contentDocument;

    
    
    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      let engineName = doc.documentElement.getAttribute("searchEngineName");
      is(e.detail, engineName, "Detail is search engine name");

      gBrowser.stop();

      getNumberOfSearches().then(num => {
        is(num, numSearchesBefore + 1, "One more search recorded.");
        deferred.resolve();
      });
    }, true, true);

    
    getNumberOfSearches().then(num => {
      numSearchesBefore = num;

      info("Perform a search.");
      doc.getElementById("searchText").value = "a search";
      doc.getElementById("searchSubmit").click();
    });

    return deferred.promise;
  }
},

{
  desc: "Check snippets map is cleared if cached version is old",
  setup: function (aSnippetsMap)
  {
    aSnippetsMap.set("snippets", "test");
    aSnippetsMap.set("snippets-cached-version", 0);
  },
  run: function (aSnippetsMap)
  {
    ok(!aSnippetsMap.has("snippets"), "snippets have been properly cleared");
    ok(!aSnippetsMap.has("snippets-cached-version"),
       "cached-version has been properly cleared");
  }
},

{
  desc: "Check cached snippets are shown if cached version is current",
  setup: function (aSnippetsMap)
  {
    aSnippetsMap.set("snippets", "test");
  },
  run: function (aSnippetsMap)
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    is(snippetsElt.innerHTML, "test", "Cached snippet is present.");

    is(aSnippetsMap.get("snippets"), "test", "snippets still cached");
    is(aSnippetsMap.get("snippets-cached-version"),
       AboutHomeUtils.snippetsVersion,
       "cached-version is correct");
    ok(aSnippetsMap.has("snippets-last-update"), "last-update still exists");
  }
},

{
  desc: "Check if the 'Know Your Rights default snippet is shown when 'browser.rights.override' pref is set",
  beforeRun: function ()
  {
    Services.prefs.setBoolPref("browser.rights.override", false);
  },
  setup: function () { },
  run: function (aSnippetsMap)
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;
    let showRights = AboutHomeUtils.showKnowYourRights;

    ok(showRights, "AboutHomeUtils.showKnowYourRights should be TRUE");

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    is(snippetsElt.getElementsByTagName("a")[0].href, "about:rights", "Snippet link is present.");

    Services.prefs.clearUserPref("browser.rights.override");
  }
},

{
  desc: "Check if the 'Know Your Rights default snippet is NOT shown when 'browser.rights.override' pref is NOT set",
  beforeRun: function ()
  {
    Services.prefs.setBoolPref("browser.rights.override", true);
  },
  setup: function () { },
  run: function (aSnippetsMap)
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;
    let rightsData = AboutHomeUtils.knowYourRightsData;
    
    ok(!rightsData, "AboutHomeUtils.knowYourRightsData should be FALSE");

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    ok(snippetsElt.getElementsByTagName("a")[0].href != "about:rights", "Snippet link should not point to about:rights.");

    Services.prefs.clearUserPref("browser.rights.override");
  }
},

{
  desc: "Check that the search UI/ action is updated when the search engine is changed",
  setup: function() {},
  run: function()
  {
    let currEngine = Services.search.currentEngine;
    let unusedEngines = [].concat(Services.search.getVisibleEngines()).filter(x => x != currEngine);
    let searchbar = document.getElementById("searchbar");

    function checkSearchUI(engine) {
      let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;
      let searchText = doc.getElementById("searchText");
      let logoElt = doc.getElementById("searchEngineLogo");
      let engineName = doc.documentElement.getAttribute("searchEngineName");

      is(engineName, engine.name, "Engine name should've been updated");

      if (!logoElt.parentNode.hidden) {
        is(logoElt.alt, engineName, "Alt text of logo image should match search engine name")
      } else {
        is(searchText.placeholder, engineName, "Placeholder text should match search engine name");
      }
    }
    
    checkSearchUI(currEngine);

    let deferred = Promise.defer();
    promiseBrowserAttributes(gBrowser.selectedTab).then(function() {
      
      checkSearchUI(unusedEngines[0]);
      deferred.resolve();
    });

    
    registerCleanupFunction(function() {
      searchbar.currentEngine = currEngine;
    });
    
    searchbar.currentEngine = unusedEngines[0];
    searchbar.select();
    return deferred.promise;
  }
}

];

function test()
{
  waitForExplicitFinish();
  requestLongerTimeout(2);

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);

      if (test.beforeRun)
        yield test.beforeRun();

      let tab = yield promiseNewTabLoadEvent("about:home", "DOMContentLoaded");

      
      
      
      
      let promise = promiseBrowserAttributes(tab);
      
      let snippetsMap = yield promiseSetupSnippetsMap(tab, test.setup);
      
      yield promise;
      info("Running test");
      yield test.run(snippetsMap);
      info("Cleanup");
      gBrowser.removeCurrentTab();
    }
  }).then(finish, ex => {
    ok(false, "Unexpected Exception: " + ex);
    finish();
  });
}










function promiseNewTabLoadEvent(aUrl, aEventType="load")
{
  let deferred = Promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(aUrl);
  info("Wait tab event: " + aEventType);
  tab.linkedBrowser.addEventListener(aEventType, function load(event) {
    if (event.originalTarget != tab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank") {
      info("skipping spurious load event");
      return;
    }
    tab.linkedBrowser.removeEventListener(aEventType, load, true);
    info("Tab event received: " + aEventType);
    deferred.resolve(tab);
  }, true);
  return deferred.promise;
}











function promiseSetupSnippetsMap(aTab, aSetupFn)
{
  let deferred = Promise.defer();
  let cw = aTab.linkedBrowser.contentWindow.wrappedJSObject;
  info("Waiting for snippets map");
  cw.ensureSnippetsMapThen(function (aSnippetsMap) {
    info("Got snippets map: " +
         "{ last-update: " + aSnippetsMap.get("snippets-last-update") +
         ", cached-version: " + aSnippetsMap.get("snippets-cached-version") +
         " }");
    
    aSnippetsMap.set("snippets-last-update", Date.now());
    aSnippetsMap.set("snippets-cached-version", AboutHomeUtils.snippetsVersion);
    
    aSnippetsMap.delete("snippets");
    aSetupFn(aSnippetsMap);
    
    executeSoon(function() deferred.resolve(aSnippetsMap));
  });
  return deferred.promise;
}









function promiseBrowserAttributes(aTab)
{
  let deferred = Promise.defer();

  let docElt = aTab.linkedBrowser.contentDocument.documentElement;
  
  let observer = new MutationObserver(function (mutations) {
    for (let mutation of mutations) {
      info("Got attribute mutation: " + mutation.attributeName +
                                    " from " + mutation.oldValue); 
      if (mutation.attributeName == "snippetsURL" &&
          docElt.getAttribute("snippetsURL") != "nonexistent://test") {
        docElt.setAttribute("snippetsURL", "nonexistent://test");
      }

      
      if (mutation.attributeName == "searchEngineURL") {
        info("Remove attributes observer");
        observer.disconnect();
        
        executeSoon(function() deferred.resolve());
        break;
      }
    }
  });
  info("Add attributes observer");
  observer.observe(docElt, { attributes: true });

  return deferred.promise;
}






function getNumberOfSearches() {
  let reporter = Components.classes["@mozilla.org/datareporting/service;1"]
                                   .getService()
                                   .wrappedJSObject
                                   .healthReporter;
  ok(reporter, "Health Reporter instance available.");

  return reporter.onInit().then(function onInit() {
    let provider = reporter.getProvider("org.mozilla.searches");
    ok(provider, "Searches provider is available.");

    let m = provider.getMeasurement("counts", 2);
    return m.getValues().then(data => {
      let now = new Date();
      let yday = new Date(now);
      yday.setDate(yday.getDate() - 1);

      
      
      
      
      
      
      return getNumberOfSearchesByDate(data, now) +
             getNumberOfSearchesByDate(data, yday);
    });
  });
}

function getNumberOfSearchesByDate(aData, aDate) {
  if (aData.days.hasDay(aDate)) {
    let doc = gBrowser.contentDocument;
    let engineName = doc.documentElement.getAttribute("searchEngineName");
    let id = Services.search.getEngineByName(engineName).identifier;

    let day = aData.days.getDay(aDate);
    let field = id + ".abouthome";

    if (day.has(field)) {
      return day.get(field) || 0;
    }
  }

  return 0; 
}
