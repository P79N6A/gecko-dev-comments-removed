



function test() {
  requestLongerTimeout(2); 
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
  };

  let postSubTest = function(cb) {
    let chats = document.getElementById("pinnedchats");
    ok(chats.children.length == 0, "no chatty children left behind");
    cb();
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, postSubTest, function() {
      finishcb();
    });
  });
}

var tests = {
  testTearoffChat: function(next) {
    let chats = document.getElementById("pinnedchats");
    let port = Social.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-sidebar-message":
          port.postMessage({topic: "test-chatbox-open"});
          break;
        case "got-chatbox-visibility":
          
          
          let doc = chats.selectedChat.contentDocument;
          let div = doc.createElement("div");
          div.setAttribute("id", "testdiv");
          div.setAttribute("test", "1");
          doc.body.appendChild(div);
          let swap = document.getAnonymousElementByAttribute(chats.selectedChat, "anonid", "swap");
          swap.click();
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
      onCloseWindow: function(xulwindow) {
        Services.wm.removeListener(this);
        info("window has been closed");
        waitForCondition(function() {
          return chats.selectedChat && chats.selectedChat.contentDocument &&
                 chats.selectedChat.contentDocument.readyState == "complete";
        },function () {
          ok(chats.selectedChat, "should have a chatbox in our window again");
          let testdiv = chats.selectedChat.contentDocument.getElementById("testdiv");
          is(testdiv.getAttribute("test"), "2", "docshell should have been swapped");
          chats.selectedChat.close();
          next();
        });
      },
      onOpenWindow: function(xulwindow) {
        var domwindow = xulwindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                              .getInterface(Components.interfaces.nsIDOMWindow);
        
        domwindow.addEventListener("load", function _load() {
          domwindow.removeEventListener("load", _load, false);
          let doc = domwindow.document;
          is(doc.documentElement.getAttribute("windowtype"), "Social:Chat", "Social:Chat window opened");
          is(doc.location.href, "chrome://browser/content/chatWindow.xul", "Should have seen the right window open");
          
          
          
          let chatbox = doc.getElementById("chatter");
          waitForCondition(function() {
            return chatbox.contentDocument &&
                   chatbox.contentDocument.readyState == "complete";
          },function() {
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
  }
}