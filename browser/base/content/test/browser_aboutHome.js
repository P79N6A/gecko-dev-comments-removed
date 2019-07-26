



XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

registerCleanupFunction(function() {
  
  try {
    Services.prefs.clearUserPref("network.cookies.cookieBehavior");
  } catch (ex) {}
  try {
    Services.prefs.clearUserPref("network.cookie.lifetimePolicy");
  } catch (ex) {}
});

let gTests = [

{
  desc: "Check that clearing cookies does not clear storage",
  setup: function ()
  {
    Cc["@mozilla.org/dom/storagemanager;1"]
      .getService(Ci.nsIObserver)
      .observe(null, "cookie-changed", "cleared");
  },
  run: function (aSnippetsMap)
  {
    isnot(aSnippetsMap.get("snippets-last-update"), null);
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
       "A default snippet is visible.");
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
       "A default snippet is visible.");

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
  desc: "Check that performing a search fires a search event.",
  setup: function () { },
  run: function () {
    let deferred = Promise.defer();
    let doc = gBrowser.contentDocument;

    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      is(e.detail, doc.documentElement.getAttribute("searchEngineName"), "Detail is search engine name");

      gBrowser.stop();
      deferred.resolve();
    }, true, true);

    doc.getElementById("searchText").value = "it works";
    doc.getElementById("searchSubmit").click();
    return deferred.promise;
  }
},

{
  desc: "Check that performing a search records to Firefox Health Report.",
  setup: function () { },
  run: function () {
    try {
      let cm = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
      cm.getCategoryEntry("healthreport-js-provider", "SearchesProvider");
    } catch (ex) {
      
      return;
    }

    let deferred = Promise.defer();
    let doc = gBrowser.contentDocument;

    
    
    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      executeSoon(gBrowser.stop.bind(gBrowser));
      let reporter = Components.classes["@mozilla.org/datareporting/service;1"]
                                       .getService()
                                       .wrappedJSObject
                                       .healthReporter;
      ok(reporter, "Health Reporter instance available.");

      reporter.onInit().then(function onInit() {
        let provider = reporter.getProvider("org.mozilla.searches");
        ok(provider, "Searches provider is available.");

        let engineName = doc.documentElement.getAttribute("searchEngineName").toLowerCase();

        let m = provider.getMeasurement("counts", 1);
        m.getValues().then(function onValues(data) {
          let now = new Date();
          ok(data.days.hasDay(now), "Have data for today.");

          let day = data.days.getDay(now);
          let field = engineName + ".abouthome";
          ok(day.has(field), "Have data for about home on this engine.");

          
          is(day.get(field), 2, "Have searches recorded.");

          deferred.resolve();
        });

      });
    }, true, true);

    doc.getElementById("searchText").value = "a search";
    doc.getElementById("searchSubmit").click();
    return deferred.promise;
  }
},

];

function test()
{
  waitForExplicitFinish();

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);

      let tab = yield promiseNewTabLoadEvent("about:home", "DOMContentLoaded");

      
      
      
      
      let promise = promiseBrowserAttributes(tab);
      
      let snippetsMap = yield promiseSetupSnippetsMap(tab, test.setup);
      
      yield promise;

      yield test.run(snippetsMap);

      gBrowser.removeCurrentTab();
    }

    finish();
  });
}










function promiseNewTabLoadEvent(aUrl, aEventType="load")
{
  let deferred = Promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(aUrl);
  tab.linkedBrowser.addEventListener(aEventType, function load(event) {
    tab.linkedBrowser.removeEventListener(aEventType, load, true);
    deferred.resolve(tab);
  }, true);
  return deferred.promise;
}











function promiseSetupSnippetsMap(aTab, aSetupFn)
{
  let deferred = Promise.defer();
  let cw = aTab.linkedBrowser.contentWindow.wrappedJSObject;
  cw.ensureSnippetsMapThen(function (aSnippetsMap) {
    
    aSnippetsMap.set("snippets-last-update", Date.now());
    
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
      if (mutation.attributeName == "snippetsURL" &&
          docElt.getAttribute("snippetsURL") != "nonexistent://test") {
        docElt.setAttribute("snippetsURL", "nonexistent://test");
      }

      
      if (mutation.attributeName == "searchEngineURL") {
        observer.disconnect();
        
        executeSoon(function() deferred.resolve());
        break;
      }
    }
  });
  observer.observe(docElt, { attributes: true });

  return deferred.promise;
}
