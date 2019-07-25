



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

function test() {
  
  if (Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2).isDebugBuild) {
    ok(true, "can't run social sidebar test in debug builds because they falsely report leaks");
    return;
  }

  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "http://example.com",
    sidebarURL: "http://example.com/browser/browser/base/content/test/social_sidebar.html",
    workerURL: "http://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  };
  runSocialTestWithProvider(manifest, doTest);
}

function doTest() {
  let iconsReady = false;
  let gotSidebarMessage = false;

  function checkNext() {
    if (iconsReady && gotSidebarMessage)
      triggerIconPanel();
  }

  function triggerIconPanel() {
    let statusIcons = document.getElementById("social-status-iconbox");
    ok(!statusIcons.firstChild.collapsed, "status icon is visible");
    
    let panel = document.getElementById("social-notification-panel");
    EventUtils.synthesizeMouseAtCenter(statusIcons.firstChild, {});
  }

  let port = Social.provider.port;
  ok(port, "provider has a port");
  port.postMessage({topic: "test-init"});
  Social.provider.port.onmessage = function (e) {
    let topic = e.data.topic;
    switch (topic) {
      case "got-panel-message":
        ok(true, "got panel message");
        
        let panel = document.getElementById("social-notification-panel");
        panel.addEventListener("popuphidden", function hiddenListener() {
          panel.removeEventListener("popuphidden", hiddenListener);
          SocialService.removeProvider(Social.provider.origin, finish);
        });
        panel.hidePopup();
        break;
      case "got-sidebar-message":
        
        ok(true, "got sidebar message");
        info(topic);
        gotSidebarMessage = true;
        checkNext();
        break;
    }
  }

  
  
  
  
  if (Social.provider.workerAPI.initialized) {
    iconsReady = true;
    checkNext();
  } else {
    Services.obs.addObserver(function obs() {
      Services.obs.removeObserver(obs, "social:ambient-notification-changed");
      
      
      executeSoon(function () {
        iconsReady = true;
        checkNext();
      });
    }, "social:ambient-notification-changed", false);
  }
}
