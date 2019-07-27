



XPCOMUtils.defineLazyModuleGetter(this, "Chat",
                                  "resource:///modules/Chat.jsm");
let openChatOrig = Chat.open;

const contact = {
  name: [ "Mr Smith" ],
  email: [{
    type: "home",
    value: "fakeEmail",
    pref: true
  }]
};

add_task(function test_startDirectCall_opens_window() {
  let openedUrl;
  Chat.open = function(contentWindow, origin, title, url) {
    openedUrl = url;
    return 1;
  };

  LoopCalls.startDirectCall(contact, "audio-video");

  do_check_true(!!openedUrl, "should open a chat window");

  
  let windowId = openedUrl.match(/about:loopconversation\#(\d+)$/)[1];
  LoopCalls.clearCallInProgress(windowId);
});

add_task(function test_startDirectCall_getConversationWindowData() {
  let openedUrl;
  Chat.open = function(contentWindow, origin, title, url) {
    openedUrl = url;
    return 2;
  };

  LoopCalls.startDirectCall(contact, "audio-video");

  let windowId = openedUrl.match(/about:loopconversation\#(\d+)$/)[1];

  let callData = MozLoopService.getConversationWindowData(windowId);

  do_check_eq(callData.callType, "audio-video", "should have the correct call type");
  do_check_eq(callData.contact, contact, "should have the contact details");

  
  LoopCalls.clearCallInProgress(windowId);
});

add_task(function test_startDirectCall_not_busy_if_window_fails_to_open() {
  let openedUrl;

  
  Chat.open = function(contentWindow, origin, title, url) {
    openedUrl = url;
    return null;
  };

  LoopCalls.startDirectCall(contact, "audio-video");

  do_check_true(!!openedUrl, "should have attempted to open chat window");

  openedUrl = null;

  
  Chat.open = function(contentWindow, origin, title, url) {
    openedUrl = url;
    return 3;
  };

  LoopCalls.startDirectCall(contact, "audio-video");

  do_check_true(!!openedUrl, "should open a chat window");

  
  let windowId = openedUrl.match(/about:loopconversation\#(\d+)$/)[1];
  LoopCalls.clearCallInProgress(windowId);
});

function run_test() {
  do_register_cleanup(function() {
    
    Chat.open = openChatOrig;
  });

  run_next_test();
}
