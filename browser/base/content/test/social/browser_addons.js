

let AddonManager = Cu.import("resource://gre/modules/AddonManager.jsm", {}).AddonManager;
let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

const ADDON_TYPE_SERVICE     = "service";
const ID_SUFFIX              = "@services.mozilla.org";
const STRING_TYPE_NAME       = "type.%ID%.name";
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
};
let manifest2 = { 
  name: "provider 2",
  origin: "https://test1.example.com",
  sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://test1.example.com/browser/browser/base/content/test/moz.png"
};

function test() {
  waitForExplicitFinish();

  Services.prefs.setCharPref("social.manifest.good", JSON.stringify(manifest));
  Services.prefs.setBoolPref("social.remote-install.enabled", true);
  runSocialTests(tests, undefined, undefined, function () {
    Services.prefs.clearUserPref("social.remote-install.enabled");
    Services.prefs.clearUserPref("social.manifest.good");
    
    Services.prefs.clearUserPref("social.whitelist");
    Services.prefs.clearUserPref("social.directories");
    finish();
  });
}

function installListener(next) {
  let expectEvent = "onInstalling";
  return {
    onInstalling: function(addon) {
      is(expectEvent, "onInstalling", "install started");
      is(addon.manifest.origin, manifest2.origin, "provider about to be installed");
      expectEvent = "onInstalled";
    },
    onInstalled: function(addon) {
      is(addon.manifest.origin, manifest2.origin, "provider installed");
      expectEvent = "onUninstalling";
    },
    onUninstalling: function(addon) {
      is(expectEvent, "onUninstalling", "uninstall started");
      is(addon.manifest.origin, manifest2.origin, "provider about to be uninstalled");
      expectEvent = "onUninstalled";
    },
    onUninstalled: function(addon) {
      is(expectEvent, "onUninstalled", "provider has been uninstalled");
      is(addon.manifest.origin, manifest2.origin, "provider uninstalled");
      AddonManager.removeAddonListener(this);
      next();
    }
  };
}

var tests = {
  testInstalledProviders: function(next) {
    
    
    
    AddonManager.getAddonsByTypes([ADDON_TYPE_SERVICE], function(addons) {
      for (let addon of addons) {
        if (addon.manifest.origin == manifest.origin) {
          ok(true, "test addon is installed");
          next();
          return;
        }
      }
      
      ok(false, "test addon is not installed");
      next();
    });
  },
  testAddonEnableToggle: function(next) {
    
    
    

    let expectEvent;
    let listener = {
      onEnabled: function(addon) {
        is(expectEvent, "onEnabled", "provider onEnabled");
        ok(!addon.userDisabled, "provider enabled");
        executeSoon(function() {
          
          expectEvent = "onDisabling";
          addon.userDisabled = !addon.userDisabled;
        });
      },
      onEnabling: function(addon) {
        is(expectEvent, "onEnabling", "provider onEnabling");
        expectEvent = "onEnabled";
      },
      onDisabled: function(addon) {
        is(expectEvent, "onDisabled", "provider onDisabled");
        ok(addon.userDisabled, "provider disabled");
        executeSoon(function() {
          
          AddonManager.removeAddonListener(listener);
          addon.userDisabled = !addon.userDisabled;
          next();
        });
      },
      onDisabling: function(addon) {
        is(expectEvent, "onDisabling", "provider onDisabling");
        expectEvent = "onDisabled";
      }
    };
    AddonManager.addAddonListener(listener);

    AddonManager.getAddonsByTypes([ADDON_TYPE_SERVICE], function(addons) {
      for (let addon of addons) {
        expectEvent = addon.userDisabled ? "onEnabling" : "onDisabling";
        addon.userDisabled = !addon.userDisabled;
        
        return;
      }
      ok(false, "no addons toggled");
      next();
    });
  },
  testProviderEnableToggle: function(next) {
    
    

    let expectEvent;

    let listener = {
      onEnabled: function(addon) {
        is(expectEvent, "onEnabled", "provider onEnabled");
        is(addon.manifest.origin, manifest.origin, "provider enabled");
        ok(!addon.userDisabled, "provider !userDisabled");
      },
      onEnabling: function(addon) {
        is(expectEvent, "onEnabling", "provider onEnabling");
        is(addon.manifest.origin, manifest.origin, "provider about to be enabled");
        expectEvent = "onEnabled";
      },
      onDisabled: function(addon) {
        is(expectEvent, "onDisabled", "provider onDisabled");
        is(addon.manifest.origin, manifest.origin, "provider disabled");
        ok(addon.userDisabled, "provider userDisabled");
      },
      onDisabling: function(addon) {
        is(expectEvent, "onDisabling", "provider onDisabling");
        is(addon.manifest.origin, manifest.origin, "provider about to be disabled");
        expectEvent = "onDisabled";
      }
    };
    AddonManager.addAddonListener(listener);

    expectEvent = "onEnabling";
    SocialService.addBuiltinProvider(manifest.origin, function(provider) {
      expectEvent = "onDisabling";
      SocialService.removeProvider(provider.origin, function() {
        AddonManager.removeAddonListener(listener);
        next();
      });
    });
  },
  testForeignInstall: function(next) {
    AddonManager.addAddonListener(installListener(next));

    
    
    info("Waiting for install dialog");
    Services.wm.addListener({
      onWindowTitleChange: function() {},
      onCloseWindow: function() {},
      onOpenWindow: function(xulwindow) {
        Services.wm.removeListener(this);
        var domwindow = xulwindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                              .getInterface(Components.interfaces.nsIDOMWindow);
        waitForFocus(function() {
          info("Saw install dialog");
          is(domwindow.document.location.href, XPINSTALL_URL, "Should have seen the right window open");
          
          var button = domwindow.document.documentElement.getButton("accept");
          button.disabled = false;
          domwindow.document.documentElement.acceptDialog();
        }, domwindow);
      }
    });

    let activationURL = manifest2.origin + "/browser/browser/base/content/test/social/social_activate.html"
    addTab(activationURL, function(tab) {
      let doc = tab.linkedBrowser.contentDocument;
      let installFrom = doc.nodePrincipal.origin;
      Services.prefs.setCharPref("social.whitelist", "");
      is(SocialService.getOriginActivationType(installFrom), "foreign", "testing foriegn install");
      Social.installProvider(doc, manifest2, function(addonManifest) {
        Services.prefs.clearUserPref("social.whitelist");
        SocialService.addBuiltinProvider(addonManifest.origin, function(provider) {
          Social.uninstallProvider(addonManifest.origin);
          gBrowser.removeTab(tab);
        });
      });
    });
  },
  testWhitelistInstall: function(next) {
    AddonManager.addAddonListener(installListener(next));

    let activationURL = manifest2.origin + "/browser/browser/base/content/test/social/social_activate.html"
    addTab(activationURL, function(tab) {
      let doc = tab.linkedBrowser.contentDocument;
      let installFrom = doc.nodePrincipal.origin;
      Services.prefs.setCharPref("social.whitelist", installFrom);
      is(SocialService.getOriginActivationType(installFrom), "whitelist", "testing whitelist install");
      Social.installProvider(doc, manifest2, function(addonManifest) {
        Services.prefs.clearUserPref("social.whitelist");
        SocialService.addBuiltinProvider(addonManifest.origin, function(provider) {
          Social.uninstallProvider(addonManifest.origin);
          gBrowser.removeTab(tab);
        });
      });
    });
  },
  testDirectoryInstall: function(next) {
    AddonManager.addAddonListener(installListener(next));

    let activationURL = manifest2.origin + "/browser/browser/base/content/test/social/social_activate.html"
    addTab(activationURL, function(tab) {
      let doc = tab.linkedBrowser.contentDocument;
      let installFrom = doc.nodePrincipal.origin;
      Services.prefs.setCharPref("social.directories", installFrom);
      is(SocialService.getOriginActivationType(installFrom), "directory", "testing directory install");
      Social.installProvider(doc, manifest2, function(addonManifest) {
        Services.prefs.clearUserPref("social.directories");
        SocialService.addBuiltinProvider(addonManifest.origin, function(provider) {
          Social.uninstallProvider(addonManifest.origin);
          gBrowser.removeTab(tab);
        });
      });
    });
  }
}
