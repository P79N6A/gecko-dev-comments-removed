

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

function test() {
  waitForExplicitFinish();

  let prefname = getManifestPrefname(manifest);
  setBuiltinManifestPref(prefname, manifest);
  
  is(SocialService.getOriginActivationType(manifest.origin), "builtin", "manifest is builtin");
  is(SocialService.getOriginActivationType(manifest2.origin), "foreign", "manifest2 is not builtin");

  Services.prefs.setBoolPref("social.remote-install.enabled", true);
  runSocialTests(tests, undefined, undefined, function () {
    Services.prefs.clearUserPref("social.remote-install.enabled");
    
    ok(!Services.prefs.prefHasUserValue(prefname), "manifest is not in user-prefs");
    Services.prefs.getDefaultBranch(null).deleteBranch(prefname);
    is(Services.prefs.getDefaultBranch(null).getPrefType(prefname),
       Services.prefs.PREF_INVALID, "default pref removed");
    
    Services.prefs.clearUserPref("social.whitelist");
    Services.prefs.clearUserPref("social.directories");
    finish();
  });
}

function installListener(next, aManifest) {
  let expectEvent = "onInstalling";
  let prefname = getManifestPrefname(aManifest);
  return {
    onInstalling: function(addon) {
      is(expectEvent, "onInstalling", "install started");
      is(addon.manifest.origin, aManifest.origin, "provider about to be installed");
      ok(!Services.prefs.prefHasUserValue(prefname), "manifest is not in user-prefs");
      expectEvent = "onInstalled";
    },
    onInstalled: function(addon) {
      is(addon.manifest.origin, aManifest.origin, "provider installed");
      ok(Services.prefs.prefHasUserValue(prefname), "manifest is in user-prefs");
      expectEvent = "onUninstalling";
    },
    onUninstalling: function(addon) {
      is(expectEvent, "onUninstalling", "uninstall started");
      is(addon.manifest.origin, aManifest.origin, "provider about to be uninstalled");
      ok(Services.prefs.prefHasUserValue(prefname), "manifest is in user-prefs");
      expectEvent = "onUninstalled";
    },
    onUninstalled: function(addon) {
      is(expectEvent, "onUninstalled", "provider has been uninstalled");
      is(addon.manifest.origin, aManifest.origin, "provider uninstalled");
      ok(!Services.prefs.prefHasUserValue(prefname), "manifest is not in user-prefs");
      AddonManager.removeAddonListener(this);
      next();
    }
  };
}

var tests = {
  testAddonEnableToggle: function(next) {
    let expectEvent;
    let prefname = getManifestPrefname(manifest);
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
          
          Services.prefs.clearUserPref(prefname);
          next();
        });
      },
      onDisabling: function(addon) {
        is(expectEvent, "onDisabling", "provider onDisabling");
        expectEvent = "onDisabled";
      }
    };
    AddonManager.addAddonListener(listener);

    
    
    setManifestPref(prefname, manifest);
    ok(Services.prefs.prefHasUserValue(prefname), "manifest is in user-prefs");
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
    let prefname = getManifestPrefname(manifest);

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
    setManifestPref(prefname, manifest);
    SocialService.addBuiltinProvider(manifest.origin, function(provider) {
      expectEvent = "onDisabling";
      SocialService.removeProvider(provider.origin, function() {
        AddonManager.removeAddonListener(listener);
        Services.prefs.clearUserPref(prefname);
        next();
      });
    });
  },
  testForeignInstall: function(next) {
    AddonManager.addAddonListener(installListener(next, manifest2));

    
    
    info("Waiting for install dialog");
    let panel = document.getElementById("servicesInstall-notification");
    PopupNotifications.panel.addEventListener("popupshown", function onpopupshown() {
      PopupNotifications.panel.removeEventListener("popupshown", onpopupshown);
      info("servicesInstall-notification panel opened");
      panel.button.click();
    })

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
  testBuiltinInstall: function(next) {
    AddonManager.addAddonListener(installListener(next, manifest));

    let prefname = getManifestPrefname(manifest);
    let activationURL = manifest.origin + "/browser/browser/base/content/test/social/social_activate.html"
    addTab(activationURL, function(tab) {
      let doc = tab.linkedBrowser.contentDocument;
      let installFrom = doc.nodePrincipal.origin;
      is(SocialService.getOriginActivationType(installFrom), "builtin", "testing builtin install");
      ok(!Services.prefs.prefHasUserValue(prefname), "manifest is not in user-prefs");
      Social.installProvider(doc, manifest, function(addonManifest) {
        ok(Services.prefs.prefHasUserValue(prefname), "manifest is in user-prefs");
        SocialService.addBuiltinProvider(addonManifest.origin, function(provider) {
          Social.uninstallProvider(addonManifest.origin);
          gBrowser.removeTab(tab);
        });
      });
    });
  },
  testWhitelistInstall: function(next) {
    AddonManager.addAddonListener(installListener(next, manifest2));

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
    AddonManager.addAddonListener(installListener(next, manifest2));

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
