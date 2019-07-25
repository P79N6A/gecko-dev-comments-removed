












































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const CHILD_SCRIPT = "chrome://specialpowers/content/specialpowers.js"




function SpecialPowersException(aMsg) {
  this.message = aMsg;
  this.name = "SpecialPowersException";
}

SpecialPowersException.prototype.toString = function() {
  return this.name + ': "' + this.message + '"';
};


function SpecialPowersObserver() {}

SpecialPowersObserver.prototype = {
  classDescription: "Special powers Observer for use in testing.",
  classID:          Components.ID("{59a52458-13e0-4d93-9d85-a637344f29a1}"),
  contractID:       "@mozilla.org/special-powers-observer;1",
  QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.nsIObserver]),
  _xpcom_categories: [{category: "profile-after-change", service: true }],

  observe: function(aSubject, aTopic, aData)
  {
    if (aTopic == "profile-after-change") {
      this.init();
    } else if (aTopic == "content-document-global-created") {
      var w = aSubject.wrappedJSObject;

      if (w) {
        var messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                             getService(Ci.nsIChromeFrameMessageManager);
        
        messageManager.addMessageListener("SPPrefService", this);

        
        messageManager.loadFrameScript(CHILD_SCRIPT, true);
        
      } else {
        dump("TEST-INFO | specialpowers | Can't attach special powers to window " + aSubject + "\n");
      }
    } else if (aTopic == "xpcom-shutdown") {
      this.uninit();
    }
  },

  init: function()
  {
    var obs = Services.obs;
    obs.addObserver(this, "xpcom-shutdown", false);
    obs.addObserver(this, "content-document-global-created", false);
  },

  uninit: function()
  {
    var obs = Services.obs;
    obs.removeObserver(this, "content-document-global-created"); 
  },
  
  



  receiveMessage: function(aMessage) {
    switch(aMessage.name) {
      case "SPPrefService":
        var prefs = Services.prefs;
        var prefType = aMessage.json.prefType.toUpperCase();
        var prefName = aMessage.json.prefName;
        var prefValue = aMessage.json.prefValue ? aMessage.json.prefValue : null;

        if (aMessage.json.op == "get") {
          if (!prefName || !prefType)
            throw new SpecialPowersException("Invalid parameters for get in SPPrefService");
        } else if (aMessage.json.op == "set") {
          if (!prefName || !prefType  || !prefValue)
            throw new SpecialPowersException("Invalid parameters for set in SPPrefService");
        } else {
          throw new SpecialPowersException("Invalid operation for SPPrefService");
        }
        
        switch(prefType) {
          case "BOOL":
            if (aMessage.json.op == "get")
              return(prefs.getBoolPref(prefName));
            else 
              return(prefs.setBoolPref(prefName, prefValue));
          case "INT":
            if (aMessage.json.op == "get") 
              return(prefs.getIntPref(prefName));
            else
              return(prefs.setIntPref(prefName, prefValue));
          case "CHAR":
            if (aMessage.json.op == "get")
              return(prefs.getCharPref(prefName));
            else
              return(prefs.setCharPref(prefName, prefValue));
          case "COMPLEX":
            if (aMessage.json.op == "get")
              return(prefs.getComplexValue(prefName, prefValue[0]));
            else
              return(prefs.setComplexValue(prefName, prefValue[0], prefValue[1]));
        }
        break;
      default:
        throw new SpecialPowersException("Unrecognized Special Powers API");
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SpecialPowersObserver]);
