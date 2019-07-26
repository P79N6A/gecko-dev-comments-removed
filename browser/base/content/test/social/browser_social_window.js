








function resetSocial() {
  Social.initialized = false;
  Social._provider = null;
  Social.providers = [];
  
  let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;
  SocialService._providerListeners.clear();
}

let createdWindows = [];

function openWindowAndWaitForInit(callback) {
  
  let topic = "browser-delayed-startup-finished";
  let w = OpenBrowserWindow();
  createdWindows.push(w);
  Services.obs.addObserver(function providerSet(subject, topic, data) {
    Services.obs.removeObserver(providerSet, topic);
    info(topic + " observer was notified - continuing test");
    
    executeSoon(function() {callback(w)});
  }, topic, false);
}

function postTestCleanup(cb) {
  for (let w of createdWindows)
    w.close();
  createdWindows = [];
  Services.prefs.clearUserPref("social.enabled");
  cb();
}

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/social/moz.png"
};

function test() {
  waitForExplicitFinish();
  runSocialTests(tests, undefined, postTestCleanup);
}

let tests = {
  
  testInactiveStartup: function(cbnext) {
    is(Social.providers.length, 0, "needs zero providers to start this test.");
    resetSocial();
    openWindowAndWaitForInit(function(w1) {
      checkSocialUI(w1);
      
      openWindowAndWaitForInit(function(w2) {
        checkSocialUI(w2);
        checkSocialUI(w1);
        cbnext();
      });
    });
  },

  
  testEnabledStartup: function(cbnext) {
    runSocialTestWithProvider(manifest, function (finishcb) {
      resetSocial();
      openWindowAndWaitForInit(function(w1) {
        ok(Social.enabled, "social is enabled");
        checkSocialUI(w1);
        
        openWindowAndWaitForInit(function(w2) {
          checkSocialUI(w2);
          checkSocialUI(w1);
          
          Services.prefs.setBoolPref("social.enabled", false);
          executeSoon(function() { 
            ok(!Social.enabled, "social is disabled");
            checkSocialUI(w2);
            checkSocialUI(w1);
            finishcb();
          });
        });
      });
    }, cbnext);
  },

  
  testDisabledStartup: function(cbnext) {
    runSocialTestWithProvider(manifest, function (finishcb) {
      Services.prefs.setBoolPref("social.enabled", false);
      resetSocial();
      openWindowAndWaitForInit(function(w1) {
        ok(!Social.enabled, "social is disabled");
        checkSocialUI(w1);
        
        openWindowAndWaitForInit(function(w2) {
          checkSocialUI(w2);
          checkSocialUI(w1);
          
          Services.prefs.setBoolPref("social.enabled", true);
          executeSoon(function() { 
            ok(Social.enabled, "social is enabled");
            checkSocialUI(w2);
            checkSocialUI(w1);
            finishcb();
          });
        });
      });
    }, cbnext);
  },

  
  testRemoveProvider: function(cbnext) {
    runSocialTestWithProvider(manifest, function (finishcb) {
      openWindowAndWaitForInit(function(w1) {
        checkSocialUI(w1);
        
        openWindowAndWaitForInit(function(w2) {
          checkSocialUI(w2);
          
          let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;
          SocialService.removeProvider(manifest.origin, function() {
            ok(!Social.enabled, "social is disabled");
            is(Social.providers.length, 0, "no providers");
            checkSocialUI(w2);
            checkSocialUI(w1);
            
            
            SocialService.addProvider(manifest, function() {
              finishcb();
            });
          });
        });
      });
    }, cbnext);
  },
}
