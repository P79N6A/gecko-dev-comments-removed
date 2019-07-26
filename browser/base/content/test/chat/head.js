





let Chat = Cu.import("resource:///modules/Chat.jsm", {}).Chat;

function promiseOpenChat(url, mode, focus) {
  let uri = Services.io.newURI(url, null, null);
  let origin = uri.prePath;
  let title = origin;
  let chatbox = Chat.open(null, origin, title, url, mode, focus);
  return chatbox.promiseChatLoaded;
}


function promiseOpenChatCallback(url, mode) {
  let uri = Services.io.newURI(url, null, null);
  let origin = uri.prePath;
  let title = origin;
  let deferred = Promise.defer();
  let callback = deferred.resolve;
  Chat.open(null, origin, title, url, mode, undefined, callback);
  return deferred.promise;
}



function promiseOneEvent(target, eventName, capture) {
  let deferred = Promise.defer();
  target.addEventListener(eventName, function handler(event) {
    target.removeEventListener(eventName, handler, capture);
    deferred.resolve();
  }, capture);
  return deferred.promise;
}


function numChatsInWindow(win) {
  let chatbar = win.document.getElementById("pinnedchats");
  return chatbar.childElementCount;
}

function promiseWaitForFocus() {
  let deferred = Promise.defer();
  waitForFocus(deferred.resolve);
  return deferred.promise;
}


function add_chat_task(genFunction) {
  add_task(function* () {
    info("Starting chat test " + genFunction.name);
    try {
      yield genFunction();
    } finally {
      info("Finished chat test " + genFunction.name + " - cleaning up.");
      
      while (chatbar.childNodes.length) {
        chatbar.childNodes[0].close();
      }
      
      let winEnum = Services.wm.getEnumerator("Social:Chat");
      while (winEnum.hasMoreElements()) {
        let win = winEnum.getNext();
        if (win.closed) {
          continue;
        }
        win.close();
      }
    }
  });
}
