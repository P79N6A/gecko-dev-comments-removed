



let chatbar = document.getElementById("pinnedchats");

add_chat_task(function* testOpenCloseChat() {
  let chatbox = yield promiseOpenChat("http://example.com");
  Assert.strictEqual(chatbox, chatbar.selectedChat);
  
  Assert.ok(!chatbox.minimized, "chat is not minimized");
  Assert.equal(chatbar.childNodes.length, 1, "should be 1 chat open");


  
  let chatbox2 = yield promiseOpenChat("http://example.com");
  Assert.strictEqual(chatbox2, chatbox, "got the same chat");
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");

  chatbox.toggle();
  is(chatbox.minimized, true, "chat is now minimized");
  
  is(chatbar.selectedChat, null);

  
  let promiseClosed = promiseOneEvent(chatbox.content, "unload", true);
  chatbox.close();
  yield promiseClosed;
});




add_chat_task(function* testMinimized() {
  let chatbox = yield promiseOpenChat("http://example.com", "minimized");
  Assert.strictEqual(chatbox, chatbar.selectedChat);
  Assert.ok(chatbox.minimized, "chat is minimized");
  Assert.equal(numChatsInWindow(window), 1, "should be 1 chat open");
  yield promiseOpenChat("http://example.com");
  Assert.ok(!chatbox.minimized, false, "chat is no longer minimized");
});



add_chat_task(function* testManyChats() {
  Assert.ok(chatbar.menupopup.parentNode.collapsed, "popup nub collapsed at start");
  
  
  let maxToOpen = 20;
  let numOpened = 0;
  for (let i = 0; i < maxToOpen; i++) {
    yield promiseOpenChat("http://example.com#" + i);
    if (!chatbar.menupopup.parentNode.collapsed) {
      info("the menu popup appeared");
      return;
    }
  }
  Assert.ok(false, "We didn't find a collapsed chat after " + maxToOpen + "chats!");
});


add_chat_task(function* testOpenTwiceCallbacks() {
  yield promiseOpenChat("http://example.com#1");
  yield promiseOpenChat("http://example.com#2");
  yield promiseOpenChat("http://test2.example.com");
  Assert.equal(numChatsInWindow(window), 3, "should be 3 chats open");
  Chat.closeAll("http://example.com");
  Assert.equal(numChatsInWindow(window), 1, "should have closed 2 chats");
  Chat.closeAll("http://test2.example.com");
  Assert.equal(numChatsInWindow(window), 0, "should have closed last chat");
});



add_chat_task(function* testOpenTwiceCallbacks() {
  yield promiseOpenChatCallback("http://example.com");
  yield promiseOpenChatCallback("http://example.com");
});


add_chat_task(function* testSecondTopLevelWindow() {
  const chatUrl = "http://example.com";
  let secondWindow = OpenBrowserWindow();
  yield promiseOneEvent(secondWindow, "load");
  yield promiseOpenChat(chatUrl);
  
  Assert.equal(numChatsInWindow(window), 0, "main window has no chats");
  Assert.equal(numChatsInWindow(secondWindow), 1, "second window has 1 chat");
  secondWindow.close();
});


add_chat_task(function* testChatWindowChooser() {
  let chat = yield promiseOpenChat("http://example.com");
  Assert.equal(numChatsInWindow(window), 1, "first window has the chat");
  
  
  let secondWindow = OpenBrowserWindow();
  yield promiseOneEvent(secondWindow, "load");
  Assert.equal(numChatsInWindow(secondWindow), 0, "second window starts with no chats");
  yield promiseOpenChat("http://example.com#2");
  Assert.equal(numChatsInWindow(secondWindow), 1, "second window now has chats");
  Assert.equal(numChatsInWindow(window), 1, "first window still has 1 chat");
  chat.close();
  Assert.equal(numChatsInWindow(window), 0, "first window now has no chats");
  
  yield promiseOpenChat("http://example.com#3");
  Assert.equal(numChatsInWindow(window), 0, "first window still has no chats");
  Assert.equal(numChatsInWindow(secondWindow), 2, "second window has both chats");

  
  
  window.focus();
  yield promiseWaitForFocus();
  chat = yield promiseOpenChat("http://example.com#4");
  Assert.equal(numChatsInWindow(window), 1, "first window got new chat");
  chat.close();
  Assert.equal(numChatsInWindow(window), 0, "first window has no chats");

  let privateWindow = OpenBrowserWindow({private: true});
  yield promiseOneEvent(privateWindow, "load")

  
  
  
  
  chat = yield promiseOpenChat("http://example.com#5");
  let os = Services.appinfo.OS;
  const BROKEN_WM_Z_ORDER = os != "WINNT" && os != "Darwin";
  let fn = BROKEN_WM_Z_ORDER ? todo : ok;
  fn(numChatsInWindow(window) == 1, "first window got the chat");
  chat.close();
  privateWindow.close();
  secondWindow.close();
});
