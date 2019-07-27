



XPCOMUtils.defineLazyModuleGetter(this, "Chat",
                                  "resource:///modules/Chat.jsm");
let openChatOrig = Chat.open;

const loopServiceModule = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});

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

  MozLoopService.register(mockPushHandler).then(() => {
    let opened = false;
    Chat.open = function() {
      opened = true;
    };

    let savedHawkClient = loopServiceModule.gHawkClient;
    loopServiceModule.gHawkClient = {request: hawkGetCallsRequest};

    mockPushHandler.notify(1);

    do_check_true(opened, "should open a chat window");

    loopServiceModule.gHawkClient = savedHawkClient;

    run_next_test();
  });
});

add_task(function test_do_not_disturb_enabled_shouldnt_open_chat_window() {
  MozLoopService.doNotDisturb = true;

  
  let opened = false;
  Chat.open = function() {
    opened = true;
  };

  mockPushHandler.notify(1);

  do_check_false(opened, "should not open a chat window");
});

function run_test()
{
  setupFakeLoopServer();

  loopServer.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });

  do_register_cleanup(function() {
    
    Chat.open = openChatOrig;

    
    Services.prefs.clearUserPref("loop.do_not_disturb");
  });

  run_next_test();
}
