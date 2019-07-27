



function gc() {
  Cu.forceGC();
  let wu =  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);
  wu.garbageCollect();
}

let openChatWindow = Cu.import("resource://gre/modules/MozSocialAPI.jsm", {}).openChatWindow;

function openPanel(url, panelCallback, loadCallback) {
  
  SocialFlyout.open(url, 0, panelCallback);
  
  
  
  waitForCondition(function() {
                    return SocialFlyout.panel.state == "open" &&
                           SocialFlyout.iframe.contentDocument.readyState == "complete";
                   },
                   function () { executeSoon(loadCallback) },
                   "flyout is open and loaded");
}

function openChat(url, panelCallback, loadCallback) {
  
  let chatbar = getChatBar();
  openChatWindow(null, SocialSidebar.provider, url, panelCallback);
  chatbar.firstChild.addEventListener("DOMContentLoaded", function panelLoad() {
    chatbar.firstChild.removeEventListener("DOMContentLoaded", panelLoad, true);
    executeSoon(loadCallback);
  }, true);
}

function onSidebarLoad(callback) {
  let sbrowser = document.getElementById("social-sidebar-browser");
  sbrowser.addEventListener("load", function load() {
    sbrowser.removeEventListener("load", load, true);
    executeSoon(callback);
  }, true);
}

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar_empty.html",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
};

function test() {
  waitForExplicitFinish();

  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, function(next) { goOnline().then(next) }, finishcb);
  });
}

var tests = {
  testSidebar: function(next) {
    let sbrowser = document.getElementById("social-sidebar-browser");
    onSidebarLoad(function() {
      ok(sbrowser.contentDocument.location.href.indexOf("about:socialerror?")==0, "sidebar is on social error page");
      gc();
      
      onSidebarLoad(function() {
        
        ok(sbrowser.contentDocument.location.href.indexOf("about:socialerror?")==0, "sidebar is still on social error page");
        
        goOnline().then(function () {
          onSidebarLoad(function() {
            
            is(sbrowser.contentDocument.location.href, manifest.sidebarURL, "sidebar is now on social sidebar page");
            next();
          });
          sbrowser.contentDocument.getElementById("btnTryAgain").click();
        });
      });
      sbrowser.contentDocument.getElementById("btnTryAgain").click();
    });
    
    goOffline().then(function() {
      SocialSidebar.show();
    });
  },

  testFlyout: function(next) {
    let panelCallbackCount = 0;
    let panel = document.getElementById("social-flyout-panel");
    goOffline().then(function() {
      openPanel(
        manifest.sidebarURL, 
        function() { 
          panelCallbackCount++;
        },
        function() { 
          todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
          let href = panel.firstChild.contentDocument.location.href;
          ok(href.indexOf("about:socialerror?")==0, "flyout is on social error page");
          
          
          gc();
          openPanel(
            manifest.sidebarURL, 
            function() { 
              panelCallbackCount++;
            },
            function() { 
              todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
              let href = panel.firstChild.contentDocument.location.href;
              ok(href.indexOf("about:socialerror?")==0, "flyout is on social error page");
              gc();
              SocialFlyout.unload();
              next();
            }
          );
        }
      );
    });
  },

  testChatWindow: function(next) {
    let panelCallbackCount = 0;
    
    
    goOffline().then(function() {
      openChat(
        manifest.sidebarURL, 
        function() { 
          panelCallbackCount++;
        },
        function() { 
          todo_is(panelCallbackCount, 0, "Bug 833207 - should be no callback when error page loads.");
          let chat = getChatBar().selectedChat;
          waitForCondition(function() chat.content != null && chat.contentDocument.location.href.indexOf("about:socialerror?")==0,
                           function() {
                            chat.close();
                            next();
                            },
                           "error page didn't appear");
        }
      );
    });
  },

  testChatWindowAfterTearOff: function(next) {
    
    let url = manifest.sidebarURL; 
    let panelCallbackCount = 0;
    
    
    
    openChat(
      url,
      null,
      function() { 
        let chat = getChatBar().selectedChat;
        is(chat.contentDocument.location.href, url, "correct url loaded");
        
        chat.swapWindows().then(
          chat => {
            ok(!!chat.content, "we have chat content 1");
            waitForCondition(function() chat.content != null && chat.contentDocument.readyState == "complete",
                             function() {
              
              goOffline().then(function() {
                ok(!!chat.content, "we have chat content 2");
                chat.contentDocument.location.reload();
                info("chat reload called");
                waitForCondition(function() chat.contentDocument.location.href.indexOf("about:socialerror?")==0,
                                 function() {
                                  chat.close();
                                  next();
                                  },
                                 "error page didn't appear");
              });
            }, "swapped window loaded");
          }
        );
      }
    );
  }
}
