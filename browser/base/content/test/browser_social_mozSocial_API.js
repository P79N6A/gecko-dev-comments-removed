



function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, undefined, finishcb);
  });
}

var tests = {
  testStatusIcons: function(next) {
    let iconsReady = false;
    let gotSidebarMessage = false;

    function checkNext() {
      if (iconsReady && gotSidebarMessage)
        triggerIconPanel();
    }

    function triggerIconPanel() {
      let statusIcons = document.getElementById("social-status-iconbox");
      ok(!statusIcons.firstChild.hidden, "status icon is visible");
      
      let panel = document.getElementById("social-notification-panel");
      EventUtils.synthesizeMouseAtCenter(statusIcons.firstChild, {});
    }

    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-panel-message":
          ok(true, "got panel message");
          
          ensureSocialUrlNotRemembered(e.data.location);
          break;
        case "got-social-panel-visibility":
          if (e.data.result == "shown") {
            ok(true, "panel shown");
            let panel = document.getElementById("social-notification-panel");
            panel.hidePopup();
          } else if (e.data.result == "hidden") {
            ok(true, "panel hidden");
            port.close();
            next();
          }
          break;
        case "got-sidebar-message":
          
          ok(true, "got sidebar message");
          gotSidebarMessage = true;
          
          port.postMessage({topic: "test-ambient-notification"});
          checkNext();
          break;
      }
    }
    port.postMessage({topic: "test-init"});

    
    
    
    
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
}
