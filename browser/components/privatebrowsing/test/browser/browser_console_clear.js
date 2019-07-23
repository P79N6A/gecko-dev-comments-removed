







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let consoleService = Cc["@mozilla.org/consoleservice;1"].
                       getService(Ci.nsIConsoleService);
  waitForExplicitFinish();

  let consoleObserver = {
    observe: function (aMessage) {
      if (!aMessage.message)
        this.gotNull = true;
    },
    gotNull: false
  };
  consoleService.registerListener(consoleObserver);

  function messageExists() {
    let out = {};
    consoleService.getMessageArray(out, {});
    let messages = out.value;
    if (!messages)
      messages = [];
    let found = false;
    for (let i = 0; i < messages.length; ++i)
      if (messages[i].message == kTestMessage) {
        found = true;
        break;
      }
    return found;
  }

  const kTestMessage = "Test message from the private browsing test";
  
  consoleService.logStringMessage(kTestMessage);
  ok(!consoleObserver.gotNull, "Console shouldn't be cleared yet");
  ok(messageExists(), "Message should exist before leaving the private mode");

  pb.privateBrowsingEnabled = true;
  ok(!consoleObserver.gotNull, "Console shouldn't be cleared yet");
  ok(messageExists(), "Message should exist after entering the private mode");
  pb.privateBrowsingEnabled = false;

  let timer = Cc["@mozilla.org/timer;1"].
              createInstance(Ci.nsITimer);
  timer.initWithCallback({
    notify: function(timer) {
      
      ok(consoleObserver.gotNull, "Console should be cleared after leaving the private mode");
      
      ok(!messageExists(), "Message should not exist after leaving the private mode");

      consoleService.unregisterListener(consoleObserver);
      prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
      finish();
    }
  }, 1000, Ci.nsITimer.TYPE_ONE_SHOT);
}
