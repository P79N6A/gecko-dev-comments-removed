





Components.utils.import("resource://gre/modules/Promise.jsm", this);
const CHAT_URL = "https://example.com/browser/browser/base/content/test/chat/chat.html";


function isTabFocused() {
  let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  
  let elt = Services.focus.getFocusedElementForWindow(window, false, {});
  return elt == tabb;
}


function isChatFocused(chat) {
  
  let elt = Services.focus.getFocusedElementForWindow(window, false, {});
  return elt == chat.content;
}

let chatbar = document.getElementById("pinnedchats");

function* setUp() {
  
  
  
  
  let html = '<input id="theinput"><button id="chat-opener"></button>';
  let url = "data:text/html;charset=utf-8," + encodeURI(html);
  let tab = gBrowser.selectedTab = gBrowser.addTab(url, {skipAnimation: true});
  yield promiseOneEvent(tab.linkedBrowser, "load", true);
  tab.linkedBrowser.contentDocument.getElementById("theinput").focus();
  registerCleanupFunction(function() {
    gBrowser.removeTab(tab);
  });
}


add_chat_task(function* testDefaultFocus() {
  yield setUp();
  let chat = yield promiseOpenChat("http://example.com");
  
  
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");
  Assert.ok(isTabFocused(), "the tab should remain focused.");
  Assert.ok(!isChatFocused(chat), "the chat should not be focused.");
});


add_chat_task(function* testDefaultFocus() {
  yield setUp();
  let tab = gBrowser.selectedTab;
  let deferred = Promise.defer();
  let button = tab.linkedBrowser.contentDocument.getElementById("chat-opener");
  button.addEventListener("click", function onclick() {
    button.removeEventListener("click", onclick);
    promiseOpenChat("http://example.com").then(
      chat => deferred.resolve(chat)
    );
  })
  
  
  
  EventUtils.synthesizeMouseAtCenter(button, {}, button.ownerDocument.defaultView);
  let chat = yield deferred.promise;

  
  
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");
  Assert.ok(!isTabFocused(), "the tab should have lost focus.");
  Assert.ok(isChatFocused(chat), "the chat should have got focus.");
});


add_chat_task(function* testExplicitFocus() {
  yield setUp();
  let chat = yield promiseOpenChat("http://example.com", undefined, true);
  
  
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");
  Assert.ok(!isTabFocused(), "the tab should have lost focus.");
  Assert.ok(isChatFocused(chat), "the chat should have got focus.");
});




add_chat_task(function* testNoFocusOnAutoRestore() {
  yield setUp();
  let chat = yield promiseOpenChat("http://example.com", "minimized");
  Assert.ok(chat.minimized, "chat is minimized");
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");
  Assert.ok(isTabFocused(), "the tab should remain focused.");
  Assert.ok(!isChatFocused(chat), "the chat should not be focused.");
  yield promiseOpenChat("http://example.com");
  Assert.ok(!chat.minimized, "chat should be restored");
  Assert.ok(isTabFocused(), "the tab should remain focused.");
  Assert.ok(!isChatFocused(chat), "the chat should not be focused.");
});



add_chat_task(function* testFocusOnExplicitRestore() {
  yield setUp();
  let chat = yield promiseOpenChat("http://example.com");
  Assert.ok(!chat.minimized, "chat should have been opened restored");
  Assert.ok(isTabFocused(), "the tab should remain focused.");
  Assert.ok(!isChatFocused(chat), "the chat should not be focused.");
  chat.minimized = true;
  Assert.ok(isTabFocused(), "tab should still be focused");
  Assert.ok(!isChatFocused(chat), "the chat should not be focused.");

  let promise = promiseOneEvent(chat.contentWindow, "focus");
  
  chat.onTitlebarClick({button: 0});
  yield promise; 
  Assert.ok(!chat.minimized, "chat should have been restored");
  Assert.ok(isChatFocused(chat), "chat should be focused");
  Assert.strictEqual(chat, chatbar.selectedChat, "chat is marked selected");
});



add_chat_task(function* testMinimizeFocused() {
  yield setUp();
  let chat1 = yield promiseOpenChat("http://example.com#1");
  let chat2 = yield promiseOpenChat("http://example.com#2");
  Assert.equal(numChatsInWindow(window), 2, "2 chats open");
  Assert.strictEqual(chatbar.selectedChat, chat2, "chat2 is selected");
  let promise = promiseOneEvent(chat1.contentWindow, "focus");
  chatbar.selectedChat = chat1;
  chatbar.focus();
  yield promise; 
  Assert.strictEqual(chat1, chatbar.selectedChat, "chat1 is marked selected");
  Assert.notStrictEqual(chat2, chatbar.selectedChat, "chat2 is not marked selected");
  promise = promiseOneEvent(chat2.contentWindow, "focus");
  chat1.minimized = true;
  yield promise; 
  Assert.notStrictEqual(chat1, chatbar.selectedChat, "chat1 is not marked selected");
  Assert.strictEqual(chat2, chatbar.selectedChat, "chat2 is marked selected");
});




add_chat_task(function* testTab() {
  yield setUp();

  function sendTabAndWaitForFocus(chat, eltid) {
    let doc = chat.contentDocument;
    EventUtils.sendKey("tab");
    
    
    
    
    let deferred = Promise.defer();
    let tries = 0;
    let interval = setInterval(function() {
      if (tries >= 30) {
        clearInterval(interval);
        deferred.reject("never got focus");
        return;
      }
      tries ++;
      let elt = eltid ? doc.getElementById(eltid) : doc.documentElement;
      if (doc.activeElement == elt) {
        clearInterval(interval);
        deferred.resolve();
      }
      info("retrying wait for focus: " + tries);
      info("(the active element is " + doc.activeElement + "/" + doc.activeElement.getAttribute("id") + ")");
    }, 100);
    info("waiting for element " + eltid + " to get focus");
    return deferred.promise;
  }

  let chat1 = yield promiseOpenChat(CHAT_URL + "#1");
  let chat2 = yield promiseOpenChat(CHAT_URL + "#2");
  chatbar.selectedChat = chat2;
  let promise = promiseOneEvent(chat2.contentWindow, "focus");
  chatbar.focus();
  info("waiting for second chat to get focus");
  yield promise;

  
  
  yield sendTabAndWaitForFocus(chat2, "input1");
  Assert.equal(chat2.contentDocument.activeElement.getAttribute("id"), "input1",
               "first input field has focus");
  Assert.ok(isChatFocused(chat2), "new chat still focused after first tab");

  yield sendTabAndWaitForFocus(chat2, "input2");
  Assert.ok(isChatFocused(chat2), "new chat still focused after tab");
  Assert.equal(chat2.contentDocument.activeElement.getAttribute("id"), "input2",
               "second input field has focus");

  yield sendTabAndWaitForFocus(chat2, "iframe");
  Assert.ok(isChatFocused(chat2), "new chat still focused after tab");
  Assert.equal(chat2.contentDocument.activeElement.getAttribute("id"), "iframe",
               "iframe has focus");

  
  
  yield sendTabAndWaitForFocus(chat1, null);
  Assert.ok(isChatFocused(chat1), "first chat is focused");
});




add_chat_task(function* testFocusedElement() {
  yield setUp();

  
  let chat = yield promiseOpenChat(CHAT_URL, undefined, true);

  chat.contentDocument.getElementById("input2").focus();

  
  let tabb = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  let promise = promiseOneEvent(tabb.contentWindow, "focus");
  Services.focus.moveFocus(tabb.contentWindow, null, Services.focus.MOVEFOCUS_ROOT, 0);
  yield promise;

  promise = promiseOneEvent(chat.contentWindow, "focus");
  chatbar.focus();
  yield promise;

  Assert.equal(chat.contentDocument.activeElement.getAttribute("id"), "input2",
               "correct input field still has focus");
});
