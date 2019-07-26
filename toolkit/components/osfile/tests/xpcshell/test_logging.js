"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");





function run_test() {
  do_test_pending();
  let messageCount = 0;

  
  let consoleListener = {
    observe: function (aMessage) {
      
      if (!(aMessage instanceof Components.interfaces.nsIConsoleMessage)) {
        return;
      }
      if (aMessage.message.indexOf("TEST OS") < 0) {
        return;
      }

      ++messageCount;
      if(messageCount == 1) {
       do_check_eq(aMessage.message, "TEST OS {\"name\":\"test\"}\n");
      }
      if(messageCount == 2) {
        do_check_eq(aMessage.message, "TEST OS name is test\n");
        
        
        do_execute_soon(function() {
          toggleConsoleListener(false);
          do_test_finished();
        });
      }
    }
  };

  
  function toggleConsoleListener (pref) {
    OS.Shared.DEBUG = pref;
    OS.Shared.TEST = pref;
    Services.console[pref ? "registerListener" : "unregisterListener"](
      consoleListener);
  }

  toggleConsoleListener(true);

  let objectDefault = {name: "test"};
  let CustomToString = function() {
    this.name = "test";
  }
  CustomToString.prototype.toString = function() {
    return "name is " + this.name;
  }
  let objectCustom = new CustomToString();
  OS.Shared.LOG(objectDefault);
  OS.Shared.LOG(objectCustom);
  
  
}

