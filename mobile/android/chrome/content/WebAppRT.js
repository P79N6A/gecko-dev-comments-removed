


let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

function pref(name, value) {
  return {
    name: name,
    value: value
  }
}

var WebAppRT = {
  prefs: [
    
    pref("extensions.enabledScopes", 1),
    
    pref("extensions.autoDisableScopes", 1),
    
    pref("xpinstall.enabled", false),
    
    pref("toolkit.telemetry.prompted", true)
  ],

  init: function(isUpdate) {
    this.deck = document.getElementById("browsers");
    this.deck.addEventListener("click", this, false, true);

    
    if (isUpdate == "new") {
      this.prefs.forEach(function(aPref) {
        switch (typeof aPref.value) {
          case "string":
            Services.prefs.setCharPref(aPref.name, aPref.value);
            break;
          case "boolean":
            Services.prefs.setBoolPref(aPref.name, aPref.value);
            break;
          case "number":
            Services.prefs.setIntPref(aPref.name, aPref.value);
            break;
        }
      });
    }
  },

  handleEvent: function(event) {
    let target = event.target;
  
    if (!(target instanceof HTMLAnchorElement) ||
        target.getAttribute("target") != "_blank") {
      return;
    }
  
    let uri = Services.io.newURI(target.href,
                                 target.ownerDocument.characterSet,
                                 null);
  
    
    Cc["@mozilla.org/uriloader/external-protocol-service;1"].
      getService(Ci.nsIExternalProtocolService).
      getProtocolHandlerInfo(uri.scheme).
      launchWithURI(uri);
  
    
    
    
    event.preventDefault();
  }
}
