



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

function waitForCondition(condition, nextTest, errorMsg) {
  var tries = 0;
  var interval = setInterval(function() {
    if (tries >= 30) {
      ok(false, errorMsg);
      moveOn();
    }
    if (condition()) {
      moveOn();
    }
    tries++;
  }, 100);
  var moveOn = function() { clearInterval(interval); nextTest(); };
}



function promiseSocialUrlNotRemembered(url) {
  let deferred = Promise.defer();
  let uri = Services.io.newURI(url, null, null);
  PlacesUtils.asyncHistory.isURIVisited(uri, function(aURI, aIsVisited) {
    ok(!aIsVisited, "social URL " + url + " should not be in global history");
    deferred.resolve();
  });
  return deferred.promise;
}

let gURLsNotRemembered = [];


function checkProviderPrefsEmpty(isError) {
  let MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");
  let prefs = MANIFEST_PREFS.getChildList("", []);
  let c = 0;
  for (let pref of prefs) {
    if (MANIFEST_PREFS.prefHasUserValue(pref)) {
      info("provider [" + pref + "] manifest left installed from previous test");
      c++;
    }
  }
  is(c, 0, "all provider prefs uninstalled from previous test");
  is(Social.providers.length, 0, "all providers uninstalled from previous test " + Social.providers.length);
}

function defaultFinishChecks() {
  checkProviderPrefsEmpty(true);
  finish();
}

function runSocialTestWithProvider(manifest, callback, finishcallback) {
  let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

  let manifests = Array.isArray(manifest) ? manifest : [manifest];

  
  function finishCleanUp() {
    for (let i = 0; i < manifests.length; i++) {
      let m = manifests[i];
      for (let what of ['sidebarURL', 'workerURL', 'iconURL']) {
        if (m[what]) {
          yield promiseSocialUrlNotRemembered(m[what]);
        }
      };
    }
    for (let i = 0; i < gURLsNotRemembered.length; i++) {
      yield promiseSocialUrlNotRemembered(gURLsNotRemembered[i]);
    }
    gURLsNotRemembered = [];
  }

  info("runSocialTestWithProvider: " + manifests.toSource());

  let finishCount = 0;
  function finishIfDone(callFinish) {
    finishCount++;
    if (finishCount == manifests.length)
      Task.spawn(finishCleanUp).then(finishcallback || defaultFinishChecks);
  }
  function removeAddedProviders(cleanup) {
    manifests.forEach(function (m) {
      
      let callback = cleanup ? function () {} : finishIfDone;
      
      let removeProvider = SocialService.removeProvider.bind(SocialService);
      if (cleanup) {
        removeProvider = function (origin, cb) {
          try {
            SocialService.removeProvider(origin, cb);
          } catch (ex) {
            
            if (ex.message.indexOf("SocialService.removeProvider: no provider with origin") == 0)
              return;
            info("Failed to clean up provider " + origin + ": " + ex);
          }
        }
      }
      removeProvider(m.origin, callback);
    });
  }
  function finishSocialTest(cleanup) {
    
    
    Services.prefs.clearUserPref("social.enabled");
    removeAddedProviders(cleanup);
  }

  let providersAdded = 0;
  let firstProvider;

  manifests.forEach(function (m) {
    SocialService.addProvider(m, function(provider) {

      providersAdded++;
      info("runSocialTestWithProvider: provider added");

      
      if (provider.origin == manifests[0].origin) {
        firstProvider = provider;
      }

      
      
      if (providersAdded == manifests.length) {
        
        Social.provider = firstProvider;

        registerCleanupFunction(function () {
          finishSocialTest(true);
        });
        callback(finishSocialTest);
      }
    });
  });
}

function runSocialTests(tests, cbPreTest, cbPostTest, cbFinish) {
  let testIter = Iterator(tests);
  let providersAtStart = Social.providers.length;

  if (cbPreTest === undefined) {
    cbPreTest = function(cb) {cb()};
  }
  if (cbPostTest === undefined) {
    cbPostTest = function(cb) {cb()};
  }

  function runNextTest() {
    let name, func;
    try {
      [name, func] = testIter.next();
    } catch (err if err instanceof StopIteration) {
      
      (cbFinish || defaultFinishChecks)();
      return;
    }
    
    
    executeSoon(function() {
      function cleanupAndRunNextTest() {
        info("sub-test " + name + " complete");
        cbPostTest(runNextTest);
      }
      cbPreTest(function() {
        is(providersAtStart, Social.providers.length, "no new providers left enabled");
        info("sub-test " + name + " starting");
        try {
          func.call(tests, cleanupAndRunNextTest);
        } catch (ex) {
          ok(false, "sub-test " + name + " failed: " + ex.toString() +"\n"+ex.stack);
          cleanupAndRunNextTest();
        }
      })
    });
  }
  runNextTest();
}



function checkSocialUI(win) {
  win = win || window;
  let doc = win.document;
  let provider = Social.provider;
  let enabled = win.SocialUI.enabled;
  let active = Social.providers.length > 0 && !win.SocialUI._chromeless &&
               !PrivateBrowsingUtils.isWindowPrivate(win);

  function isbool(a, b, msg) {
    is(!!a, !!b, msg);
  }
  isbool(win.SocialSidebar.canShow, enabled, "social sidebar active?");
  if (enabled)
    isbool(win.SocialSidebar.opened, enabled, "social sidebar open?");
  isbool(win.SocialChatBar.isAvailable, enabled && Social.haveLoggedInUser(), "chatbar available?");
  isbool(!win.SocialChatBar.chatbar.hidden, enabled && Social.haveLoggedInUser(), "chatbar visible?");

  let markVisible = enabled && provider.pageMarkInfo;
  let canMark = markVisible && win.SocialMark.canMarkPage(win.gBrowser.currentURI);
  isbool(!win.SocialMark.button.hidden, markVisible, "SocialMark button visible?");
  isbool(!win.SocialMark.button.disabled, canMark, "SocialMark button enabled?");
  isbool(!doc.getElementById("social-toolbar-item").hidden, active, "toolbar items visible?");
  if (active) {
    if (!enabled) {
      ok(!win.SocialToolbar.button.style.listStyleImage, "toolbar button is default icon");
    } else {
      is(win.SocialToolbar.button.style.listStyleImage, 'url("' + Social.defaultProvider.iconURL + '")', "toolbar button has provider icon");
    }
  }
  
  if (provider) {
    for (let id of ["menu_socialSidebar", "menu_socialAmbientMenu"])
      is(document.getElementById(id).getAttribute("label"), Social.provider.name, "element has the provider name");
  }

  
  isbool(!doc.getElementById("Social:Toggle").hidden, active, "Social:Toggle visible?");
  isbool(!doc.getElementById("Social:ToggleNotifications").hidden, enabled, "Social:ToggleNotifications visible?");
  isbool(!doc.getElementById("Social:FocusChat").hidden, enabled && Social.haveLoggedInUser(), "Social:FocusChat visible?");
  isbool(doc.getElementById("Social:FocusChat").getAttribute("disabled"), enabled ? "false" : "true", "Social:FocusChat disabled?");
  is(doc.getElementById("Social:TogglePageMark").getAttribute("disabled"), canMark ? "false" : "true", "Social:TogglePageMark enabled?");

  
  isbool(!doc.getElementById("socialActiveBroadcaster").hidden, active, "socialActiveBroadcaster hidden?");
}


function updateBlocklist(aCallback) {
  var blocklistNotifier = Cc["@mozilla.org/extensions/blocklist;1"]
                          .getService(Ci.nsITimerCallback);
  var observer = function() {
    Services.obs.removeObserver(observer, "blocklist-updated");
    if (aCallback)
      executeSoon(aCallback);
  };
  Services.obs.addObserver(observer, "blocklist-updated", false);
  blocklistNotifier.notify(null);
}

function setAndUpdateBlocklist(aURL, aCallback) {
  Services.prefs.setCharPref("extensions.blocklist.url", aURL);
  updateBlocklist(aCallback);
}

function resetBlocklist(aCallback) {
  Services.prefs.clearUserPref("extensions.blocklist.url");
  updateBlocklist(aCallback);
}

function setManifestPref(name, manifest) {
  let string = Cc["@mozilla.org/supports-string;1"].
               createInstance(Ci.nsISupportsString);
  string.data = JSON.stringify(manifest);
  Services.prefs.setComplexValue(name, Ci.nsISupportsString, string);
}

function getManifestPrefname(aManifest) {
  
  let originUri = Services.io.newURI(aManifest.origin, null, null);
  return "social.manifest." + originUri.hostPort.replace('.','-');
}

function setBuiltinManifestPref(name, manifest) {
  
  manifest.builtin = true;
  let string = Cc["@mozilla.org/supports-string;1"].
               createInstance(Ci.nsISupportsString);
  string.data = JSON.stringify(manifest);
  Services.prefs.getDefaultBranch(null).setComplexValue(name, Ci.nsISupportsString, string);
  
  let stored = Services.prefs.getComplexValue(name, Ci.nsISupportsString).data;
  is(stored, string.data, "manifest '"+name+"' stored in default prefs");
  
  delete manifest.builtin;
  
  ok(!Services.prefs.prefHasUserValue(name), "manifest '"+name+"' is not in user-prefs");
}

function resetBuiltinManifestPref(name) {
  Services.prefs.getDefaultBranch(null).deleteBranch(name);
  is(Services.prefs.getDefaultBranch(null).getPrefType(name),
     Services.prefs.PREF_INVALID, "default manifest removed");
}

function addWindowListener(aURL, aCallback) {
  Services.wm.addListener({
    onOpenWindow: function(aXULWindow) {
      info("window opened, waiting for focus");
      Services.wm.removeListener(this);

      var domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindow);
      waitForFocus(function() {
        is(domwindow.document.location.href, aURL, "window opened and focused");
        executeSoon(function() {
          aCallback(domwindow);
        });
      }, domwindow);
    },
    onCloseWindow: function(aXULWindow) { },
    onWindowTitleChange: function(aXULWindow, aNewTitle) { }
  });
}

function addTab(url, callback) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    executeSoon(function() {callback(tab)});
  }, true);
}

function selectBrowserTab(tab, callback) {
  if (gBrowser.selectedTab == tab) {
    executeSoon(function() {callback(tab)});
    return;
  }
  gBrowser.tabContainer.addEventListener("TabSelect", function onTabSelect() {
    gBrowser.tabContainer.removeEventListener("TabSelect", onTabSelect, false);
    is(gBrowser.selectedTab, tab, "browser tab is selected");
    executeSoon(function() {callback(tab)});
  });
  gBrowser.selectedTab = tab;
}

function loadIntoTab(tab, url, callback) {
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    executeSoon(function() {callback(tab)});
  }, true);
  tab.linkedBrowser.loadURI(url);
}

