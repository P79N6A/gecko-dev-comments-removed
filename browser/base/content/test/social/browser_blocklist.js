





let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

const URI_EXTENSION_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";
let blocklistURL = "http://test:80/browser/browser/base/content/test/social/blocklist.xml";

let manifest = { 
  name: "provider ok",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
};
let manifest_bad = { 
  name: "provider blocked",
  origin: "https://test1.example.com",
  sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://test1.example.com/browser/browser/base/content/test/moz.png"
};

function test() {
  waitForExplicitFinish();

  runSocialTests(tests, undefined, undefined, function () {
    resetBlocklist(finish); 
  });
}

var tests = {
  testSimpleBlocklist: function(next) {
    
    setAndUpdateBlocklist(blocklistURL, function() {
      ok(Services.blocklist.isAddonBlocklisted("test1.example.com@services.mozilla.org", "0", "0", "0"), "blocking 'blocked'");
      ok(!Services.blocklist.isAddonBlocklisted("example.com@services.mozilla.org", "0", "0", "0"), "not blocking 'good'");
      resetBlocklist(function() {
        ok(!Services.blocklist.isAddonBlocklisted("test1.example.com@services.mozilla.org", "0", "0", "0"), "blocklist cleared");
        next();
      });
    });
  },
  testAddingNonBlockedProvider: function(next) {
    function finish(isgood) {
      ok(isgood, "adding non-blocked provider ok");
      Services.prefs.clearUserPref("social.manifest.good");
      resetBlocklist(next);
    }
    setManifestPref("social.manifest.good", manifest);
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest, function(provider) {
          try {
            SocialService.removeProvider(provider.origin, function() {
              ok(true, "added and removed provider");
              finish(true);
            });
          } catch(e) {
            ok(false, "SocialService.removeProvider threw exception: " + e);
            finish(false);
          }
        });
      } catch(e) {
        ok(false, "SocialService.addProvider threw exception: " + e);
        finish(false);
      }
    });
  },
  testAddingBlockedProvider: function(next) {
    function finish(good) {
      ok(good, "Unable to add blocklisted provider");
      Services.prefs.clearUserPref("social.manifest.blocked");
      resetBlocklist(next);
    }
    setManifestPref("social.manifest.blocked", manifest_bad);
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest_bad, function(provider) {
          ok(false, "SocialService.addProvider should throw blocklist exception");
          finish(false);
        });
      } catch(e) {
        ok(true, "SocialService.addProvider should throw blocklist exception: " + e);
        finish(true);
      }
    });
  },
  testInstallingBlockedProvider: function(next) {
    function finish(good) {
      ok(good, "Unable to add blocklisted provider");
      Services.prefs.clearUserPref("social.whitelist");
      resetBlocklist(next);
    }
    let activationURL = manifest_bad.origin + "/browser/browser/base/content/test/social/social_activate.html"
    addTab(activationURL, function(tab) {
      let doc = tab.linkedBrowser.contentDocument;
      let installFrom = doc.nodePrincipal.origin;
      
      
      Services.prefs.setCharPref("social.whitelist", installFrom);
      setAndUpdateBlocklist(blocklistURL, function() {
        try {
          
          
          Social.installProvider(doc, manifest_bad, function(addonManifest) {
            gBrowser.removeTab(tab);
            finish(false);
          });
        } catch(e) {
          gBrowser.removeTab(tab);
          finish(true);
        }
      });
    });
  },
  testBlockingExistingProvider: function(next) {
    let windowWasClosed = false;
    function finish() {
      waitForCondition(function() windowWasClosed, function() {
        Services.wm.removeListener(listener);
        next();
      }, "blocklist dialog was closed");
    }

    let listener = {
      _window: null,
      onOpenWindow: function(aXULWindow) {
        Services.wm.removeListener(this);
        this._window = aXULWindow;
        let domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIDOMWindow);

        domwindow.addEventListener("unload", function _unload() {
          domwindow.removeEventListener("unload", _unload, false);
          windowWasClosed = true;
        }, false);
        info("dialog opened, waiting for focus");
        waitForFocus(function() {
          is(domwindow.document.location.href, URI_EXTENSION_BLOCKLIST_DIALOG, "dialog opened and focused");
          executeSoon(function() {
            domwindow.close();
          });
        }, domwindow);
      },
      onCloseWindow: function(aXULWindow) { },
      onWindowTitleChange: function(aXULWindow, aNewTitle) { }
    };

    Services.wm.addListener(listener);

    setManifestPref("social.manifest.blocked", manifest_bad);
    try {
      SocialService.addProvider(manifest_bad, function(provider) {
        
        
        SocialService.registerProviderListener(function providerListener(topic, origin, providers) {
          if (topic != "provider-disabled")
            return;
          SocialService.unregisterProviderListener(providerListener);
          is(origin, provider.origin, "provider disabled");
          SocialService.getProvider(provider.origin, function(p) {
            ok(p == null, "blocklisted provider disabled");
            Services.prefs.clearUserPref("social.manifest.blocked");
            resetBlocklist(finish);
          });
        });
        
        
        setAndUpdateBlocklist(blocklistURL);
      });
    } catch(e) {
      ok(false, "unable to add provider " + e);
      finish();
    }
  }
}
