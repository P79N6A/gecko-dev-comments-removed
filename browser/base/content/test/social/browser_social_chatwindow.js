



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let manifests = [
  {
    name: "provider@example.com",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html?example.com",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  },
  {
    name: "provider@test1",
    origin: "https://test1.example.com",
    sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html?test1",
    workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  },
  {
    name: "provider@test2",
    origin: "https://test2.example.com",
    sidebarURL: "https://test2.example.com/browser/browser/base/content/test/social/social_sidebar.html?test2",
    workerURL: "https://test2.example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  }
];

let chatId = 0;
function openChat(provider, callback) {
  let chatUrl = provider.origin + "/browser/browser/base/content/test/social/social_chat.html";
  let port = provider.getWorkerPort();
  port.onmessage = function(e) {
    if (e.data.topic == "got-chatbox-message") {
      port.close();
      callback();
    }
  }
  let url = chatUrl + "?" + (chatId++);
  port.postMessage({topic: "test-init"});
  port.postMessage({topic: "test-worker-chat", data: url});
  gURLsNotRemembered.push(url);
}

function windowHasChats(win) {
  let chatbar = win.document.getElementById("pinnedchats");
  return !!chatbar.firstElementChild;
}

function test() {
  requestLongerTimeout(2); 
  waitForExplicitFinish();

  let oldwidth = window.outerWidth; 
  let oldleft = window.screenX;
  window.moveTo(0, window.screenY)
  let postSubTest = function(cb) {
    let chats = document.getElementById("pinnedchats");
    ok(chats.children.length == 0, "no chatty children left behind");
    cb();
  };
  runSocialTestWithProvider(manifests, function (finishcb) {
    ok(Social.enabled, "Social is enabled");
    ok(Social.providers[0].getWorkerPort(), "provider 0 has port");
    ok(Social.providers[1].getWorkerPort(), "provider 1 has port");
    ok(Social.providers[2].getWorkerPort(), "provider 2 has port");
    SocialSidebar.show();
    runSocialTests(tests, undefined, postSubTest, function() {
      window.moveTo(oldleft, window.screenY)
      window.resizeTo(oldwidth, window.outerHeight);
      finishcb();
    });
  });
}

var tests = {
  testOpenCloseChat: function(next) {
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "got-chatbox-visibility":
          if (e.data.result == "hidden") {
            ok(true, "chatbox got minimized");
            chats.selectedChat.toggle();
          } else if (e.data.result == "shown") {
            ok(true, "chatbox got shown");
            
            let content = chats.selectedChat.content;
            content.addEventListener("unload", function chatUnload() {
              content.removeEventListener("unload", chatUnload, true);
              ok(true, "got chatbox unload on close");
              port.close();
              next();
            }, true);
            chats.selectedChat.close();
          }
          break;
        case "got-chatbox-message":
          ok(true, "got chatbox message");
          ok(e.data.result == "ok", "got chatbox windowRef result: "+e.data.result);
          chats.selectedChat.toggle();
          break;
      }
    }
    port.postMessage({topic: "test-init", data: { id: 1 }});
  },
  testOpenMinimized: function(next) {
    
    
    
    
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    let seen_opened = false;
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "chatbox-opened":
          is(e.data.result, "ok", "the sidebar says it got a chatbox");
          if (!seen_opened) {
            
            
            
            ok(!chats.selectedChat.minimized, "chat not initially minimized")
            chats.selectedChat.minimized = true
            seen_opened = true;
            port.postMessage({topic: "test-chatbox-open"});
          } else {
            
            
            let chats = document.getElementById("pinnedchats");
            ok(!chats.selectedChat.minimized, "chat no longer minimized")
            chats.selectedChat.close();
            is(chats.selectedChat, null, "should only have been one chat open");
            port.close();
            next();
          }
      }
    }
    port.postMessage({topic: "test-init", data: { id: 1 }});
  },
  testManyChats: function(next) {
    
    
    let port = SocialSidebar.provider.getWorkerPort();
    let chats = document.getElementById("pinnedchats");
    ok(port, "provider has a port");
    ok(chats.menupopup.parentNode.collapsed, "popup nub collapsed at start");
    port.postMessage({topic: "test-init"});
    
    
    let maxToOpen = 20;
    let numOpened = 0;
    let maybeOpenAnother = function() {
      if (numOpened++ >= maxToOpen) {
        ok(false, "We didn't find a collapsed chat after " + maxToOpen + "chats!");
        closeAllChats();
        next();
      }
      port.postMessage({topic: "test-chatbox-open", data: { id: numOpened }});
    }
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-chatbox-message":
          if (!chats.menupopup.parentNode.collapsed) {
            maybeOpenAnother();
            break;
          }
          ok(true, "popup nub became visible");
          
          while (chats.selectedChat) {
            chats.selectedChat.close();
          }
          ok(!chats.selectedChat, "chats are all closed");
          port.close();
          next();
          break;
      }
    }
    maybeOpenAnother();
  },
  testWorkerChatWindow: function(next) {
    const chatUrl = SocialSidebar.provider.origin + "/browser/browser/base/content/test/social/social_chat.html";
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-chatbox-message":
          ok(true, "got a chat window opened");
          ok(chats.selectedChat, "chatbox from worker opened");
          while (chats.selectedChat) {
            chats.selectedChat.close();
          }
          ok(!chats.selectedChat, "chats are all closed");
          gURLsNotRemembered.push(chatUrl);
          port.close();
          next();
          break;
      }
    }
    ok(!chats.selectedChat, "chats are all closed");
    port.postMessage({topic: "test-worker-chat", data: chatUrl});
  },
  testCloseSelf: function(next) {
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "got-chatbox-visibility":
          is(e.data.result, "shown", "chatbox shown");
          port.close(); 
          let chat = chats.selectedChat;
          ok(chat.parentNode, "chat has a parent node before it is closed");
          
          let doc = chat.contentDocument;
          let evt = doc.createEvent("CustomEvent");
          evt.initCustomEvent("socialTest-CloseSelf", true, true, {});
          doc.documentElement.dispatchEvent(evt);
          ok(!chat.parentNode, "chat is now closed");
          port.close();
          next();
          break;
      }
    }
    port.postMessage({topic: "test-init", data: { id: 1 }});
  },
  testSameChatCallbacks: function(next) {
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    let seen_opened = false;
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "chatbox-opened":
          is(e.data.result, "ok", "the sidebar says it got a chatbox");
          if (seen_opened) {
            
            
            let chats = document.getElementById("pinnedchats");
            chats.selectedChat.close();
            is(chats.selectedChat, null, "should only have been one chat open");
            port.close();
            next();
          } else {
            
            
            seen_opened = true;
            port.postMessage({topic: "test-chatbox-open"});
          }
      }
    }
    port.postMessage({topic: "test-init", data: { id: 1 }});
  },

  
  testRemoveAll: function(next, mode) {
    let port = SocialSidebar.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    get3ChatsForCollapsing(mode || "normal", function() {
      let chatbar = window.SocialChatBar.chatbar;
      chatbar.removeAll();
      
      is(chatbar.childNodes.length, 0, "should be no chats left");
      checkPopup();
      is(chatbar.selectedChat, null, "nothing should be selected");
      is(chatbar.chatboxForURL.size, 0, "chatboxForURL map should be empty");
      port.close();
      next();
    });
  },

  testRemoveAllMinimized: function(next) {
    this.testRemoveAll(next, "minimized");
  },

  
  testCloseOnlyVisible: function(next) {
    let chatbar = window.SocialChatBar.chatbar;
    let chatWidth = undefined;
    let num = 0;
    is(chatbar.childNodes.length, 0, "chatbar starting empty");
    is(chatbar.menupopup.childNodes.length, 0, "popup starting empty");

    makeChat("normal", "first chat", function() {
      
      checkPopup();
      ok(chatbar.menupopup.parentNode.collapsed, "menu selection isn't visible");
      
      
      chatWidth = chatbar.calcTotalWidthOf(chatbar.selectedChat);
      let desired = chatWidth * 1.5;
      resizeWindowToChatAreaWidth(desired, function(sizedOk) {
        ok(sizedOk, "can't do any tests without this width");
        checkPopup();
        makeChat("normal", "second chat", function() {
          is(chatbar.childNodes.length, 2, "now have 2 chats");
          let first = chatbar.childNodes[0];
          let second = chatbar.childNodes[1];
          is(chatbar.selectedChat, first, "first chat is selected");
          ok(second.collapsed, "second chat is currently collapsed");
          
          
          chatbar.selectedChat.close();
          is(chatbar.selectedChat, second, "second chat is selected");
          closeAllChats();
          next();
        });
      });
    });
  },

  testShowWhenCollapsed: function(next) {
    let port = SocialSidebar.provider.getWorkerPort();
    port.postMessage({topic: "test-init"});
    get3ChatsForCollapsing("normal", function(first, second, third) {
      let chatbar = window.SocialChatBar.chatbar;
      chatbar.showChat(first);
      ok(!first.collapsed, "first should no longer be collapsed");
      ok(second.collapsed ||  third.collapsed, false, "one of the others should be collapsed");
      closeAllChats();
      port.close();
      next();
    });
  },

  testOnlyOneCallback: function(next) {
    let chats = document.getElementById("pinnedchats");
    let port = SocialSidebar.provider.getWorkerPort();
    let numOpened = 0;
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "chatbox-opened":
          numOpened += 1;
          port.postMessage({topic: "ping"});
          break;
        case "pong":
          executeSoon(function() {
            is(numOpened, 1, "only got one open message");
            chats.removeAll();
            port.close();
            next();
          });
      }
    }
    port.postMessage({topic: "test-init", data: { id: 1 }});
  },

  testSecondTopLevelWindow: function(next) {
    
    const chatUrl = SocialSidebar.provider.origin + "/browser/browser/base/content/test/social/social_chat.html";
    let port = SocialSidebar.provider.getWorkerPort();
    let secondWindow;
    port.onmessage = function(e) {
      if (e.data.topic == "test-init-done") {
        secondWindow = OpenBrowserWindow();
        secondWindow.addEventListener("load", function loadListener() {
          secondWindow.removeEventListener("load", loadListener);
          port.postMessage({topic: "test-worker-chat", data: chatUrl});
        });
      } else if (e.data.topic == "got-chatbox-message") {
        
        is(secondWindow.SocialChatBar.chatbar.childElementCount, 1);
        secondWindow.close();
        next();
      }
    }
    port.postMessage({topic: "test-init"});
  },

  testChatWindowChooser: function(next) {
    
    
    
    ok(!windowHasChats(window), "first window should start with no chats");
    openChat(SocialSidebar.provider, function() {
      ok(windowHasChats(window), "first window has the chat");
      
      
      let secondWindow = OpenBrowserWindow();
      secondWindow.addEventListener("load", function loadListener() {
        secondWindow.removeEventListener("load", loadListener);
        ok(!windowHasChats(secondWindow), "second window has no chats");
        openChat(SocialSidebar.provider, function() {
          ok(windowHasChats(secondWindow), "second window now has chats");
          is(window.SocialChatBar.chatbar.childElementCount, 1, "first window still has 1 chat");
          window.SocialChatBar.chatbar.removeAll();
          
          openChat(SocialSidebar.provider, function() {
            ok(!windowHasChats(window), "first window has no chats");
            ok(windowHasChats(secondWindow), "second window has a chat");

            
            
            waitForFocus(function() {
              openChat(SocialSidebar.provider, function() {
                ok(windowHasChats(window), "first window has chats");
                window.SocialChatBar.chatbar.removeAll();
                ok(!windowHasChats(window), "first window has no chats");

                let privateWindow = OpenBrowserWindow({private: true});
                privateWindow.addEventListener("load", function loadListener() {
                  privateWindow.removeEventListener("load", loadListener);

                  
                  
                  
                  
                  openChat(SocialSidebar.provider, function() {
                    let os = Services.appinfo.OS;
                    const BROKEN_WM_Z_ORDER = os != "WINNT" && os != "Darwin";
                    let fn = BROKEN_WM_Z_ORDER ? todo : ok;
                    fn(windowHasChats(window), "first window has a chat");
                    window.SocialChatBar.chatbar.removeAll();

                    privateWindow.close();
                    secondWindow.close();
                    next();
                  });
                });
              });
            });
            window.focus();
          });
        });
      })
    });
  },
  testMultipleProviderChat: function(next) {
    
    openChat(Social.providers[0], function() {
      openChat(Social.providers[1], function() {
        openChat(Social.providers[2], function() {
          let chats = document.getElementById("pinnedchats");
          waitForCondition(function() chats.children.length == Social.providers.length,
            function() {
              ok(true, "one chat window per provider opened");
              
              let provider = Social.providers[2];
              let port = provider.getWorkerPort();
              port.postMessage({topic: "test-logout"});
              waitForCondition(function() chats.children.length == Social.providers.length - 1,
                function() {
                  chats.removeAll();
                  waitForCondition(function() chats.children.length == 0,
                                   function() {
                                    ok(!chats.selectedChat, "multiprovider chats are all closed");
                                    port.close();
                                    next();
                                   },
                                   "chat windows didn't close");
                },
                "chat window didn't close");
            }, "chat windows did not open");
        });
      });
    });
  },

  
  
  testCloseOnLogout: function(next) {
    const chatUrl = SocialSidebar.provider.origin + "/browser/browser/base/content/test/social/social_chat.html";
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    let opened = false;
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-init-done":
          info("open first chat window");
          port.postMessage({topic: "test-worker-chat", data: chatUrl});
          break;
        case "got-chatbox-message":
          ok(true, "got a chat window opened");
          if (opened) {
            port.postMessage({topic: "test-logout"});
            waitForCondition(function() document.getElementById("pinnedchats").firstChild == null,
                             function() {
                              port.close();
                              next();
                             },
                             "chat windows didn't close");
          } else {
            
            opened = true;
            port.postMessage({topic: "test-worker-chat", data: chatUrl+"?id=1"});
          }
          break;
      }
    }
    
    
    
    port.postMessage({topic: "test-set-profile"});
    port.postMessage({topic: "test-init"});
  }
}
