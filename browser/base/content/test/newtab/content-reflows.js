



(function () {
  "use strict";

  const Ci = Components.interfaces;

  docShell.addWeakReflowObserver({
    reflow() {
      
      let path = (new Error().stack).split("\n").slice(1).join("\n");
      if (path) {
        sendSyncMessage("newtab-reflow", path);
      }
    },

    reflowInterruptible() {
      
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIReflowObserver,
                                           Ci.nsISupportsWeakReference])
  });
})();
