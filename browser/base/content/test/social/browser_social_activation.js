



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let tabsToRemove = [];

function postTestCleanup(callback) {
  Social.provider = null;
  
  for (let tab of tabsToRemove)
    gBrowser.removeTab(tab);
  tabsToRemove = [];
  
  
  SocialUI.activationPanel.hidePopup();

  Services.prefs.clearUserPref("social.whitelist");

  
  for (let manifest of gProviders)
    Services.prefs.clearUserPref("social.manifest." + manifest.origin);
  
  let numRemoved = 0;
  let cbRemoved = function() {
    if (++numRemoved == gProviders.length) {
      executeSoon(function() {
        is(Social.providers.length, 0, "all providers removed");
        callback();
      });
    }
  }
  for (let [i, manifest] of Iterator(gProviders)) {
    try {
      SocialService.removeProvider(manifest.origin, cbRemoved);
    } catch(ex) {
      
      cbRemoved();
    }
  }
}

function addBuiltinManifest(manifest) {
  setManifestPref("social.manifest."+manifest.origin, manifest);
}

function addTab(url, callback) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    tabsToRemove.push(tab);
    executeSoon(function() {callback(tab)});
  }, true);
}

function sendActivationEvent(tab, callback) {
  
  Social.lastEventReceived = 0;
  let doc = tab.linkedBrowser.contentDocument;
  
  if (doc.defaultView.frames[0])
    doc = doc.defaultView.frames[0].document;
  let button = doc.getElementById("activation");
  EventUtils.synthesizeMouseAtCenter(button, {}, doc.defaultView);
  executeSoon(callback);
}

function activateProvider(domain, callback) {
  let activationURL = domain+"/browser/browser/base/content/test/social/social_activate.html"
  addTab(activationURL, function(tab) {
    sendActivationEvent(tab, callback);
  });
}

function activateIFrameProvider(domain, callback) {
  let activationURL = domain+"/browser/browser/base/content/test/social/social_activate_iframe.html"
  addTab(activationURL, function(tab) {
    sendActivationEvent(tab, callback);
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
      ok(SocialUI.activationPanel.hidden, "activation panel still hidden");
      checkSocialUI();
      Services.prefs.clearUserPref("social.remote-install.enabled");
      next();
    });
  },

  testIFrameActivation: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    activateIFrameProvider(gTestDomains[0], function() {
      is(SocialUI.enabled, false, "SocialUI is not enabled");
      ok(!Social.provider, "provider is not installed");
      ok(SocialUI.activationPanel.hidden, "activation panel still hidden");
      checkSocialUI();
      Services.prefs.clearUserPref("social.whitelist");
      next();
    });
  },

  testActivationFirstProvider: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    
    activateProvider(gTestDomains[0], function() {
      ok(!SocialUI.activationPanel.hidden, "activation panel should be showing");
      is(Social.provider.origin, gTestDomains[0], "new provider is active");
      checkSocialUI();
      
      document.getElementById("social-undoactivation-button").click();
      executeSoon(function() {
        
        ok(!Social.provider, "should be no provider left after disabling");
        checkSocialUI();
        Services.prefs.clearUserPref("social.whitelist");
        next();
      })
    });
  },

  testActivationMultipleProvider: function(next) {
    
    
    
    
    
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    SocialService.addProvider(gProviders[0], function() {
      SocialService.addProvider(gProviders[1], function() {
        Social.provider = Social.providers[1];
        checkSocialUI();
        
        addBuiltinManifest(gProviders[2]);
        activateProvider(gTestDomains[2], function() {
          ok(!SocialUI.activationPanel.hidden, "activation panel should be showing");
          is(Social.provider.origin, gTestDomains[2], "new provider is active");
          checkSocialUI();
          
          document.getElementById("social-undoactivation-button").click();
          executeSoon(function() {
            
            is(Social.provider.origin, Social.providers[1].origin, "original provider should have been reactivated");
            checkSocialUI();
            Services.prefs.clearUserPref("social.whitelist");
            next();
          });
        });
      });
    });
  },

  testRemoveNonCurrentProvider: function(next) {
    Services.prefs.setCharPref("social.whitelist", gTestDomains.join(","));
    SocialService.addProvider(gProviders[0], function() {
      SocialService.addProvider(gProviders[1], function() {
        Social.provider = Social.providers[1];
        checkSocialUI();
        
        addBuiltinManifest(gProviders[2]);
        activateProvider(gTestDomains[2], function() {
          ok(!SocialUI.activationPanel.hidden, "activation panel should be showing");
          is(Social.provider.origin, gTestDomains[2], "new provider is active");
          checkSocialUI();
          
          
          Social.provider = Social.providers[1];
          
          document.getElementById("social-undoactivation-button").click();
          executeSoon(function() {
            
            is(Social.provider.origin, Social.providers[1].origin, "original provider still be active");
            checkSocialUI();
            Services.prefs.clearUserPref("social.whitelist");
            next();
          });
        });
      });
    });
  },
}
