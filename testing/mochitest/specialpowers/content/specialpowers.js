







































function SpecialPowers() {}

var SpecialPowers = {
  sanityCheck: function() { return "foo"; },
  
  
  getBoolPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'BOOL'));
  },
  getIntPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'INT'));
  },
  getCharPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'CHAR'));
  },
  getComplexValue: function(aPrefName, aIid) {
    return (this._getPref(aPrefName, 'COMPLEX', aIid));
  },

  
  setBoolPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'BOOL', aValue));
  },
  setIntPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'INT', aValue));
  },
  setCharPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'CHAR', aValue));
  },
  setComplexValue: function(aPrefName, aIid, aValue) {
    return (this._setPref(aPrefName, 'COMPLEX', aValue, aIid));
  },

  
  _getPref: function(aPrefName, aPrefType, aIid) {
    var msg = {};
    if (aIid) {
      
      msg = {'op':'get', 'prefName': aPrefName, 'prefType':aPrefType, 'prefValue':[aIid]};
    } else {
      msg = {'op':'get', 'prefName': aPrefName,'prefType': aPrefType};
    }
    return(sendSyncMessage('SPPrefService', msg)[0]);
  },
  _setPref: function(aPrefName, aPrefType, aValue, aIid) {
    var msg = {};
    if (aIid) {
      msg = {'op':'set','prefName':aPrefName, 'prefType': aPrefType, 'prefValue': [aIid,aValue]};
    } else {
      msg = {'op':'set', 'prefName': aPrefName, 'prefType': aPrefType, 'prefValue': aValue};
    }
    return(sendSyncMessage('SPPrefService', msg)[0]);
  }
}


function attachToWindow() {
  try {
    if ((content !== null) && 
        (content !== undefined) &&
        (content.wrappedJSObject)) {
      content.wrappedJSObject.SpecialPowers = SpecialPowers;
    }
  } catch(ex) {
    dump("TEST-INFO | specialpowers.js |  Failed to attach specialpowers to window exception: " + ex + "\n");
  }
}











function frameScriptObserver() {
  this.register();
}
frameScriptObserver.prototype = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "content-document-global-created") {
      attachToWindow();
    } 
  },
  register: function() {
    var obsSvc = Components.classes["@mozilla.org/observer-service;1"]
                 .getService(Components.interfaces.nsIObserverService);
    obsSvc.addObserver(this, "content-document-global-created", false);
  },
  unregister: function() {
    var obsSvc = Components.classes["@mozilla.org/observer-service;1"]
                 .getService(Components.interfaces.nsIObserverService);
    obsSvc.removeObserver(this, "content-document-global-created");
  }
};

var xulruntime = Components.classes["@mozilla.org/xre/app-info;1"]
                 .getService(Components.interfaces.nsIXULRuntime);
if (xulruntime.processType == 2) {
  var frameScriptObsv = new frameScriptObserver();
} else {
  attachToWindow();
}
