



function gc() {
  Cu.forceGC();
  let wu =  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);
  wu.garbageCollect();
}

let openChatWindow = Cu.import("resource://gre/modules/MozSocialAPI.jsm", {}).openChatWindow;



let origProxyType = Services.prefs.getIntPref('network.proxy.type');

function goOffline() {
  
  
  if (!Services.io.offline)
    BrowserOffline.toggleOfflineStatus();
  Services.prefs.setIntPref('network.proxy.type', 0);
  
  Services.cache2.clear();
}

function goOnline(callback) {
  Services.prefs.setIntPref('network.proxy.type', origProxyType);
  if (Services.io.offline)
    BrowserOffline.toggleOfflineStatus();
  if (callback)
    callback();
}

function openPanel(url, panelCallback, loadCallback) {
  
  SocialFlyout.open(url, 0, panelCallback);
  SocialFlyout.panel.firstChild.addEventListener("load", function panelLoad(evt) {
    if (evt.target != SocialFlyout.panel.firstChild.contentDocument) {
      return;
    }
    SocialFlyout.panel.firstChild.removeEventListener("load", panelLoad, true);
    loadCallback();
  }, true);
}

function openChat(url, panelCallback, loadCallback) {
  
  let chatbar = getChatBar();
  openChatWindow(null, SocialSidebar.provider, url, panelCallback);
  chatbar.firstChild.addEventListener("DOMContentLoaded", function panelLoad() {
    chatbar.firstChild.removeEventListener("DOMContentLoaded", panelLoad, true);
    loadCallback();
  }, true);
}

function onSidebarLoad(callback) {
  let sbrowser = document.getElementById("social-sidebar-browser");
  sbrowser.addEventListener("load", function load() {
    sbrowser.removeEventListener("load", load, true);
    callback();
  }, true);
}

function ensureWorkerLoaded(provider, callback) {
  
  let port = provider.getWorkerPort();
  port.onmessage = function(msg) {
    if (msg.data.topic == "pong") {
      port.close();
      callback();
    }
  }
  port.postMessage({topic: "ping"})
}

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
};

function test() {
  waitForExplicitFinish();

  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, goOnline, finishcb);
  });
}

var tests = {
  testSidebar: function(next) {
    let sbrowser = document.getElementById("social-sidebar-browser");
    onSidebarLoad(function() {
      ok(sbrowser.contentDocument.location.href.indexOf("about:socialerror?")==0, "is on social error page");
      gc();
      
      onSidebarLoad(function() {
        
        ok(sbrowser.contentDocument.location.href.indexOf("about:socialerror?")==0, "is still on social error page");
        
        goOnline();
        onSidebarLoad(function() {
          
          is(sbrowser.contentDocument.location.href, manifest.sidebarURL, "is now on social sidebar page");
          next();
        });
        sbrowser.contentDocument.getElementById("btnTryAgain").click();
      });
      sbrowser.contentDocument.getElementById("btnTryAgain").click();
    });
    
    
    ensureWorkerLoaded(SocialSidebar.provider, function() {
      
      goOffline();
      SocialSidebar.show();
  });
  },

  testFlyout: function(next) {
    let panelCallbackCount = 0;
    let panel = document.getElementById("social-flyout-panel");
    
    goOffline();
    openPanel(
      "https://example.com/browser/browser/base/content/test/social/social_panel.html",
      function() { 
        panelCallbackCount++;
      },
      function() { 
        executeSoon(function() {
          todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
          ok(panel.firstChild.contentDocument.location.href.indexOf("about:socialerror?")==0, "is on social error page");
          
          
          gc();
          openPanel(
            "https://example.com/browser/browser/base/content/test/social/social_panel.html",
            function() { 
              panelCallbackCount++;
            },
            function() { 
              executeSoon(function() {
                todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
                ok(panel.firstChild.contentDocument.location.href.indexOf("about:socialerror?")==0, "is on social error page");
                gc();
                executeSoon(function() {
                  SocialFlyout.unload();
                  next();
                });
              });
            }
          );
        });
      }
    );
  },

  testChatWindow: function(next) {
    let panelCallbackCount = 0;
    
    goOffline();
    openChat(
      "https://example.com/browser/browser/base/content/test/social/social_chat.html",
      function() { 
        panelCallbackCount++;
      },
      function() { 
        executeSoon(function() {
          todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
          let chat = getChatBar().selectedChat;
          waitForCondition(function() chat.contentDocument.location.href.indexOf("about:socialerror?")==0,
                           function() {
                            chat.close();
                            next();
                            },
                           "error page didn't appear");
        });
      }
    );
  },

  testChatWindowAfterTearOff: function(next) {
    
    let url = "https://example.com/browser/browser/base/content/test/social/social_chat.html";
    let panelCallbackCount = 0;
    
    openChat(
      url,
      null,
      function() { 
        executeSoon(function() {
          let chat = getChatBar().selectedChat;
          is(chat.contentDocument.location.href, url, "correct url loaded");
          
          chat.swapWindows().then(
            chat => {
              
              goOffline();
              chat.contentDocument.location.reload();
              waitForCondition(function() chat.contentDocument.location.href.indexOf("about:socialerror?")==0,
                               function() {
                                chat.close();
                                next();
                                },
                               "error page didn't appear");
            }
          );
        });
      }
    );
  }
}
