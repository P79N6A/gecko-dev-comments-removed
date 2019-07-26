



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
  testSidebarMessage: function(next) {
    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          
          ok(true, "got sidebar message");
          port.close();
          next();
          break;
      }
    };
  },
  testIsVisible: function(next) {
    let port = Social.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-isVisible-response":
          is(e.data.result, true, "Sidebar should be visible by default");
          Social.toggleSidebar();
          port.close();
          next();
      }
    };
    port.postMessage({topic: "test-isVisible"});
  },
  testIsNotVisible: function(next) {
    let port = Social.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-isVisible-response":
          is(e.data.result, false, "Sidebar should be hidden");
          Services.prefs.clearUserPref("social.sidebar.open");
          port.close();
          next();
      }
    };
    port.postMessage({topic: "test-isVisible"});
  }
}
