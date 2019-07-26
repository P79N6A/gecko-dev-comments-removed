



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let tabsToRemove = [];

function postTestCleanup(callback) {
  
  for (let tab of tabsToRemove)
    gBrowser.removeTab(tab);
  tabsToRemove = [];
  
  
  

  Services.prefs.clearUserPref("social.whitelist");

  
  for (let manifest of gProviders)
    Services.prefs.clearUserPref("social.manifest." + manifest.origin);

  
  let providers = gProviders.slice(0)
  function removeProviders() {
    if (providers.length < 1) {
      executeSoon(function() {
        is(Social.providers.length, 0, "all providers removed");
        callback();
      });
      return;
    }

    let provider = providers.pop();
    try {
      SocialService.removeProvider(provider.origin, removeProviders);
    } catch(ex) {
      removeProviders();
    }
  }
  removeProviders();
}

function addBuiltinManifest(manifest) {
  let prefname = getManifestPrefname(manifest);
  setBuiltinManifestPref(prefname, manifest);
  return prefname;
}

function addTab(url, callback) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    tabsToRemove.push(tab);
    executeSoon(function() {callback(tab)});
  }, true);
}

function sendActivationEvent(tab, callback, nullManifest) {
  
  Social.lastEventReceived = 0;
  let doc = tab.linkedBrowser.contentDocument;
  
  if (doc.defaultView.frames[0])
    doc = doc.defaultView.frames[0].document;
  let button = doc.getElementById(nullManifest ? "activation-old" : "activation");
  EventUtils.synthesizeMouseAtCenter(button, {}, doc.defaultView);
  executeSoon(callback);
}

function activateProvider(domain, callback, nullManifest) {
  let activationURL = domain+"/browser/browser/base/content/test/social/social_activate.html"
  addTab(activationURL, function(tab) {
    sendActivationEvent(tab, callback, nullManifest);
  });
}

function activateIFrameProvider(domain, callback) {
  let activationURL = domain+"/browser/browser/base/content/test/social/social_activate_iframe.html"
  addTab(activationURL, function(tab) {
    sendActivationEvent(tab, callback, false);
  });
}

function waitForProviderLoad(cb) {
  Services.obs.addObserver(function providerSet(subject, topic, data) {
    Services.obs.removeObserver(providerSet, "social:provider-enabled");
    info("social:provider-enabled observer was notified");
    waitForCondition(function() {
      let sbrowser = document.getElementById("social-sidebar-browser");
      let provider = SocialSidebar.provider;
      let postActivation = provider && gBrowser.contentDocument.location.href == provider.origin + "/browser/browser/base/content/test/social/social_postActivation.html";

      return provider &&
             provider.profile &&
             provider.profile.displayName &&
             postActivation &&
             sbrowser.docShellIsActive;
    }, function() {
      
      executeSoon(cb);
    },
    "waitForProviderLoad: provider profile was not set");
  }, "social:provider-enabled", false);
}


function getAddonItemInList(aId, aList) {
  var item = aList.firstChild;
  while (item) {
    if ("mAddon" in item && item.mAddon.id == aId) {
      aList.ensureElementIsVisible(item);
      return item;
    }
    item = item.nextSibling;
  }
  return null;
}

function clickAddonRemoveButton(tab, aCallback) {
  AddonManager.getAddonsByTypes(["service"], function(aAddons) {
    let addon = aAddons[0];

    let doc = tab.linkedBrowser.contentDocument;
    let list = doc.getElementById("addon-list");

    let item = getAddonItemInList(addon.id, list);
    isnot(item, null, "Should have found the add-on in the list");

    var button = doc.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
    isnot(button, null, "Should have a remove button");
    ok(!button.disabled, "Button should not be disabled");

    EventUtils.synthesizeMouseAtCenter(button, { }, doc.defaultView);

    
    item.clientTop;

    is(item.getAttribute("pending"), "uninstall", "Add-on should be uninstalling");

    executeSoon(function() { aCallback(addon); });
  });
}

function activateOneProvider(manifest, finishActivation, aCallback) {
  let panel = document.getElementById("servicesInstall-notification");
  PopupNotifications.panel.addEventListener("popupshown", function onpopupshown() {
    PopupNotifications.panel.removeEventListener("popupshown", onpopupshown);
    info("servicesInstall-notification panel opened");
    if (finishActivation)
      panel.button.click();
    else
      panel.closebutton.click();
  });

  activateProvider(manifest.origin, function() {
    if (!finishActivation) {
      ok(panel.hidden, "activation panel is not showing");
      executeSoon(aCallback);
    } else {
      waitForProviderLoad(function() {
        is(SocialSidebar.provider.origin, manifest.origin, "new provider is active");
        ok(SocialSidebar.opened, "sidebar is open");
        checkSocialUI();
        executeSoon(aCallback);
      });
    }
  });
}

let gTestDomains = ["https://example.com", "https://test1.example.com", "https://test2.example.com"];
let gProviders = [
  {
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html?provider1",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js#no-profile,no-recommend",
    iconURL: "chrome://branding/content/icon48.png"
  },
  {
    name: "provider 2",
    origin: "https://test1.example.com",
    sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html?provider2",
    workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js#no-profile,no-recommend",
    iconURL: "chrome://branding/content/icon64.png"
  },
  {
    name: "provider 3",
    origin: "https://test2.example.com",
    sidebarURL: "https://test2.example.com/browser/browser/base/content/test/social/social_sidebar.html?provider2",
    workerURL: "https://test2.example.com/browser/browser/base/content/test/social/social_worker.js#no-profile,no-recommend",
    iconURL: "chrome://branding/content/about-logo.png"
  }
];


function test() {
  waitForExplicitFinish();
  runSocialTests(tests, undefined, postTestCleanup);
}

var tests = {
  testActivationWrongOrigin: function(next) {
    
    Services.prefs.setBoolPref("social.remote-install.enabled", false);
    activateProvider(gTestDomains[0], function() {
      is(SocialUI.enabled, false, "SocialUI is not enabled");
      let panel = document.getElementById("servicesInstall-notification");
      ok(panel.hidden, "activation panel still hidden");
      checkSocialUI();
      Services.prefs.clearUserPref("social.remote-install.enabled");
      next();
    });
  },

  testIFrameActivation: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    activateIFrameProvider(gTestDomains[0], function() {
      is(SocialUI.enabled, false, "SocialUI is not enabled");
      ok(!SocialSidebar.provider, "provider is not installed");
      let panel = document.getElementById("servicesInstall-notification");
      ok(panel.hidden, "activation panel still hidden");
      checkSocialUI();
      Services.prefs.clearUserPref("social.whitelist");
      next();
    });
  },

  testActivationFirstProvider: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    
    activateOneProvider(gProviders[0], false, function() {
      
      ok(!SocialSidebar.provider, "should be no provider left after disabling");
      checkSocialUI();
      Services.prefs.clearUserPref("social.whitelist");
      next();
    });
  },

  testActivationBuiltin: function(next) {
    let prefname = addBuiltinManifest(gProviders[0]);
    is(SocialService.getOriginActivationType(gTestDomains[0]), "builtin", "manifest is builtin");
    
    activateOneProvider(gProviders[0], false, function() {
      
      ok(!SocialSidebar.provider, "should be no provider left after disabling");
      checkSocialUI();
      resetBuiltinManifestPref(prefname);
      next();
    });
  },

  testActivationMultipleProvider: function(next) {
    
    
    
    
    
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    SocialService.addProvider(gProviders[0], function() {
      SocialService.addProvider(gProviders[1], function() {
        checkSocialUI();
        
        let prefname = addBuiltinManifest(gProviders[2]);
        activateOneProvider(gProviders[2], false, function() {
          
          is(SocialSidebar.provider.origin, Social.providers[1].origin, "original provider should have been reactivated");
          checkSocialUI();
          Services.prefs.clearUserPref("social.whitelist");
          resetBuiltinManifestPref(prefname);
          next();
        });
      });
    });
  },

  testAddonManagerDoubleInstall: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    
    let blanktab = gBrowser.addTab();
    gBrowser.selectedTab = blanktab;
    BrowserOpenAddonsMgr('addons://list/service');

    is(blanktab, gBrowser.selectedTab, "Current tab should be blank tab");

    gBrowser.selectedBrowser.addEventListener("load", function tabLoad() {
      gBrowser.selectedBrowser.removeEventListener("load", tabLoad, true);
      let browser = blanktab.linkedBrowser;
      is(browser.currentURI.spec, "about:addons", "about:addons should load into blank tab.");

      let prefname = addBuiltinManifest(gProviders[0]);
      activateOneProvider(gProviders[0], true, function() {
        info("first activation completed");
        is(gBrowser.contentDocument.location.href, gProviders[0].origin + "/browser/browser/base/content/test/social/social_postActivation.html");
        gBrowser.removeTab(gBrowser.selectedTab);
        is(gBrowser.contentDocument.location.href, gProviders[0].origin + "/browser/browser/base/content/test/social/social_activate.html");
        gBrowser.removeTab(gBrowser.selectedTab);
        tabsToRemove.pop();
        
        clickAddonRemoveButton(blanktab, function(addon) {
          checkSocialUI();
          activateOneProvider(gProviders[0], true, function() {
            info("second activation completed");
            is(gBrowser.contentDocument.location.href, gProviders[0].origin + "/browser/browser/base/content/test/social/social_postActivation.html");
            gBrowser.removeTab(gBrowser.selectedTab);

            
            gBrowser.tabContainer.addEventListener("TabClose", function onTabClose() {
              gBrowser.tabContainer.removeEventListener("TabClose", onTabClose);
              AddonManager.getAddonsByTypes(["service"], function(aAddons) {
                is(aAddons.length, 1, "there can be only one");
                Services.prefs.clearUserPref("social.whitelist");
                resetBuiltinManifestPref(prefname);
                next();
              });
            });

            
            AddonManager.getAddonsByTypes(["service"], function(aAddons) {
              is(aAddons.length, 1, "there can be only one");

              let doc = blanktab.linkedBrowser.contentDocument;
              let list = doc.getElementById("addon-list");
              is(list.childNodes.length, 1, "only one addon is displayed");

              gBrowser.removeTab(blanktab);
            });

          });
        });
      });
    }, true);
  }
}
