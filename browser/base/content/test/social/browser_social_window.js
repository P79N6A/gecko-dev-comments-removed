





let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;




function resetSocial() {
  Social.initialized = false;
  Social.providers = [];
  
  SocialService._providerListeners.clear();
}

let createdWindows = [];

function openWindowAndWaitForInit(parentWin, callback) {
  
  let topic = "browser-delayed-startup-finished";
  let w = parentWin.OpenBrowserWindow();
  createdWindows.push(w);
  Services.obs.addObserver(function providerSet(subject, topic, data) {
    Services.obs.removeObserver(providerSet, topic);
    info(topic + " observer was notified - continuing test");
    executeSoon(() => callback(w));
  }, topic, false);
}

function closeWindow(w, cb) {
  waitForNotification("domwindowclosed", cb);
  w.close();
}

function closeOneWindow(cb) {
  let w = createdWindows.pop();
  if (!w || w.closed) {
    cb();
    return;
  }
  closeWindow(w, function() {
    closeOneWindow(cb);
  });
  w.close();
}

function postTestCleanup(cb) {
  closeOneWindow(cb);
}

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
};
let manifest2 = { 
  name: "provider test1",
  origin: "https://test1.example.com",
  workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
  sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html",
  iconURL: "https://test1.example.com/browser/browser/base/content/test/general/moz.png",
};

function test() {
  waitForExplicitFinish();
  runSocialTests(tests, undefined, postTestCleanup);
}

let tests = {
  
  testInactiveStartup: function(cbnext) {
    is(Social.providers.length, 0, "needs zero providers to start this test.");
    ok(!SocialService.hasEnabledProviders, "no providers are enabled");
    resetSocial();
    openWindowAndWaitForInit(window, function(w1) {
      checkSocialUI(w1);
      
      openWindowAndWaitForInit(window, function(w2) {
        checkSocialUI(w2);
        checkSocialUI(w1);
        cbnext();
      });
    });
  },

  
  testEnabledStartup: function(cbnext) {
    setManifestPref("social.manifest.test", manifest);
    ok(!SocialSidebar.opened, "sidebar is closed initially");
    SocialService.addProvider(manifest, function() {
      SocialService.addProvider(manifest2, function (provider) {
        SocialSidebar.show();
        waitForCondition(function() SocialSidebar.opened,
                     function() {
          ok(SocialSidebar.opened, "first window sidebar is open");
          openWindowAndWaitForInit(window, function(w1) {
            ok(w1.SocialSidebar.opened, "new window sidebar is open");
            ok(SocialService.hasEnabledProviders, "providers are enabled");
            checkSocialUI(w1);
            
            openWindowAndWaitForInit(window, function(w2) {
              ok(w1.SocialSidebar.opened, "w1 sidebar is open");
              ok(w2.SocialSidebar.opened, "w2 sidebar is open");
              checkSocialUI(w2);
              checkSocialUI(w1);

              
              SocialService.disableProvider(manifest.origin, function() {
                SocialService.disableProvider(manifest2.origin, function() {
                  ok(!Social.enabled, "social is disabled");
                  is(Social.providers.length, 0, "no providers");
                  ok(!w1.SocialSidebar.opened, "w1 sidebar is closed");
                  ok(!w2.SocialSidebar.opened, "w2 sidebar is closed");
                  checkSocialUI(w2);
                  checkSocialUI(w1);
                  Services.prefs.clearUserPref("social.manifest.test");
                  cbnext();
                });
              });
            });
          });
        }, "sidebar did not open");
      }, cbnext);
    }, cbnext);
  },

  testGlobalState: function(cbnext) {
    setManifestPref("social.manifest.test", manifest);
    ok(!SocialSidebar.opened, "sidebar is closed initially");
    ok(!Services.prefs.prefHasUserValue("social.sidebar.provider"), "global state unset");
    
    SessionStore.deleteWindowValue(window, "socialSidebar");
    ok(!SessionStore.getWindowValue(window, "socialSidebar"), "window state unset");
    SocialService.addProvider(manifest, function() {
      openWindowAndWaitForInit(window, function(w1) {
        w1.SocialSidebar.show();
        waitForCondition(function() w1.SocialSidebar.opened,
                     function() {
          ok(Services.prefs.prefHasUserValue("social.sidebar.provider"), "global state set");
          ok(!SocialSidebar.opened, "1. main sidebar is still closed");
          ok(w1.SocialSidebar.opened, "1. window sidebar is open");
          closeWindow(w1, function() {
            
            
            openWindowAndWaitForInit(window, function(w1) {
              ok(!SocialSidebar.opened, "2. main sidebar is still closed");
              ok(w1.SocialSidebar.opened, "2. window sidebar is open");
              w1.SocialSidebar.hide();
              ok(!w1.SocialSidebar.opened, "2. window sidebar is closed");
              ok(!Services.prefs.prefHasUserValue("social.sidebar.provider"), "2. global state unset");
              
              closeWindow(w1, function() {
                ok(!Services.prefs.prefHasUserValue("social.sidebar.provider"), "3. global state unset");
                ok(!SocialSidebar.opened, "3. main sidebar is still closed");
                openWindowAndWaitForInit(window, function(w1) {
                  ok(!Services.prefs.prefHasUserValue("social.sidebar.provider"), "4. global state unset");
                  ok(!SocialSidebar.opened, "4. main sidebar is still closed");
                  ok(!w1.SocialSidebar.opened, "4. window sidebar is closed");
                  SocialService.disableProvider(manifest.origin, function() {
                    Services.prefs.clearUserPref("social.manifest.test");
                    cbnext();
                  });
                });
              });
            });
          });
        });        
      });
    });
  },

  
  
  
  testPerWindowSidebar: function(cbnext) {
    function finishCheck() {
      
      SocialService.disableProvider(manifest.origin, function() {
        SocialService.disableProvider(manifest2.origin, function() {
          ok(!Social.enabled, "social is disabled");
          is(Social.providers.length, 0, "no providers");
          Services.prefs.clearUserPref("social.manifest.test");
          cbnext();
        });
      });
    }

    setManifestPref("social.manifest.test", manifest);
    ok(!SocialSidebar.opened, "sidebar is closed initially");
    SocialService.addProvider(manifest, function() {
      SocialService.addProvider(manifest2, function (provider) {
        
        
        
        
        
        Services.prefs.setCharPref("social.provider.current", "https://example.com");
        Services.prefs.setBoolPref("social.sidebar.open", true);

        openWindowAndWaitForInit(window, function(w1) {
          ok(w1.SocialSidebar.opened, "new window sidebar is open");
          ok(SocialService.hasEnabledProviders, "providers are enabled");
          ok(!Services.prefs.prefHasUserValue("social.provider.current"), "social.provider.current pref removed");
          ok(!Services.prefs.prefHasUserValue("social.sidebar.open"), "social.sidebar.open pref removed");
          checkSocialUI(w1);
          
          openWindowAndWaitForInit(w1, function(w2) {
            ok(w1.SocialSidebar.opened, "w1 sidebar is open");
            ok(w2.SocialSidebar.opened, "w2 sidebar is open");
            checkSocialUI(w2);
            checkSocialUI(w1);

            
            w2.SocialSidebar.show(manifest2.origin);
            let sbrowser1 = w1.document.getElementById("social-sidebar-browser");
            is(manifest.origin, sbrowser1.getAttribute("origin"), "w1 sidebar origin matches");
            let sbrowser2 = w2.document.getElementById("social-sidebar-browser");
            is(manifest2.origin, sbrowser2.getAttribute("origin"), "w2 sidebar origin matches");

            
            w2.SocialSidebar.hide();
            ok(w1.SocialSidebar.opened, "w1 sidebar is opened");
            ok(!w2.SocialSidebar.opened, "w2 sidebar is closed");
            ok(sbrowser2.parentNode.hidden, "w2 sidebar is hidden");

            
            openWindowAndWaitForInit(w2, function(w3) {
              
              
              w3.SocialSidebar.ensureProvider();
              is(w3.SocialSidebar.provider, w2.SocialSidebar.provider, "w3 has same provider as w2");
              ok(!w3.SocialSidebar.opened, "w2 sidebar is closed");

              
              openWindowAndWaitForInit(w1, function(w4) {
                is(w4.SocialSidebar.provider, w1.SocialSidebar.provider, "w4 has same provider as w1");
                ok(w4.SocialSidebar.opened, "w4 sidebar is opened");

                finishCheck();
              });
            });

          });
        });
      }, cbnext);
    }, cbnext);
  }
}
