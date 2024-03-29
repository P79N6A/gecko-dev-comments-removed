








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: Assert is null");


XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AboutHomeUtils",
  "resource:///modules/AboutHome.jsm");

const TEST_CONTENT_HELPER = "chrome://mochitests/content/browser/browser/base/content/test/general/aboutHome_content_script.js";
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
    let doc = gBrowser.selectedBrowser.contentDocument;
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
    let doc = gBrowser.selectedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    is(snippetsElt.getElementsByTagName("span").length, 1,
       "A default snippet is present.");

    aSnippetsMap.delete("snippets");
  }
},


{
  desc: "Check that performing a search fires a search event and records to " +
        "Firefox Health Report.",
  setup: function () { },
  run: function* () {
    
    if (navigator.platform.indexOf("Linux") == 0) {
      return Promise.resolve();
    }

    try {
      let cm = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
      cm.getCategoryEntry("healthreport-js-provider-default", "SearchesProvider");
    } catch (ex) {
      
      return Promise.resolve();
    }

    let engine = yield promiseNewEngine("searchSuggestionEngine.xml");
    
    engine.wrappedJSObject._identifier = 'org.mozilla.testsearchsuggestions';

    let p = promiseContentSearchChange(engine.name);
    Services.search.currentEngine = engine;
    yield p;

    let numSearchesBefore = 0;
    let searchEventDeferred = Promise.defer();
    let doc = gBrowser.contentDocument;
    let engineName = gBrowser.contentWindow.wrappedJSObject.gContentSearchController.defaultEngine.name;
    is(engine.name, engineName, "Engine name in DOM should match engine we just added");

    
    let searchStr = "a search";
    getNumberOfSearches(engineName).then(num => {
      numSearchesBefore = num;

      info("Perform a search.");
      doc.getElementById("searchText").value = searchStr;
      doc.getElementById("searchSubmit").click();
    });

    let expectedURL = Services.search.currentEngine.
                      getSubmission(searchStr, null, "homepage").
                      uri.spec;
    let loadPromise = waitForDocLoadAndStopIt(expectedURL).then(() => {
      getNumberOfSearches(engineName).then(num => {
        is(num, numSearchesBefore + 1, "One more search recorded.");
        searchEventDeferred.resolve();
      });
    });

    try {
      yield Promise.all([searchEventDeferred.promise, loadPromise]);
    } catch (ex) {
      Cu.reportError(ex);
      ok(false, "An error occurred waiting for the search to be performed: " + ex);
    } finally {
      try {
        Services.search.removeEngine(engine);
      } catch (ex) {}
    }
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
    let doc = gBrowser.selectedBrowser.contentDocument;

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
    let doc = gBrowser.selectedBrowser.contentDocument;
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
    let doc = gBrowser.selectedBrowser.contentDocument;
    let rightsData = AboutHomeUtils.knowYourRightsData;

    ok(!rightsData, "AboutHomeUtils.knowYourRightsData should be FALSE");

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    ok(snippetsElt.getElementsByTagName("a")[0].href != "about:rights", "Snippet link should not point to about:rights.");

    Services.prefs.clearUserPref("browser.rights.override");
  }
},

{
  desc: "Check POST search engine support",
  setup: function() {},
  run: function* ()
  {
    let deferred = Promise.defer();
    let currEngine = Services.search.defaultEngine;
    let searchObserver = Task.async(function* search_observer(aSubject, aTopic, aData) {
      let engine = aSubject.QueryInterface(Ci.nsISearchEngine);
      info("Observer: " + aData + " for " + engine.name);

      if (aData != "engine-added")
        return;

      if (engine.name != "POST Search")
        return;

      
      let needle = "Search for something awesome.";
      let document = gBrowser.selectedBrowser.contentDocument;
      let searchText = document.getElementById("searchText");

      let p = promiseContentSearchChange(engine.name);
      Services.search.defaultEngine = engine;
      yield p;

      searchText.value = needle;
      searchText.focus();
      EventUtils.synthesizeKey("VK_RETURN", {});

      registerCleanupFunction(function() {
        Services.search.removeEngine(engine);
        Services.search.defaultEngine = currEngine;
      });


      
      waitForLoad(function() {
        let loadedText = gBrowser.contentDocument.body.textContent;
        ok(loadedText, "search page loaded");
        is(loadedText, "searchterms=" + escape(needle.replace(/\s/g, "+")),
           "Search text should arrive correctly");
        deferred.resolve();
      });
    });
    Services.obs.addObserver(searchObserver, "browser-search-engine-modified", false);
    registerCleanupFunction(function () {
      Services.obs.removeObserver(searchObserver, "browser-search-engine-modified");
    });
    Services.search.addEngine("http://test:80/browser/browser/base/content/test/general/POSTSearchEngine.xml",
                              Ci.nsISearchEngine.DATA_XML, null, false);
    return deferred.promise;
  }
},

{
  desc: "Make sure that a page can't imitate about:home",
  setup: function () { },
  run: function (aSnippetsMap)
  {
    let deferred = Promise.defer();

    let browser = gBrowser.selectedBrowser;
    waitForLoad(() => {
      let button = browser.contentDocument.getElementById("settings");
      ok(button, "Found settings button in test page");
      button.click();

      
      
      function check(n) {
        let win = Services.wm.getMostRecentWindow("Browser:Preferences");
        ok(!win, "Preferences window not showing");
        if (win) {
          win.close();
        }

        if (n > 0) {
          executeSoon(() => check(n-1));
        } else {
          deferred.resolve();
        }
      }

      check(5);
    });

    browser.loadURI("https://example.com/browser/browser/base/content/test/general/test_bug959531.html");
    return deferred.promise;
  }
},

{
  
  desc: "Search suggestion smoke test",
  setup: function() {},
  run: function()
  {
    return Task.spawn(function* () {
      
      let engine = yield promiseNewEngine("searchSuggestionEngine.xml");
      let p = promiseContentSearchChange(engine.name);
      Services.search.currentEngine = engine;
      yield p;

      
      gBrowser.contentWindow.wrappedJSObject.gContentSearchController.remoteTimeout = 5000;

      
      let input = gBrowser.contentDocument.getElementById("searchText");
      input.focus();
      EventUtils.synthesizeKey("x", {});

      
      let table =
        gBrowser.contentDocument.getElementById("searchSuggestionTable");
      let deferred = Promise.defer();
      let observer = new MutationObserver(() => {
        if (input.getAttribute("aria-expanded") == "true") {
          observer.disconnect();
          ok(!table.hidden, "Search suggestion table unhidden");
          deferred.resolve();
        }
      });
      observer.observe(input, {
        attributes: true,
        attributeFilter: ["aria-expanded"],
      });
      yield deferred.promise;

      
      EventUtils.synthesizeKey("a", { accelKey: true });
      EventUtils.synthesizeKey("VK_DELETE", {});
      ok(table.hidden, "Search suggestion table hidden");
    });
  }
},
{
  desc: "Clicking suggestion list while composing",
  setup: function() {},
  run: function()
  {
    return Task.spawn(function* () {
      
      let input = gBrowser.contentDocument.getElementById("searchText");
      input.focus();
      EventUtils.synthesizeComposition({ type: "compositionstart", data: "" },
                                       gBrowser.contentWindow);
      EventUtils.synthesizeCompositionChange({
        composition: {
          string: "x",
          clauses: [
            { length: 1, attr: EventUtils.COMPOSITION_ATTR_RAW_CLAUSE }
          ]
        },
        caret: { start: 1, length: 0 }
      }, gBrowser.contentWindow);

      let searchController =
        gBrowser.contentWindow.wrappedJSObject.gContentSearchController;

      
      let table = searchController._suggestionsList;
      let deferred = Promise.defer();
      let observer = new MutationObserver(() => {
        if (input.getAttribute("aria-expanded") == "true") {
          observer.disconnect();
          ok(!table.hidden, "Search suggestion table unhidden");
          deferred.resolve();
        }
      });
      observer.observe(input, {
        attributes: true,
        attributeFilter: ["aria-expanded"],
      });
      yield deferred.promise;

      
      let expectedURL = Services.search.currentEngine.
                        getSubmission("xbar", null, "homepage").
                        uri.spec;
      let loadPromise = waitForDocLoadAndStopIt(expectedURL);
      let row = table.children[1];
      
      
      
      
      searchController.selectedIndex = 1;
      EventUtils.synthesizeMouseAtCenter(row, {button: 0}, gBrowser.contentWindow);
      yield loadPromise;
      ok(input.value == "x", "Input value did not change");
    });
  }
},
{
  desc: "Cmd+k should focus the search box in the page when the search box in the toolbar is absent",
  setup: function () {
    
    CustomizableUI.removeWidgetFromArea("search-container");
  },
  run: Task.async(function* () {
    let doc = gBrowser.selectedBrowser.contentDocument;
    let logo = doc.getElementById("brandLogo");
    let searchInput = doc.getElementById("searchText");

    EventUtils.synthesizeMouseAtCenter(logo, {});
    isnot(searchInput, doc.activeElement, "Search input should not be the active element.");

    EventUtils.synthesizeKey("k", { accelKey: true });
    yield promiseWaitForCondition(() => doc.activeElement === searchInput);
    is(searchInput, doc.activeElement, "Search input should be the active element.");
    CustomizableUI.reset();
  })
},
{
  desc: "Cmd+k should focus the search box in the toolbar when it's present",
  setup: function () {},
  run: Task.async(function* () {
    let logo = gBrowser.selectedBrowser.contentDocument.getElementById("brandLogo");
    let doc = window.document;
    let searchInput = doc.getElementById("searchbar").textbox.inputField;

    EventUtils.synthesizeMouseAtCenter(logo, {});
    isnot(searchInput, doc.activeElement, "Search bar should not be the active element.");

    EventUtils.synthesizeKey("k", { accelKey: true });
    yield promiseWaitForCondition(() => doc.activeElement === searchInput);
    is(searchInput, doc.activeElement, "Search bar should be the active element.");
  })
},
{
  desc: "Sync button should open about:accounts page with `abouthome` entrypoint",
  setup: function () {},
  run: Task.async(function* () {
    let syncButton = gBrowser.selectedBrowser.contentDocument.getElementById("sync");
    yield EventUtils.synthesizeMouseAtCenter(syncButton, {}, gBrowser.contentWindow);

    yield promiseTabLoadEvent(gBrowser.selectedTab, null, "load");
    is(gBrowser.currentURI.spec, "about:accounts?entrypoint=abouthome",
      "Entry point should be `abouthome`.");
  })
}

];

function test()
{
  waitForExplicitFinish();
  requestLongerTimeout(2);
  ignoreAllUncaughtExceptions();

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);

      if (test.beforeRun)
        yield test.beforeRun();

      
      let tab = gBrowser.selectedTab = gBrowser.addTab("about:blank");

      
      let snippetsPromise = promiseSetupSnippetsMap(tab, test.setup);

      
      yield promiseTabLoadEvent(tab, "about:home", "AboutHomeLoadSnippetsCompleted");

      
      
      let snippetsMap = yield snippetsPromise;

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











function promiseSetupSnippetsMap(aTab, aSetupFn)
{
  let deferred = Promise.defer();
  info("Waiting for snippets map");
  aTab.linkedBrowser.addEventListener("AboutHomeLoadSnippets", function load(event) {
    aTab.linkedBrowser.removeEventListener("AboutHomeLoadSnippets", load, true);

    let cw = aTab.linkedBrowser.contentWindow.wrappedJSObject;
    
    
    cw.ensureSnippetsMapThen(function (aSnippetsMap) {
      aSnippetsMap = Cu.waiveXrays(aSnippetsMap);
      info("Got snippets map: " +
           "{ last-update: " + aSnippetsMap.get("snippets-last-update") +
           ", cached-version: " + aSnippetsMap.get("snippets-cached-version") +
           " }");
      
      aSnippetsMap.set("snippets-last-update", Date.now());
      aSnippetsMap.set("snippets-cached-version", AboutHomeUtils.snippetsVersion);
      
      aSnippetsMap.delete("snippets");
      aSetupFn(aSnippetsMap);
      deferred.resolve(aSnippetsMap);
    });
  }, true, true);
  return deferred.promise;
}









function getNumberOfSearches(aEngineName) {
  let reporter = Components.classes["@mozilla.org/datareporting/service;1"]
                                   .getService()
                                   .wrappedJSObject
                                   .healthReporter;
  ok(reporter, "Health Reporter instance available.");

  return reporter.onInit().then(function onInit() {
    let provider = reporter.getProvider("org.mozilla.searches");
    ok(provider, "Searches provider is available.");

    let m = provider.getMeasurement("counts", 3);
    return m.getValues().then(data => {
      let now = new Date();
      let yday = new Date(now);
      yday.setDate(yday.getDate() - 1);

      
      
      
      
      
      
      return getNumberOfSearchesByDate(aEngineName, data, now) +
             getNumberOfSearchesByDate(aEngineName, data, yday);
    });
  });
}

function getNumberOfSearchesByDate(aEngineName, aData, aDate) {
  if (aData.days.hasDay(aDate)) {
    let id = Services.search.getEngineByName(aEngineName).identifier;

    let day = aData.days.getDay(aDate);
    let field = id + ".abouthome";

    if (day.has(field)) {
      return day.get(field) || 0;
    }
  }

  return 0; 
}

function waitForLoad(cb) {
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function listener() {
    if (browser.currentURI.spec == "about:blank")
      return;
    info("Page loaded: " + browser.currentURI.spec);
    browser.removeEventListener("load", listener, true);

    cb();
  }, true);
}

function promiseWaitForEvent(node, type, capturing) {
  return new Promise((resolve) => {
    node.addEventListener(type, function listener(event) {
      node.removeEventListener(type, listener, capturing);
      resolve(event);
    }, capturing);
  });
}

let promisePrefsOpen = Task.async(function*() {
  if (Services.prefs.getBoolPref("browser.preferences.inContent")) {
    info("Waiting for the preferences tab to open...");
    let event = yield promiseWaitForEvent(gBrowser.tabContainer, "TabOpen", true);
    let tab = event.target;
    yield promiseTabLoadEvent(tab);
    is(tab.linkedBrowser.currentURI.spec, "about:preferences#search", "Should have seen the prefs tab");
    gBrowser.removeTab(tab);
  } else {
    info("Waiting for the preferences window to open...");
    yield new Promise(resolve => {
      let winWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                       getService(Ci.nsIWindowWatcher);
      winWatcher.registerNotification(function onWin(subj, topic, data) {
        if (topic == "domwindowopened" && subj instanceof Ci.nsIDOMWindow) {
          subj.addEventListener("load", function onLoad() {
            subj.removeEventListener("load", onLoad);
            is(subj.document.documentURI, "chrome://browser/content/preferences/preferences.xul", "Should have seen the prefs window");
            winWatcher.unregisterNotification(onWin);
            executeSoon(() => {
              subj.close();
              resolve();
            });
          });
        }
      });
    });
  }
});

function promiseContentSearchChange(newEngineName) {
  return new Promise(resolve => {
    content.addEventListener("ContentSearchService", function listener(aEvent) {
      if (aEvent.detail.type == "CurrentState" &&
          gBrowser.contentWindow.wrappedJSObject.gContentSearchController.defaultEngine.name == newEngineName) {
        content.removeEventListener("ContentSearchService", listener);
        resolve();
      }
    });
  });
}

function promiseNewEngine(basename) {
  info("Waiting for engine to be added: " + basename);
  let addDeferred = Promise.defer();
  let url = getRootDirectory(gTestPath) + basename;
  Services.search.addEngine(url, Ci.nsISearchEngine.TYPE_MOZSEARCH, "", false, {
    onSuccess: function (engine) {
      info("Search engine added: " + basename);
      registerCleanupFunction(() => {
        try {
          Services.search.removeEngine(engine);
        } catch (ex) {  }
      });
      addDeferred.resolve(engine);
    },
    onError: function (errCode) {
      ok(false, "addEngine failed with error code " + errCode);
      addDeferred.reject();
    },
  });
  return addDeferred.promise;
}
