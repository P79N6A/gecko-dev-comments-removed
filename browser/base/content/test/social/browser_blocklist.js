





let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

const URI_EXTENSION_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";
let blocklistURL = "http://example.com/browser/browser/base/content/test/social/blocklist.xml";

let manifest = { 
  name: "provider ok",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
};
let manifest_bad = { 
  name: "provider blocked",
  origin: "https://test1.example.com",
  sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://test1.example.com/browser/browser/base/content/test/general/moz.png"
};

function test() {
  waitForExplicitFinish();
  
  Services.prefs.setBoolPref("extensions.logging.enabled", true);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("extensions.logging.enabled");
  });

  runSocialTests(tests, undefined, undefined, function () {
    resetBlocklist(finish); 
  });
}

var tests = {
  testSimpleBlocklist: function(next) {
    
    setAndUpdateBlocklist(blocklistURL, function() {
      ok(Services.blocklist.isAddonBlocklisted(SocialService.createWrapper(manifest_bad)), "blocking 'blocked'");
      ok(!Services.blocklist.isAddonBlocklisted(SocialService.createWrapper(manifest)), "not blocking 'good'");
      resetBlocklist(function() {
        ok(!Services.blocklist.isAddonBlocklisted(SocialService.createWrapper(manifest_bad)), "blocklist cleared");
        next();
      });
    });
  },
  testAddingNonBlockedProvider: function(next) {
    function finishTest(isgood) {
      ok(isgood, "adding non-blocked provider ok");
      Services.prefs.clearUserPref("social.manifest.good");
      resetBlocklist(next);
    }
    setManifestPref("social.manifest.good", manifest);
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest, function(provider) {
          try {
            SocialService.disableProvider(provider.origin, function() {
              ok(true, "added and removed provider");
              finishTest(true);
            });
          } catch(e) {
            ok(false, "SocialService.disableProvider threw exception: " + e);
            finishTest(false);
          }
        });
      } catch(e) {
        ok(false, "SocialService.addProvider threw exception: " + e);
        finishTest(false);
      }
    });
  },
  testAddingBlockedProvider: function(next) {
    function finishTest(good) {
      ok(good, "Unable to add blocklisted provider");
      Services.prefs.clearUserPref("social.manifest.blocked");
      resetBlocklist(next);
    }
    setManifestPref("social.manifest.blocked", manifest_bad);
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest_bad, function(provider) {
          SocialService.disableProvider(provider.origin, function() {
            ok(false, "SocialService.addProvider should throw blocklist exception");
            finishTest(false);
          });
        });
      } catch(e) {
        ok(true, "SocialService.addProvider should throw blocklist exception: " + e);
        finishTest(true);
      }
    });
  },
  testInstallingBlockedProvider: function(next) {
    function finishTest(good) {
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
          
          
          let data = {
            origin: doc.nodePrincipal.origin,
            url: doc.location.href,
            manifest: manifest_bad,
            window: window
          }
          Social.installProvider(data, function(addonManifest) {
            gBrowser.removeTab(tab);
            finishTest(false);
          });
        } catch(e) {
          gBrowser.removeTab(tab);
          finishTest(true);
        }
      });
    });
  },
  testBlockingExistingProvider: function(next) {
    let listener = {
      _window: null,
      onOpenWindow: function(aXULWindow) {
        Services.wm.removeListener(this);
        this._window = aXULWindow;
        let domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIDOMWindow);

        domwindow.addEventListener("load", function _load() {
          domwindow.removeEventListener("load", _load, false);

          domwindow.addEventListener("unload", function _unload() {
            domwindow.removeEventListener("unload", _unload, false);
            info("blocklist window was closed");
            Services.wm.removeListener(listener);
            next();
          }, false);

          is(domwindow.document.location.href, URI_EXTENSION_BLOCKLIST_DIALOG, "dialog opened and focused");
          
          
          executeSoon(() => {
            let cancelButton = domwindow.document.documentElement.getButton("cancel");
            info("***** hit the cancel button\n");
            cancelButton.doCommand();
          });
        }, false);
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
            resetBlocklist();
          });
        });
        
        
        setAndUpdateBlocklist(blocklistURL);
      });
    } catch(e) {
      ok(false, "unable to add provider " + e);
      next();
    }
  }
}
