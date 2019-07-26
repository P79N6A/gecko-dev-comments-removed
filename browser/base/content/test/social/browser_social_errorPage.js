



function gc() {
  Cu.forceGC();
  let wu =  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);
  wu.garbageCollect();
}



let origProxyType = Services.prefs.getIntPref('network.proxy.type');

function goOffline() {
  
  
  if (!Services.io.offline)
    BrowserOffline.toggleOfflineStatus();
  Services.prefs.setIntPref('network.proxy.type', 0);
  
  Services.cache.evictEntries(Components.interfaces.nsICache.STORE_ANYWHERE);
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
  SocialFlyout.panel.firstChild.addEventListener("load", function panelLoad() {
    SocialFlyout.panel.firstChild.removeEventListener("load", panelLoad, true);
    loadCallback();
  }, true);
}

function openChat(url, panelCallback, loadCallback) {
  
  SocialChatBar.openChat(Social.provider, url, panelCallback);
  SocialChatBar.chatbar.firstChild.addEventListener("DOMContentLoaded", function panelLoad() {
    SocialChatBar.chatbar.firstChild.removeEventListener("DOMContentLoaded", panelLoad, true);
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

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
};

function test() {
  waitForExplicitFinish();
  
  Services.prefs.setBoolPref("social.sidebar.open", false);
  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("social.sidebar.open");
  });

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
    
    goOffline();
    Services.prefs.setBoolPref("social.sidebar.open", true);
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
                SocialFlyout.unload();
                next();
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
          let iframe = SocialChatBar.chatbar.selectedChat.iframe;
          waitForCondition(function() iframe.contentDocument.location.href.indexOf("about:socialerror?")==0,
                           function() {
                            SocialChatBar.chatbar.selectedChat.close();
                            next();
                            },
                           "error page didn't appear");
        });
      }
    );
  }
}
