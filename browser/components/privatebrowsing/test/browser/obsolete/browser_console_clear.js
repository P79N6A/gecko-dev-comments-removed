






function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let consoleService = Cc["@mozilla.org/consoleservice;1"].
                       getService(Ci.nsIConsoleService);
  const EXIT_MESSAGE = "Message to signal the end of the test";
  waitForExplicitFinish();

  let consoleObserver = {
    observe: function (aMessage) {
      if (!aMessage.message)
        this.gotNull = true;
      else if (aMessage.message == EXIT_MESSAGE) {
        
        ok(this.gotNull, "Console should be cleared after leaving the private mode");
        
        ok(!messageExists(), "Message should not exist after leaving the private mode");

        consoleService.unregisterListener(consoleObserver);
        gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
        finish();
      }
    },
    gotNull: false
  };
  consoleService.registerListener(consoleObserver);

  function messageExists() {
    let out = {};
    consoleService.getMessageArray(out, {});
    let messages = out.value || [];
    for (let i = 0; i < messages.length; ++i) {
      if (messages[i].message == TEST_MESSAGE)
        return true;
    }
    return false;
  }

  const TEST_MESSAGE = "Test message from the private browsing test";
  
  consoleService.logStringMessage(TEST_MESSAGE);
  ok(!consoleObserver.gotNull, "Console shouldn't be cleared yet");
  ok(messageExists(), "Message should exist before leaving the private mode");

  pb.privateBrowsingEnabled = true;
  ok(!consoleObserver.gotNull, "Console shouldn't be cleared yet");
  ok(messageExists(), "Message should exist after entering the private mode");
  pb.privateBrowsingEnabled = false;

  
  consoleService.logStringMessage(EXIT_MESSAGE);
}
