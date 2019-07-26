





let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

const URI_EXTENSION_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";
let blocklistURL = "http://test:80/browser/browser/base/content/test/social/blocklist.xml";
let blocklistEmpty = "http://test:80/browser/browser/base/content/test/social/blocklistEmpty.xml";

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
    resetBlocklist(); 
    finish();
  });
}

var tests = {
  testSimpleBlocklist: function(next) {
    
    var blocklist = Components.classes["@mozilla.org/extensions/blocklist;1"]
                        .getService(Components.interfaces.nsIBlocklistService);
    setAndUpdateBlocklist(blocklistURL, function() {
      ok(blocklist.isAddonBlocklisted("test1.example.com@services.mozilla.org", "0", "0", "0"), "blocking 'blocked'");
      ok(!blocklist.isAddonBlocklisted("example.com@services.mozilla.org", "0", "0", "0"), "not blocking 'good'");
      setAndUpdateBlocklist(blocklistEmpty, function() {
        ok(!blocklist.isAddonBlocklisted("test1.example.com@services.mozilla.org", "0", "0", "0"), "blocklist cleared");
        next();
      });
    });
  },
  testAddingNonBlockedProvider: function(next) {
    function finish(isgood) {
      ok(isgood, "adding non-blocked provider ok");
      Services.prefs.clearUserPref("social.manifest.good");
      setAndUpdateBlocklist(blocklistEmpty, next);
    }
    Services.prefs.setCharPref("social.manifest.good", JSON.stringify(manifest));
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest, function(provider) {
          if (provider) {
            SocialService.removeProvider(provider.origin, function() {
              ok(true, "added and removed provider");
              finish(true);
            });
          } else {
            finish(false);
          }
        });
      } catch(e) {
        dump(e+" - "+e.stack+"\n");
        finish(false);
      }
    });
  },
  testAddingBlockedProvider: function(next) {
    function finish(good) {
      ok(good, "Unable to add blocklisted provider");
      Services.prefs.clearUserPref("social.manifest.blocked");
      setAndUpdateBlocklist(blocklistEmpty, next);
    }
    Services.prefs.setCharPref("social.manifest.blocked", JSON.stringify(manifest_bad));
    setAndUpdateBlocklist(blocklistURL, function() {
      try {
        SocialService.addProvider(manifest_bad, function(provider) {
          if (provider) {
            SocialService.removeProvider(provider.origin, function() {
              finish(false);
            });
          } else {
            finish(true);
          }
        });
      } catch(e) {
        finish(true);
      }
    });
  },
  testInstallingBlockedProvider: function(next) {
    function finish(good) {
      ok(good, "Unable to add blocklisted provider");
      Services.prefs.clearUserPref("social.whitelist");
      setAndUpdateBlocklist(blocklistEmpty, next);
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

    addWindowListener(URI_EXTENSION_BLOCKLIST_DIALOG,  function(win) {
      win.close();
      ok(true, "window closed");
    });

    function finish(good) {
      ok(good, "blocklisted provider removed");
      Services.prefs.clearUserPref("social.manifest.blocked");
      setAndUpdateBlocklist(blocklistEmpty, next);
    }
    Services.prefs.setCharPref("social.manifest.blocked", JSON.stringify(manifest_bad));
    SocialService.addProvider(manifest_bad, function(provider) {
      if (provider) {
        setAndUpdateBlocklist(blocklistURL, function() {
          SocialService.getProvider(provider.origin, function(p) {
            finish(p==null);
          })
        });
      } else {
        finish(false);
      }
    });
  }
}
