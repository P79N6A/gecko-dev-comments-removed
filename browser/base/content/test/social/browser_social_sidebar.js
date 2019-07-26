



function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
  };
  runSocialTestWithProvider(manifest, doTest);
}

function doTest(finishcb) {
  ok(SocialSidebar.canShow, "social sidebar should be able to be shown");
  ok(SocialSidebar.opened, "social sidebar should be open by default");

  let command = document.getElementById("Social:ToggleSidebar");
  let sidebar = document.getElementById("social-sidebar-box");
  let browser = sidebar.firstChild;

  function checkShown(shouldBeShown) {
    is(command.getAttribute("checked"), shouldBeShown ? "true" : "false",
       "toggle command should be " + (shouldBeShown ? "checked" : "unchecked"));
    is(sidebar.hidden, !shouldBeShown,
       "sidebar should be " + (shouldBeShown ? "visible" : "hidden"));
    
    
    if (Social.enabled)
      is(Services.prefs.getBoolPref("social.sidebar.open"), shouldBeShown,
         "sidebar open pref should be " + shouldBeShown);
    if (shouldBeShown) {
      is(browser.getAttribute('src'), Social.provider.sidebarURL, "sidebar url should be set");
      
      
      
    }
    else {
      ok(!browser.docShellIsActive, "sidebar should have an inactive docshell");
      
      
      if (SocialSidebar.canShow) {
        
        is(browser.getAttribute('src'), Social.provider.sidebarURL, "sidebar url should be set");
      } else {
        
        is(browser.getAttribute('src'), "about:blank", "sidebar url should be blank");
      }
    }
  }

  
  ok(!command.hidden, "toggle command should be visible");
  checkShown(true);

  browser.addEventListener("socialFrameHide", function sidebarhide() {
    browser.removeEventListener("socialFrameHide", sidebarhide);

    checkShown(false);

    browser.addEventListener("socialFrameShow", function sidebarshow() {
      browser.removeEventListener("socialFrameShow", sidebarshow);

      checkShown(true);

      
      Social.enabled = false;
      checkShown(false);

      Social.enabled = true;
      checkShown(true);

      
      Social.provider = null;
      Social.enabled = false;
      checkShown(false);

      
      finishcb();
    });

    
    info("Toggling sidebar back on");
    Social.toggleSidebar();
  });

  
  
  let port = Social.provider.getWorkerPort();
  port.postMessage({topic: "test-init"});
  port.onmessage = function (e) {
    let topic = e.data.topic;
    switch (topic) {
      case "got-sidebar-message":
        ok(true, "sidebar is loaded and ready");
        Social.toggleSidebar();
    }
  };
}


