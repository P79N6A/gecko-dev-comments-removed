



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
    SocialSidebar.show();
    runSocialTests(tests, undefined, undefined, finishcb);
  });
}

var tests = {
  testSidebarMessage: function(next) {
    let port = SocialSidebar.provider.getWorkerPort();
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
    let port = SocialSidebar.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-isVisible-response":
          is(e.data.result, true, "Sidebar should be visible by default");
          SocialSidebar.toggleSidebar();
          port.close();
          next();
      }
    };
    port.postMessage({topic: "test-isVisible"});
  },
  testIsNotVisible: function(next) {
    let port = SocialSidebar.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-isVisible-response":
          is(e.data.result, false, "Sidebar should be hidden");
          port.close();
          next();
      }
    };
    port.postMessage({topic: "test-isVisible"});
  }
}
