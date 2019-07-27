



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
  };

  LoopCalls.startDirectCall(contact, "audio-video");

  let windowId = openedUrl.match(/about:loopconversation\#(\d+)$/)[1];

  let callData = MozLoopService.getConversationWindowData(windowId);

  do_check_eq(callData.callType, "audio-video", "should have the correct call type");
  do_check_eq(callData.contact, contact, "should have the contact details");

  
  LoopCalls.clearCallInProgress(windowId);
});

function run_test() {
  do_register_cleanup(function() {
    
    Chat.open = openChatOrig;
  });

  run_next_test();
}
