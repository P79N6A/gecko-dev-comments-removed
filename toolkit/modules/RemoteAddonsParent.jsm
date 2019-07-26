




this.EXPORTED_SYMBOLS = ["RemoteAddonsParent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');

let RemoteAddonsParent = {
  initialized: false,

  init: function() {
    if (this.initialized)
      return;

    this.initialized = true;
  },
};
