



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

function test() {
  
  if (Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2).isDebugBuild) {
    ok(true, "can't run social sidebar test in debug builds because they falsely report leaks");
    return;
  }

  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  };
  runSocialTestWithProvider(manifest, doTest);
}

function doTest() {
  ok(SocialSidebar.canShow, "social sidebar should be able to be shown");
  ok(SocialSidebar.enabled, "social sidebar should be on by default");

  let command = document.getElementById("Social:ToggleSidebar");
  let sidebar = document.getElementById("social-sidebar-box");
  let browser = sidebar.firstChild;

  function checkShown(shouldBeShown) {
    is(command.getAttribute("checked"), shouldBeShown ? "true" : "false",
       "toggle command should be " + (shouldBeShown ? "checked" : "unchecked"));
    is(sidebar.hidden, !shouldBeShown,
       "sidebar should be " + (shouldBeShown ? "visible" : "hidden"));
    is(Services.prefs.getBoolPref("social.sidebar.open"), shouldBeShown,
       "sidebar open pref should be " + shouldBeShown);
    if (shouldBeShown)
      is(browser.getAttribute('src'), Social.provider.sidebarURL, "sidebar url should be set");
  }

  
  ok(!command.hidden, "toggle command should be visible");
  checkShown(true);

  browser.addEventListener("sidebarhide", function sidebarhide() {
    browser.removeEventListener("sidebarhide", sidebarhide);

    checkShown(false);

    browser.addEventListener("sidebarshow", function sidebarshow() {
      browser.removeEventListener("sidebarshow", sidebarshow);

      checkShown(true);

      
      SocialService.removeProvider(Social.provider.origin, finish);
    });

    
    info("Toggling sidebar back on");
    Social.toggleSidebar();
  });

  
  info("Toggling sidebar off");
  Social.toggleSidebar();
}


