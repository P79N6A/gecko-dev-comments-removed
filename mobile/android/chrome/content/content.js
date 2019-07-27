




let { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutReader", "resource://gre/modules/AboutReader.jsm");

let dump = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog.d.bind(null, "Content");

let AboutReaderListener = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener("AboutReaderContentLoaded", this, false, true);
  },

  handleEvent: function(event) {
    if (!event.originalTarget.documentURI.startsWith("about:reader")) {
      return;
    }

    switch (event.type) {
      case "AboutReaderContentLoaded":
        
        
        
        
        if (content.document.body) {
          new AboutReader(content.document, content);
        }
        break;
    }
  }
};
AboutReaderListener.init(this);
