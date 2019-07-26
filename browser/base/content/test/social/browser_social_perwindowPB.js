



function openTab(win, url, callback) {
  let newTab = win.gBrowser.addTab(url);
  let tabBrowser = win.gBrowser.getBrowserForTab(newTab);
  tabBrowser.addEventListener("load", function tabLoadListener() {
    tabBrowser.removeEventListener("load", tabLoadListener, true);
    win.gBrowser.selectedTab = newTab;
    callback(newTab);
  }, true)
}


function openPBWindow(callback) {
  let w = OpenBrowserWindow({private: true});
  w.addEventListener("load", function loadListener() {
    w.removeEventListener("load", loadListener);
    openTab(w, "http://example.com", function() {
      callback(w);
    });
  });
}

function postAndReceive(port, postTopic, receiveTopic, callback) {
  port.onmessage = function(e) {
    if (e.data.topic == receiveTopic)
      callback();
  }
  port.postMessage({topic: postTopic});
}

function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/social/moz.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    openTab(window, "http://example.com", function(newTab) {
      runSocialTests(tests, undefined, undefined, function() {
        window.gBrowser.removeTab(newTab);
        finishcb();
      });
    });
  });
}

var tests = {
  testPrivateBrowsing: function(next) {
    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    postAndReceive(port, "test-init", "test-init-done", function() {
      
      info("checking main window ui");
      ok(window.SocialUI.enabled, "social is enabled in normal window");
      checkSocialUI(window);
      
      openPBWindow(function(pbwin) {
        
        postAndReceive(port, "ping", "pong", function() {
          
          info("checking private window ui");
          ok(!pbwin.SocialUI.enabled, "social is disabled in a PB window");
          checkSocialUI(pbwin);
          
          info("checking main window ui");
          ok(window.SocialUI.enabled, "social is still enabled in normal window");
          checkSocialUI(window);
          
          pbwin.close();
          next();
        })
      });
    });
  },
}
