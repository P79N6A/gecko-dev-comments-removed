




function isTabFocused() {
  let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  return Services.focus.focusedWindow == tabb.contentWindow;
}

function isChatFocused(chat) {
  return SocialChatBar.chatbar._isChatFocused(chat);
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
  
  
  let chatbar = SocialChatBar.chatbar;
  let numExpected = chatbar.childElementCount + 1;
  port.postMessage({topic: "test-worker-chat", data: data});
  waitForCondition(function() chatbar.childElementCount == numExpected,
                   function() {
                      
                      
                      
                      SocialChatBar.openChat(Social.provider,
                                             data,
                                             function() {
                                                callback();
                                             },
                                             "minimized");
                   },
                   "No new chat appeared");
}


let isSidebarLoaded = false;

function startTestAndWaitForSidebar(callback) {
  let doneCallback;
  let port = Social.provider.getWorkerPort();
  function maybeCallback() {
    if (!doneCallback)
      callback(port);
    doneCallback = true;
  }
  port.onmessage = function(e) {
    let topic = e.data.topic;
    switch (topic) {
      case "got-sidebar-message":
        isSidebarLoaded = true;
        maybeCallback();
        break;
      case "test-init-done":
        if (isSidebarLoaded)
          maybeCallback();
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
  iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
};

function test() {
  waitForExplicitFinish();

  
  
  
  
  let url = "data:text/html;charset=utf-8," + encodeURI('<input id="theinput">');
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    
    let preSubTest = function(cb) {
      
      
      
      tab.linkedBrowser.contentDocument.getElementById("theinput").focus();
      cb();
    }
    let postSubTest = function(cb) {
      window.SocialChatBar.chatbar.removeAll();
      cb();
    }
    
    runSocialTestWithProvider(manifest, function (finishcb) {
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
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
        ok(true, "got chatbox message");
        is(SocialChatBar.chatbar.childElementCount, 1, "exactly 1 chat open");
        ok(isTabFocused(), "tab should still be focused");
        
        openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
          is(SocialChatBar.chatbar.childElementCount, 1, "still exactly 1 chat open");
          ok(isTabFocused(), "tab should still be focused");
          
          openChatViaUser();
          is(SocialChatBar.chatbar.childElementCount, 1, "still exactly 1 chat open");
          
          ok(isChatFocused(SocialChatBar.chatbar.firstElementChild), "chat should be focused");
          next();
        });
      });
    });
  },

  
  
  testFocusWhenViaUser: function(next) {
    startTestAndWaitForSidebar(function(port) {
      openChatViaUser();
      ok(SocialChatBar.chatbar.firstElementChild, "chat opened");
      ok(isChatFocused(SocialChatBar.chatbar.firstElementChild), "chat should be focused");
      next();
    });
  },

  
  
  
  testNoFocusOnAutoRestore: function(next) {
    const chatUrl = "https://example.com/browser/browser/base/content/test/social/social_chat.html?id=1";
    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaWorkerMessage(port, chatUrl, function() {
        is(chatbar.childElementCount, 1, "exactly 1 chat open");
        ok(chatbar.firstElementChild.minimized, "chat is minimized");
        ok(isTabFocused(), "tab should be focused");
        openChatViaSidebarMessage(port, {stealFocus: 1, id: 1}, function() {
          is(chatbar.childElementCount, 1, "still 1 chat open");
          ok(!chatbar.firstElementChild.minimized, "chat no longer minimized");
          ok(isTabFocused(), "tab should still be focused");
          next();
        });
      });
    });
  },

  
  
  testFocusOnExplicitRestore: function(next) {
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
        ok(true, "got chatbox message");
        ok(isTabFocused(), "tab should still be focused");
        let chatbox = SocialChatBar.chatbar.firstElementChild;
        ok(chatbox, "chat opened");
        chatbox.minimized = true;
        ok(isTabFocused(), "tab should still be focused");
        
        chatbox.onTitlebarClick({button: 0});
        ok(!chatbox.minimized, "chat should have been restored");
        ok(isChatFocused(chatbox), "chat should be focused");
        next();
      });
    });
  },

  
  
  testMinimizeFocused: function(next) {
    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {stealFocus: 1, id: 1}, function() {
        let chat1 = chatbar.firstElementChild;
        openChatViaSidebarMessage(port, {stealFocus: 1, id: 2}, function() {
          is(chatbar.childElementCount, 2, "exactly 2 chats open");
          let chat2 = chat1.nextElementSibling || chat1.previousElementSibling;
          chatbar.selectedChat = chat1;
          chatbar.focus();
          ok(isChatFocused(chat1), "first chat should be focused");
          chat1.minimized = true;
          
          ok(isChatFocused(chat2), "second chat should be focused");
          next();
        });
      });
    });
  },

  
  
  testReopenNonFocused: function(next) {
    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {id: 1}, function() {
        let chat1 = chatbar.firstElementChild;
        openChatViaSidebarMessage(port, {id: 2}, function() {
          let chat2 = chat1.nextElementSibling || chat1.previousElementSibling;
          chatbar.selectedChat = chat2;
          
          ok(isTabFocused(), "tab should still be focused");
          
          openChatViaSidebarMessage(port, {id: 1}, function() {
            is(chatbar.selectedChat, chat1, "chat1 now selected");
            ok(isTabFocused(), "tab should still be focused");
            next();
          });
        });
      });
    });
  },

  
  
  
  testTab: function(next) {
    function sendTabAndWaitForFocus(chat, eltid, callback) {
      
      
      
      
      let doc = chat.iframe.contentDocument;
      EventUtils.sendKey("tab");
      waitForCondition(function() {
        let elt = eltid ? doc.getElementById(eltid) : doc.documentElement;
        return doc.activeElement == elt;
      }, callback, "element " + eltid + " never got focus");
    }

    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {id: 1}, function() {
        let chat1 = chatbar.firstElementChild;
        openChatViaSidebarMessage(port, {id: 2}, function() {
          let chat2 = chat1.nextElementSibling || chat1.previousElementSibling;
          chatbar.selectedChat = chat2;
          chatbar.focus();
          ok(isChatFocused(chat2), "new chat is focused");
          
          
          sendTabAndWaitForFocus(chat2, "input1", function() {
            is(chat2.iframe.contentDocument.activeElement.getAttribute("id"), "input1",
               "first input field has focus");
            ok(isChatFocused(chat2), "new chat still focused after first tab");
            sendTabAndWaitForFocus(chat2, "input2", function() {
              ok(isChatFocused(chat2), "new chat still focused after tab");
              is(chat2.iframe.contentDocument.activeElement.getAttribute("id"), "input2",
                 "second input field has focus");
              sendTabAndWaitForFocus(chat2, "iframe", function() {
                ok(isChatFocused(chat2), "new chat still focused after tab");
                is(chat2.iframe.contentDocument.activeElement.getAttribute("id"), "iframe",
                   "iframe has focus");
                
                
                sendTabAndWaitForFocus(chat1, null, function() {
                  ok(isChatFocused(chat1), "first chat is focused");
                  next();
                });
              });
            });
          });
        });
      });
    });
  },

  
  
  
  testFocusedElement: function(next) {
    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaUser();
      let chat = chatbar.firstElementChild;
      
      chat.addEventListener("DOMContentLoaded", function DOMContentLoaded() {
        chat.removeEventListener("DOMContentLoaded", DOMContentLoaded);
        chat.iframe.contentDocument.getElementById("input2").focus();
        is(chat.iframe.contentDocument.activeElement.getAttribute("id"), "input2",
           "correct input field has focus");
        
        let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
        Services.focus.moveFocus(tabb.contentWindow, null, Services.focus.MOVEFOCUS_ROOT, 0);
        ok(isTabFocused(), "tab took focus");
        chatbar.focus();
        ok(isChatFocused(chat), "chat took focus");
        is(chat.iframe.contentDocument.activeElement.getAttribute("id"), "input2",
           "correct input field still has focus");
        next();
      });
    });
  },
};
