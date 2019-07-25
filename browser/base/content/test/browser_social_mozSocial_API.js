



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
            next();
          });
          panel.hidePopup();
          break;
        case "got-sidebar-message":
          
          ok(true, "got sidebar message");
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
  },

  testServiceWindow: function(next) {
    
    
    let port = Social.provider.port;
    ok(port, "provider has a port");
    port.postMessage({topic: "test-service-window"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-service-window-message":
          
          ok(true, "got service window message");
          port.postMessage({topic: "test-close-service-window"});
          break;
        case "got-service-window-closed-message":
          ok(true, "got service window closed message");
          next();
          break;
      }
    }
  },

  testServiceWindowTwice: function(next) {
    let port = Social.provider.port;
    port.postMessage({topic: "test-service-window-twice"});
    Social.provider.port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-service-window-twice-result":
          is(e.data.result, "ok", "only one window should open when name is reused");
          break;
        case "got-service-window-message":
          ok(true, "got service window message");
          port.postMessage({topic: "test-close-service-window"});
          break;
        case "got-service-window-closed-message":
          ok(true, "got service window closed message");
          next();
          break;
      }
    }
  }
}
