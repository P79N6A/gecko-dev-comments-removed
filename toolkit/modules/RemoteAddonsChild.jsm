




this.EXPORTED_SYMBOLS = ["RemoteAddonsChild"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');

let RemoteAddonsChild = {
  initialized: false,

  init: function(aContentGlobal) {
    if (this.initialized)
      return;

    this.initialized = true;
  },
};
