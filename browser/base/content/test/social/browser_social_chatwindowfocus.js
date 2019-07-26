




function isTabFocused() {
  let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  return Services.focus.focusedWindow == tabb.contentWindow;
}

function isChatFocused(chat) {
  return getChatBar()._isChatFocused(chat);
}

function openChatViaUser() {
  let sidebarDoc = document.getElementById("social-sidebar-browser").contentDocument;
  let button = sidebarDoc.getElementById("chat-opener");
  
  
  
  EventUtils.synthesizeMouseAtCenter(button, {}, sidebarDoc.defaultView);
}

function openChatViaSidebarMessage(port, data, callback) {
  port.onmessage = function (e) {
    if (e.data.topic == "chatbox-opened")
      callback();
  }
  port.postMessage({topic: "test-chatbox-open", data: data});
}

function openChatViaWorkerMessage(port, data, callback) {
  
  
  let chatbar = getChatBar();
  let numExpected = chatbar.childElementCount + 1;
  port.postMessage({topic: "test-worker-chat", data: data});
  waitForCondition(function() chatbar.childElementCount == numExpected,
                   function() {
                      
                      
                      
                      chatbar.openChat(SocialSidebar.provider.origin,
                                       SocialSidebar.provider.name,
                                       data,
                                       "minimized",
                                       function() {
                                          callback();
                                       });
                   },
                   "No new chat appeared");
}


let isSidebarLoaded = false;

function startTestAndWaitForSidebar(callback) {
  let doneCallback;
  let port = SocialSidebar.provider.getWorkerPort();
  function maybeCallback() {
    if (!doneCallback)
      callback(port);
    doneCallback = true;
  }
  port.onmessage = function(e) {
    let topic = e.data.topic;
    switch (topic) {
      case "got-sidebar-message":
        
      case "got-isVisible-response":
        isSidebarLoaded = true;
        maybeCallback();
        break;
      case "test-init-done":
        if (isSidebarLoaded)
          maybeCallback();
        else
          port.postMessage({topic: "test-isVisible"});
        break;
    }
  }
  port.postMessage({topic: "test-init"});
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

  
  
  
  
  let url = "data:text/html;charset=utf-8," + encodeURI('<input id="theinput">');
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    
    let preSubTest = function(cb) {
      
      
      
      tab.linkedBrowser.contentDocument.getElementById("theinput").focus();
      waitForCondition(function() isTabFocused(), cb, "tab should have focus");
    }
    let postSubTest = function(cb) {
      closeAllChats();
      cb();
    }
    
    runSocialTestWithProvider(manifest, function (finishcb) {
      SocialSidebar.show();
      runSocialTests(tests, preSubTest, postSubTest, function () {
        finishcb();
      });
    });
  }, true);
  registerCleanupFunction(function() {
    gBrowser.removeTab(tab);
  });

}

var tests = {
  
  
  
  
  testNoFocusWhenViaWorker: function(next) {
    let chatbar = getChatBar();
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
        ok(true, "got chatbox message");
        is(chatbar.childElementCount, 1, "exactly 1 chat open");
        ok(isTabFocused(), "tab should still be focused");
        
        openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
          is(chatbar.childElementCount, 1, "still exactly 1 chat open");
          ok(isTabFocused(), "tab should still be focused");
          
          openChatViaUser();
          waitForCondition(function() isChatFocused(chatbar.selectedChat),
                           function() {
            is(chatbar.childElementCount, 1, "still exactly 1 chat open");
            is(chatbar.selectedChat, chatbar.firstElementChild, "chat should be selected");
            next();
          }, "chat should be focused");
        });
      });
    });
  },

  
  
  testFocusWhenViaUser: function(next) {
    startTestAndWaitForSidebar(function(port) {
      let chatbar = getChatBar();
      openChatViaUser();
      ok(chatbar.firstElementChild, "chat opened");
      waitForCondition(function() isChatFocused(chatbar.selectedChat),
                       function() {
        is(chatbar.selectedChat, chatbar.firstElementChild, "chat is selected");
        next();
      }, "chat should be focused");
    });
  },
};
