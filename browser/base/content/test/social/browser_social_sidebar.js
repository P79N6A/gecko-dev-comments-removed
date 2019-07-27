



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
};

function test() {
  waitForExplicitFinish();

  SocialService.addProvider(manifest, function() {
    
    doTest();
  });
}

function doTest() {
  ok(SocialSidebar.canShow, "social sidebar should be able to be shown");
  ok(!SocialSidebar.opened, "social sidebar should not be open by default");
  SocialSidebar.show();

  let command = document.getElementById("Social:ToggleSidebar");
  let sidebar = document.getElementById("social-sidebar-box");
  let browser = sidebar.lastChild;

  function checkShown(shouldBeShown) {
    is(command.getAttribute("checked"), shouldBeShown ? "true" : "false",
       "toggle command should be " + (shouldBeShown ? "checked" : "unchecked"));
    is(sidebar.hidden, !shouldBeShown,
       "sidebar should be " + (shouldBeShown ? "visible" : "hidden"));
    if (shouldBeShown) {
      is(browser.getAttribute('src'), SocialSidebar.provider.sidebarURL, "sidebar url should be set");
      
      
      
    }
    else {
      ok(!browser.docShellIsActive, "sidebar should have an inactive docshell");
      
      
      if (SocialSidebar.canShow) {
        
        is(browser.getAttribute('src'), SocialSidebar.provider.sidebarURL, "sidebar url should be set");
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

      
      SocialService.disableProvider(SocialSidebar.provider.origin, function() {
        checkShown(false);
        is(Social.providers.length, 0, "no providers left");
        defaultFinishChecks();
        
        executeSoon(finish);
      });
    });

    
    info("Toggling sidebar back on");
    SocialSidebar.toggleSidebar();
  });

  
  
  let port = SocialSidebar.provider.getWorkerPort();
  port.postMessage({topic: "test-init"});
  port.onmessage = function (e) {
    let topic = e.data.topic;
    switch (topic) {
      case "got-sidebar-message":
        ok(true, "sidebar is loaded and ready");
        SocialSidebar.toggleSidebar();
    }
  };
}
