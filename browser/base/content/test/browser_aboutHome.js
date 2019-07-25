



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
  desc: "Check that rejecting cookies does not prevent page from working",
  setup: function ()
  {
    Services.prefs.setIntPref("network.cookies.cookieBehavior", 2);
  },
  run: function ()
  {
    let storage = getStorage();
    isnot(storage.getItem("search-engine"), null);
    try {
      Services.prefs.clearUserPref("network.cookies.cookieBehavior");
    } catch (ex) {}
    executeSoon(runNextTest);
  }
},

{
  desc: "Check that asking for cookies does not prevent page from working",
  setup: function ()
  {
    Services.prefs.setIntPref("network.cookie.lifetimePolicy", 1);
  },
  run: function ()
  {
    let storage = getStorage();
    isnot(storage.getItem("search-engine"), null);
    try {
      Services.prefs.clearUserPref("network.cookie.lifetimePolicy");
    } catch (ex) {}
    executeSoon(runNextTest);
  }
},

{
  desc: "Check that clearing cookies does not prevent page from working",
  setup: function ()
  {
    Components.classes["@mozilla.org/dom/storagemanager;1"].
    getService(Components.interfaces.nsIObserver).
    observe(null, "cookie-changed", "cleared");
  },
  run: function ()
  {
    let storage = getStorage();
    isnot(storage.getItem("search-engine"), null);
    executeSoon(runNextTest);
  }
},

{
  desc: "Check normal status is working",
  setup: function ()
  {
  },
  run: function ()
  {
    let storage = getStorage();
    isnot(storage.getItem("search-engine"), null);
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
    let snippetsElt = doc.getElementById("defaultSnippets");
    ok(snippetsElt, "Found default snippets element")
    ok(Array.some(snippetsElt.getElementsByTagName("span"), function(elt) {
      return !elt.hidden;
    }), "A default snippet is visible.");
    executeSoon(runNextTest);
  }
},

];

function test()
{
  waitForExplicitFinish();

  
  
  
  
  
  
  Cc["@mozilla.org/browser/clh;1"].getService(Ci.nsIBrowserHandler).defaultArgs;

  
  
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
    tab.linkedBrowser.addEventListener("load", function (event) {
      tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
      
      executeSoon(test.run);
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
                  getCodebasePrincipal(Services.io.newURI("about:home", null, null));
  let dsm = Components.classes["@mozilla.org/dom/storagemanager;1"].
            getService(Components.interfaces.nsIDOMStorageManager);
  return dsm.getLocalStorageForPrincipal(principal, "");
};
