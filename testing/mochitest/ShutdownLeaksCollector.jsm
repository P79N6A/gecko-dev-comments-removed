



var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["ContentCollector"];






var ContentCollector = {
  init: function() {
      let processType = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).processType;
      if (processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
        
        return;
      }

    let cpmm = Cc["@mozilla.org/childprocessmessagemanager;1"]
                 .getService(Ci.nsISyncMessageSender);
    cpmm.addMessageListener("browser-test:collect-request", this);
  },

  receiveMessage: function(aMessage) {
    switch (aMessage.name) {
      case "browser-test:collect-request":
        Services.obs.notifyObservers(null, "memory-pressure", "heap-minimize");

        Cu.forceGC();
        Cu.forceCC();

        Cu.schedulePreciseShrinkingGC(() => {
          Cu.forceCC();

          let pid = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).processID;
          dump("Completed ShutdownLeaks collections in process " + pid + "\n")});

          let cpmm = Cc["@mozilla.org/childprocessmessagemanager;1"]
                       .getService(Ci.nsISyncMessageSender);
          cpmm.removeMessageListener("browser-test:collect-request", this);

        break;
    }
  }

};
ContentCollector.init();
