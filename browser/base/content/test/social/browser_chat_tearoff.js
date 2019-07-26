



function test() {
  requestLongerTimeout(2); 
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png"
  };

  let postSubTest = function(cb) {
    let chats = document.getElementById("pinnedchats");
    ok(chats.children.length == 0, "no chatty children left behind");
    cb();
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    SocialSidebar.show();
    ok(SocialSidebar.provider, "sidebar provider exists");
    runSocialTests(tests, undefined, postSubTest, function() {
      finishcb();
    });
  });
}

var tests = {
  testTearoffChat: function(next) {
    let chats = document.getElementById("pinnedchats");
    let chatTitle;
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "got-chatbox-visibility":
          
          
          let doc = chats.selectedChat.contentDocument;
          
          
          
          if (doc.location == "about:blank")
            return;
          chatTitle = doc.title;
          ok(chats.selectedChat.getAttribute("label") == chatTitle,
             "the new chatbox should show the title of the chat window");
          let div = doc.createElement("div");
          div.setAttribute("id", "testdiv");
          div.setAttribute("test", "1");
          doc.body.appendChild(div);
          let swap = document.getAnonymousElementByAttribute(chats.selectedChat, "anonid", "swap");
          swap.click();
          port.close();
          break;
        case "got-chatbox-message":
          ok(true, "got chatbox message");
          ok(e.data.result == "ok", "got chatbox windowRef result: "+e.data.result);
          chats.selectedChat.toggle();
          break;
      }
    }

    Services.wm.addListener({
      onWindowTitleChange: function() {},
      onCloseWindow: function(xulwindow) {},
      onOpenWindow: function(xulwindow) {
        var domwindow = xulwindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                              .getInterface(Components.interfaces.nsIDOMWindow);
        Services.wm.removeListener(this);
        
        domwindow.addEventListener("load", function _load(event) {
          let doc = domwindow.document;
          if (event.target != doc)
              return;

          domwindow.removeEventListener("load", _load, false);

          domwindow.addEventListener("unload", function _close(event) {
            if (event.target != doc)
              return;
            domwindow.removeEventListener("unload", _close, false);
            info("window has been closed");
            waitForCondition(function() {
              return chats.selectedChat && chats.selectedChat.contentDocument &&
                     chats.selectedChat.contentDocument.readyState == "complete";
            },function () {
              ok(chats.selectedChat, "should have a chatbox in our window again");
              ok(chats.selectedChat.getAttribute("label") == chatTitle,
                 "the new chatbox should show the title of the chat window again");
              let testdiv = chats.selectedChat.contentDocument.getElementById("testdiv");
              is(testdiv.getAttribute("test"), "2", "docshell should have been swapped");
              chats.selectedChat.close();
              waitForCondition(function() {
                return chats.children.length == 0;
              },function () {
                next();
              });
            });
          }, false);

          is(doc.documentElement.getAttribute("windowtype"), "Social:Chat", "Social:Chat window opened");
          
          
          
          let chatbox = doc.getElementById("chatter");
          waitForCondition(function() {
            return chats.selectedChat == null &&
                   chatbox.contentDocument &&
                   chatbox.contentDocument.readyState == "complete";
          },function() {
            ok(chatbox.getAttribute("label") == chatTitle,
               "detached window should show the title of the chat window");
            let testdiv = chatbox.contentDocument.getElementById("testdiv");
            is(testdiv.getAttribute("test"), "1", "docshell should have been swapped");
            testdiv.setAttribute("test", "2");
            
            let swap = doc.getAnonymousElementByAttribute(chatbox, "anonid", "swap");
            swap.click();
          }, domwindow);
        }, false);
      }
    });

    port.postMessage({topic: "test-init", data: { id: 1 }});
  },

  testCloseOnLogout: function(next) {
    let chats = document.getElementById("pinnedchats");
    const chatUrl = "https://example.com/browser/browser/base/content/test/social/social_chat.html";
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-chatbox-visibility":
          
          
          let doc = chats.selectedChat.contentDocument;
          
          
          
          if (doc.location == "about:blank")
            return;
          info("chatbox is open, detach from window");
          let swap = document.getAnonymousElementByAttribute(chats.selectedChat, "anonid", "swap");
          swap.click();
          break;
      }
    }

    Services.wm.addListener({
      onWindowTitleChange: function() {},
      onCloseWindow: function(xulwindow) {},
      onOpenWindow: function(xulwindow) {
        let domwindow = xulwindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                              .getInterface(Components.interfaces.nsIDOMWindow);
        Services.wm.removeListener(this);
        
        
        domwindow.addEventListener("load", function _load(event) {
          let doc = domwindow.document;
          if (event.target != doc)
              return;
          domwindow.removeEventListener("load", _load, false);

          domwindow.addEventListener("unload", function _close(event) {
            if (event.target != doc)
              return;
            domwindow.removeEventListener("unload", _close, false);
            ok(true, "window has been closed");
            next();
          }, false);

          is(doc.documentElement.getAttribute("windowtype"), "Social:Chat", "Social:Chat window opened");
          
          
          
          let chatbox = doc.getElementById("chatter");
          waitForCondition(function() {
            return chats.children.length == 0 &&
                   chatbox.contentDocument &&
                   chatbox.contentDocument.readyState == "complete";
          },function() {
            
            port.postMessage({topic: "test-logout"});
            port.close();
          }, domwindow);

        }, false);
      }
    });

    port.postMessage({topic: "test-worker-chat", data: chatUrl});
  },

  testReattachTwice: function(next) {
    let chats = document.getElementById("pinnedchats");
    const chatUrl = "https://example.com/browser/browser/base/content/test/social/social_chat.html";
    let chatBoxCount = 0, reattachCount = 0;
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-chatbox-visibility":
          
          
          let doc = chats.selectedChat.contentDocument;
          
          
          
          if (doc.location == "about:blank")
            return;
          if (++chatBoxCount != 2) {
            
            port.postMessage({topic: "test-worker-chat", data: chatUrl + "?id=2"});
            return;
          }
          info("chatbox is open, detach from window");
          let chat1 = chats.firstChild;
          let chat2 = chat1.nextSibling;
          document.getAnonymousElementByAttribute(chat1, "anonid", "swap").click();
          document.getAnonymousElementByAttribute(chat2, "anonid", "swap").click();
          break;
      }
    };

    let firstChatWindowDoc;
    Services.wm.addListener({
      onWindowTitleChange: function() {},
      onCloseWindow: function(xulwindow) {},
      onOpenWindow: function(xulwindow) {
        let listener = this;
        let domwindow = xulwindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                 .getInterface(Components.interfaces.nsIDOMWindow);
        
        
        domwindow.addEventListener("load", function _load(event) {
          let doc = domwindow.document;
          if (event.target != doc)
            return;
          domwindow.removeEventListener("load", _load, false);

          domwindow.addEventListener("unload", function _close(event) {
            if (event.target != doc)
              return;
            domwindow.removeEventListener("unload", _close, false);
            ok(true, "window has been closed");
            waitForCondition(function() {
              return chats.selectedChat && chats.selectedChat.contentDocument &&
                     chats.selectedChat.contentDocument.readyState == "complete";
            }, function () {
              ++reattachCount;
              if (reattachCount == 1) {
                info("reattaching second chat window");
                let chatbox = firstChatWindowDoc.getElementById("chatter");
                firstChatWindowDoc.getAnonymousElementByAttribute(chatbox, "anonid", "swap").click();
                firstChatWindowDoc = null;
              }
              else if (reattachCount == 2) {
                is(chats.children.length, 2, "both chat windows should be reattached");
                chats.removeAll();
                waitForCondition(() => chats.children.length == 0, function () {
                  info("no chat window left");
                  is(chats.chatboxForURL.size, 0, "chatboxForURL map should be empty");
                  next();
                });
              }
            }, "waited too long for the window to reattach");
          }, false);

          is(doc.documentElement.getAttribute("windowtype"), "Social:Chat", "Social:Chat window opened");
          if (!firstChatWindowDoc) {
            firstChatWindowDoc = doc;
            return;
          }
          Services.wm.removeListener(listener);

          
          
          
          let chatbox = doc.getElementById("chatter");
          waitForCondition(function() {
            return chats.children.length == 0 &&
                   chatbox.contentDocument &&
                   chatbox.contentDocument.readyState == "complete";
          },function() {
            info("reattaching chat window");
            doc.getAnonymousElementByAttribute(chatbox, "anonid", "swap").click();
          }, "waited too long for the chat window to be detached");

        }, false);
      }
    });

    port.postMessage({topic: "test-worker-chat", data: chatUrl + "?id=1"});
  }
};
