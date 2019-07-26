






const pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);


function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, undefined, finishcb);
  });
}

var tests = {
  testPrivateBrowsing: function(next) {
    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          ok(true, "got sidebar message");
          port.close();
          togglePrivateBrowsing(function () {
            ok(!Social.enabled, "Social shuts down during private browsing");
            togglePrivateBrowsing(function () {
              ok(Social.enabled, "Social enabled after private browsing");
              next();
            });
          });
          break;
      }
    };
  },

  testPrivateBrowsingSocialDisabled: function(next) {
    
    
    
    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          ok(true, "got sidebar message");
          port.close();
          Social.enabled = false;
          break;
      }
    }

    
    
    Services.obs.addObserver(function observer(aSubject, aTopic) {
      Services.obs.removeObserver(observer, aTopic);
      ok(!Social.enabled, "Social is not enabled");
      togglePrivateBrowsing(function () {
        ok(!Social.enabled, "Social not available during private browsing");
        togglePrivateBrowsing(function () {
          ok(!Social.enabled, "Social is not enabled after private browsing");
          
          next();
        });
      });
    }, "social:pref-changed", false);
  }
}

function togglePrivateBrowsing(aCallback) {
  Services.obs.addObserver(function observe(subject, topic, data) {
    Services.obs.removeObserver(observe, topic);
    executeSoon(aCallback);
  }, "private-browsing-transition-complete", false);

  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}
