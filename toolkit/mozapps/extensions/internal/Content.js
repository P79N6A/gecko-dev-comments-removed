



"use strict";

(function() {

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

let nsIFile = Components.Constructor("@mozilla.org/file/local;1", "nsIFile",
                                     "initWithPath");

const MSG_JAR_FLUSH = "AddonJarFlush";


try {
  if (Services.appinfo.processType !== Services.appinfo.PROCESS_TYPE_DEFAULT) {
  
    addMessageListener(MSG_JAR_FLUSH, function jar_flushMessageListener(message) {
      let file = new nsIFile(message.data);
      Services.obs.notifyObservers(file, "flush-cache-entry", null);
    });
  }
} catch(e) {
  Cu.reportError(e);
}

})();
