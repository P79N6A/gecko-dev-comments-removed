




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
                      
                      
                      
                      SocialChatBar.openChat(SocialSidebar.provider,
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
      window.SocialChatBar.chatbar.removeAll();
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
    startTestAndWaitForSidebar(function(port) {
      openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
        ok(true, "got chatbox message");
        is(SocialChatBar.chatbar.childElementCount, 1, "exactly 1 chat open");
        ok(isTabFocused(), "tab should still be focused");
        
        openChatViaSidebarMessage(port, {stealFocus: 1}, function() {
          is(SocialChatBar.chatbar.childElementCount, 1, "still exactly 1 chat open");
          ok(isTabFocused(), "tab should still be focused");
          
          openChatViaUser();
          waitForCondition(function() isChatFocused(SocialChatBar.chatbar.selectedChat),
                           function() {
            is(SocialChatBar.chatbar.childElementCount, 1, "still exactly 1 chat open");
            is(SocialChatBar.chatbar.selectedChat, SocialChatBar.chatbar.firstElementChild, "chat should be selected");
            next();
          }, "chat should be focused");
        });
      });
    });
  },

  
  
  testFocusWhenViaUser: function(next) {
    startTestAndWaitForSidebar(function(port) {
      openChatViaUser();
      ok(SocialChatBar.chatbar.firstElementChild, "chat opened");
      waitForCondition(function() isChatFocused(SocialChatBar.chatbar.selectedChat),
                       function() {
        is(SocialChatBar.chatbar.selectedChat, SocialChatBar.chatbar.firstElementChild, "chat is selected");
        next();
      }, "chat should be focused");
    });
  },

  
  
  
  testNoFocusOnAutoRestore: function(next) {
    const chatUrl = "https://example.com/browser/browser/base/content/test/social/social_chat.html?id=1";
    let chatbar = SocialChatBar.chatbar;
    startTestAndWaitForSidebar(function(port) {
      openChatViaWorkerMessage(port, chatUrl, function() {
        is(chatbar.childElementCount, 1, "exactly 1 chat open");
        
        todo(chatbar.selectedChat != chatbar.firstElementChild, "chat is not selected");
        ok(isTabFocused(), "tab should be focused");
        openChatViaSidebarMessage(port, {stealFocus: 1, id: 1}, function() {
          is(chatbar.childElementCount, 1, "still 1 chat open");
          ok(!chatbar.firstElementChild.minimized, "chat no longer minimized");
          
          todo(chatbar.selectedChat != chatbar.firstElementChild, "chat is not selected");
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
        waitForCondition(function() isChatFocused(SocialChatBar.chatbar.selectedChat),
                         function() {
          ok(!chatbox.minimized, "chat should have been restored");
          ok(isChatFocused(chatbox), "chat should be focused");
          is(chatbox, SocialChatBar.chatbar.selectedChat, "chat is marked selected");
          next();
        }, "chat should have focus");
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
          waitForCondition(function() isChatFocused(chat1),
                           function() {
            is(chat1, SocialChatBar.chatbar.selectedChat, "chat1 is marked selected");
            isnot(chat2, SocialChatBar.chatbar.selectedChat, "chat2 is not marked selected");
            chat1.minimized = true;
            waitForCondition(function() isChatFocused(chat2),
                             function() {
              
              isnot(chat1, SocialChatBar.chatbar.selectedChat, "chat1 is not marked selected");
              is(chat2, SocialChatBar.chatbar.selectedChat, "chat2 is marked selected");
              next();
            }, "chat2 should have focus");
          }, "chat1 should have focus");
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
      
      
      
      
      let doc = chat.contentDocument;
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
          waitForCondition(function() isChatFocused(chatbar.selectedChat),
                           function() {
            
            
            sendTabAndWaitForFocus(chat2, "input1", function() {
              is(chat2.contentDocument.activeElement.getAttribute("id"), "input1",
                 "first input field has focus");
              ok(isChatFocused(chat2), "new chat still focused after first tab");
              sendTabAndWaitForFocus(chat2, "input2", function() {
                ok(isChatFocused(chat2), "new chat still focused after tab");
                is(chat2.contentDocument.activeElement.getAttribute("id"), "input2",
                   "second input field has focus");
                sendTabAndWaitForFocus(chat2, "iframe", function() {
                  ok(isChatFocused(chat2), "new chat still focused after tab");
                  is(chat2.contentDocument.activeElement.getAttribute("id"), "iframe",
                     "iframe has focus");
                  
                  
                  sendTabAndWaitForFocus(chat1, null, function() {
                    ok(isChatFocused(chat1), "first chat is focused");
                    next();
                  });
                });
              });
            });
          }, "chat should have focus");
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
        chat.contentDocument.getElementById("input2").focus();
        waitForCondition(function() isChatFocused(chat),
                         function() {
          is(chat.contentDocument.activeElement.getAttribute("id"), "input2",
             "correct input field has focus");
          
          let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
          Services.focus.moveFocus(tabb.contentWindow, null, Services.focus.MOVEFOCUS_ROOT, 0);
          waitForCondition(function() isTabFocused(),
                           function() {
            chatbar.focus();
            waitForCondition(function() isChatFocused(chat),
                             function() {
              is(chat.contentDocument.activeElement.getAttribute("id"), "input2",
                 "correct input field still has focus");
              next();
            }, "chat took focus");
          }, "tab has focus");
        }, "chat took focus");
      });
    });
  },
};
