



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

function test() {
  waitForExplicitFinish();

  let oldProvider;
  function saveOldProviderAndStartTestWith(provider) {
    oldProvider = Social.provider;
    registerCleanupFunction(function () {
      Social.provider = oldProvider;
    });
    Social.provider = provider;

    
    Services.prefs.setBoolPref("social.enabled", true);
    registerCleanupFunction(function () {
      Services.prefs.clearUserPref("social.enabled");
    });

    doTest();
  }

  let manifest = { 
    name: "provider 1",
    origin: "https://example1.com",
    sidebarURL: "https://example1.com/sidebar.html",
    workerURL: "https://example1.com/worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  };
  SocialService.addProvider(manifest, function(provider) {
    
    
    if (Social.provider) {
      saveOldProviderAndStartTestWith(provider);
    } else {
      Services.obs.addObserver(function obs() {
        Services.obs.removeObserver(obs, "test-social-ui-ready");
        saveOldProviderAndStartTestWith(provider);
      }, "test-social-ui-ready", false);
    }
  });
}

function doTest() {
  ok(SocialSidebar.canShow, "social sidebar should be able to be shown");
  ok(SocialSidebar.enabled, "social sidebar should be on by default");

  let command = document.getElementById("Social:ToggleSidebar");
  let sidebar = document.getElementById("social-sidebar-box");

  
  ok(!command.hidden, "sidebar toggle command should be visible");
  is(command.getAttribute("checked"), "true", "sidebar toggle command should be checked");
  ok(!sidebar.hidden, "sidebar itself should be visible");
  ok(Services.prefs.getBoolPref("social.sidebar.open"), "sidebar open pref should be true");
  is(sidebar.firstChild.getAttribute('src'), "https://example1.com/sidebar.html", "sidebar url should be set");

  
  info("Toggling sidebar");
  Social.toggleSidebar();
  is(command.getAttribute("checked"), "false", "sidebar toggle command should not be checked");
  ok(sidebar.hidden, "sidebar itself should not be visible");
  ok(!Services.prefs.getBoolPref("social.sidebar.open"), "sidebar open pref should be false");
  is(sidebar.firstChild.getAttribute('src'), "about:blank", "sidebar url should not be set");

  
  SocialService.removeProvider(Social.provider.origin, finish);
}


