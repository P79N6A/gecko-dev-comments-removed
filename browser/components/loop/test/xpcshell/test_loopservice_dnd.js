



XPCOMUtils.defineLazyModuleGetter(this, "Chat",
                                  "resource:///modules/Chat.jsm");
let openChatOrig = Chat.open;

add_test(function test_get_do_not_disturb() {
  Services.prefs.setBoolPref("loop.do_not_disturb", false);

  do_check_false(MozLoopService.doNotDisturb);

  Services.prefs.setBoolPref("loop.do_not_disturb", true);

  do_check_true(MozLoopService.doNotDisturb);

  run_next_test();
});

add_test(function test_set_do_not_disturb() {
  Services.prefs.setBoolPref("loop.do_not_disturb", false);

  MozLoopService.doNotDisturb = true;

  do_check_true(Services.prefs.getBoolPref("loop.do_not_disturb"));

  run_next_test();
});

add_test(function test_do_not_disturb_disabled_should_open_chat_window() {
  MozLoopService.doNotDisturb = false;

  MozLoopService.promiseRegisteredWithServers().then(() => {
    let opened = false;
    Chat.open = function() {
      opened = true;
    };

    mockPushHandler.notify(1, MozLoopService.channelIDs.callsGuest);

    waitForCondition(function() opened).then(() => {
      run_next_test();
    }, () => {
      do_throw("should have opened a chat window");
    });
  });
});

add_test(function test_do_not_disturb_enabled_shouldnt_open_chat_window() {
  MozLoopService.doNotDisturb = true;

  
  let opened = false;
  Chat.open = function() {
    opened = true;
  };

  mockPushHandler.notify(1, MozLoopService.channelIDs.callsGuest);

  do_timeout(500, function() {
    do_check_false(opened, "should not open a chat window");
    run_next_test();
  });
});

function run_test() {
  setupFakeLoopServer();

  loopServer.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });
  loopServer.registerPathHandler("/calls", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.write(JSON.stringify({calls: [{callId: 4444333221, websocketToken: "0deadbeef0"}]}));
    response.processAsync();
    response.finish();
  });

  do_register_cleanup(function() {
    
    Chat.open = openChatOrig;

    
    Services.prefs.clearUserPref("loop.do_not_disturb");
  });

  run_next_test();
}
