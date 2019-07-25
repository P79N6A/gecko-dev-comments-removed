


let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

var WebAppRT = {
  init: function() {
    this.deck = document.getElementById("browsers");
    this.deck.addEventListener("click", onContentClick, false, true);
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
