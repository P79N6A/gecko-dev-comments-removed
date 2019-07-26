



function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, undefined, finishcb);
  });
}

var tests = {
  testStatusIcons: function(next) {
    let icon = {
      name: "testIcon",
      iconURL: "chrome://browser/skin/Info.png",
      contentPanel: "https://example.com/browser/browser/base/content/test/social/social_panel.html",
      counter: 1
    };

    let iconsReady = false;
    let gotSidebarMessage = false;

    function checkNext() {
      if (iconsReady && gotSidebarMessage)
        triggerIconPanel();
    }

    function triggerIconPanel() {
      let pButton = document.getElementById("social-provider-button");
      waitForCondition(function() {
        
        
        return !!pButton.nextSibling;
      }, function() {
        
        let statusIcon = pButton.nextSibling;
        EventUtils.synthesizeMouseAtCenter(statusIcon, {});
      }, "Status icon didn't become non-hidden");
    }

    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          iconsReady = true;
          checkNext();
          break;
        case "got-panel-message":
          ok(true, "got panel message");
          
          gURLsNotRemembered.push(e.data.location);
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
          
          port.postMessage({topic: "test-ambient-notification", data: icon});
          checkNext();
          break;
      }
    }
    port.postMessage({topic: "test-init"});
  }
}
