



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const CHILD_SCRIPT = "chrome://quitter/content/contentscript.js";


function QuitterObserver() {}

QuitterObserver.prototype = {
  classDescription: "Quitter Observer for use in testing.",
  classID:          Components.ID("{c235a986-5ac1-4f28-ad73-825dae9bad90}"),
  contractID:       "@mozilla.org/quitter-observer;1",
  QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.nsIObserver]),
  _xpcom_categories: [{category: "profile-after-change", service: true }],
  isFrameScriptLoaded: false,

  observe: function(aSubject, aTopic, aData)
  {
    if (aTopic == "profile-after-change") {
      this.init();
    } else if (!this.isFrameScriptLoaded &&
               aTopic == "chrome-document-global-created") {

      var messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                           getService(Ci.nsIMessageBroadcaster);
      
      messageManager.addMessageListener("Quitter.Quit", this);

      messageManager.loadFrameScript(CHILD_SCRIPT, true);
      this.isFrameScriptLoaded = true;
    } else if (aTopic == "xpcom-shutdown") {
      this.uninit();
    }
  },

  init: function()
  {
    var obs = Services.obs;
    obs.addObserver(this, "xpcom-shutdown", false);
    obs.addObserver(this, "chrome-document-global-created", false);
  },

  uninit: function()
  {
    var obs = Services.obs;
    obs.removeObserver(this, "chrome-document-global-created", false);
  },

  



  receiveMessage: function(aMessage) {
    switch(aMessage.name) {
      case "Quitter.Quit":
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eForceQuit);
        break;
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([QuitterObserver]);
