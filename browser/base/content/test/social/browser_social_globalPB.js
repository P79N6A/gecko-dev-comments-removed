






const pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

function waitForPortMessage(port, topic, callback) {
  port.onmessage = function(evt) {
    if (evt.data.topic == topic)
      callback(evt.data);
  }
}

function portClosed(port) {
  try {
    port.postMessage({topic: "ping"});
    return false; 
  } catch (ex) {
    return true;
  }
}

function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
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
              ok(Social.provider.getWorkerPort(), "port still obtainable after PB")
              ok(Social.enabled, "Social enabled after private browsing");
              next();
            });
          });
          break;
      }
    };
  },

  testPrivateBrowsingSocialDisabled: function(next) {
    
    
    
    ok(Social.enabled, "social is still enabled");
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
          
          Social.enabled = true;
          next();
        });
      });
    }, "social:pref-changed", false);
  },

  testPrivateBrowsingExitReloads: function(next) {
    let port = Social.provider.getWorkerPort();
    waitForPortMessage(port, "got-sidebar-message", function(data) {
      ok(!portClosed(port), "port not closed before PB transition");
      togglePrivateBrowsing(function () {
        ok(!Social.enabled, "Social shuts down during private browsing");
        
        ok(portClosed(port), "port closed after PB transition");
        
        Social.enabled = true;
        port = Social.provider.getWorkerPort();
        waitForPortMessage(port, "got-sidebar-message", function(data) {
          
          
          
          let sbw = document.getElementById("social-sidebar-browser").contentWindow;
          sbw.wrappedJSObject.foo = "bar";
          
          togglePrivateBrowsing(function () {
            ok(Social.enabled, "Social still enabled after leaving private browsing");
            ok(portClosed(port), "port closed after PB transition");
            port = Social.provider.getWorkerPort();
            waitForPortMessage(port, "got-sidebar-message", function() {
              sbw = document.getElementById("social-sidebar-browser").contentWindow;
              is(sbw.wrappedJSObject.foo, undefined, "should have lost window variable when exiting")
              next();
            });
            port.postMessage({topic: "test-init"});
          });
        });
        port.postMessage({topic: "test-init"});
      });
    });
    port.postMessage({topic: "test-init"});
  },

}

function togglePrivateBrowsing(aCallback) {
  Services.obs.addObserver(function observe(subject, topic, data) {
    Services.obs.removeObserver(observe, topic);
    executeSoon(aCallback);
  }, "private-browsing-transition-complete", false);

  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}
