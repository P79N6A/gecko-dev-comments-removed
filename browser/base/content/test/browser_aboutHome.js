



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
  run: function ()
  {
    let storage = getStorage();
    isnot(storage.getItem("snippets-last-update"), null);
    executeSoon(runNextTest);
  }
},

{
  desc: "Check default snippets are shown",
  setup: function ()
  {
  },
  run: function ()
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;
    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element")
    is(snippetsElt.getElementsByTagName("span").length, 1,
       "A default snippet is visible.");
    executeSoon(runNextTest);
  }
},

{
  desc: "Check default snippets are shown if snippets are invalid xml",
  setup: function ()
  {
    let storage = getStorage();
    
    storage.setItem("snippets", "<p><b></p></b>");
  },
  run: function ()
  {
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    is(snippetsElt.getElementsByTagName("span").length, 1,
       "A default snippet is visible.");
    let storage = getStorage();
    storage.removeItem("snippets");
    executeSoon(runNextTest);
  }
},
{
  desc: "Check that search engine logo has alt text",
  setup: function ()
  {
  },
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

    executeSoon(runNextTest);
  }
},
{
  desc: "Check that performing a search fires a search event.",
  setup: function () { },
  run: function () {
    let doc = gBrowser.contentDocument;

    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      is(e.detail, doc.documentElement.getAttribute("searchEngineName"), "Detail is search engine name");

      gBrowser.stop();
      executeSoon(runNextTest);
    }, true, true);

    doc.getElementById("searchText").value = "it works";
    doc.getElementById("searchSubmit").click();
  },
},
{
  desc: "Check that performing a search records to Firefox Health Report.",
  setup: function () { },
  run: function () {
    try {
      let cm = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
      cm.getCategoryEntry("healthreport-js-provider", "SearchesProvider");
    } catch (ex) {
      
      runNextTest();
      return;
    }

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

          executeSoon(runNextTest);
        });

      });
    }, true, true);

    doc.getElementById("searchText").value = "a search";
    doc.getElementById("searchSubmit").click();
  },
},
];

function test()
{
  waitForExplicitFinish();

  
  
  let storage = getStorage();
  storage.setItem("snippets-last-update", Date.now());
  storage.removeItem("snippets");

  executeSoon(runNextTest);
}

function runNextTest()
{
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  if (gTests.length) {
    let test = gTests.shift();
    info(test.desc);
    test.setup();
    let tab = gBrowser.selectedTab = gBrowser.addTab("about:home");
    tab.linkedBrowser.addEventListener("load", function load(event) {
      tab.linkedBrowser.removeEventListener("load", load, true);

      let observer = new MutationObserver(function (mutations) {
        for (let mutation of mutations) {
          if (mutation.attributeName == "searchEngineURL") {
            observer.disconnect();
            executeSoon(test.run);
            return;
          }
        }
      });
      let docElt = tab.linkedBrowser.contentDocument.documentElement;
      observer.observe(docElt, { attributes: true });
    }, true);
  }
  else {
    finish();
  }
}

function getStorage()
{
  let aboutHomeURI = Services.io.newURI("moz-safe-about:home", null, null);
  let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"].
                  getService(Components.interfaces.nsIScriptSecurityManager).
                  getNoAppCodebasePrincipal(Services.io.newURI("about:home", null, null));
  let dsm = Components.classes["@mozilla.org/dom/storagemanager;1"].
            getService(Components.interfaces.nsIDOMStorageManager);
  return dsm.getLocalStorageForPrincipal(principal, "");
};
