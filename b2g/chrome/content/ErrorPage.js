



'use strict';

let Cu = Components.utils;
let Cc = Components.classes;
let Ci = Components.interfaces;

let ErrorPageHandler = {
  _reload: function() {
    docShell.QueryInterface(Ci.nsIWebNavigation).reload(Ci.nsIWebNavigation.LOAD_FLAGS_NONE);
  },

  _certErrorPageEventHandler: function(e) {
    let target = e.originalTarget;
    let errorDoc = target.ownerDocument;

    
    
    if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      let permanent = errorDoc.getElementById("permanentExceptionButton");
      let temp = errorDoc.getElementById("temporaryExceptionButton");
      if (target == temp || target == permanent) {
        sendAsyncMessage("ErrorPage:AddCertException", {
          url: errorDoc.location.href,
          isPermanent: target == permanent
        });
      }
    }
  },

  domContentLoadedHandler: function(e) {
    let target = e.originalTarget;
    let targetDocShell = target.defaultView
                               .QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation);
    if (targetDocShell != docShell) {
      return;
    }

    if (/^about:certerror/.test(target.documentURI)) {
      let errorPageEventHandler = this._certErrorPageEventHandler.bind(this);
      addEventListener("click", errorPageEventHandler, true, false);
      let listener = function() {
        removeEventListener("click", errorPageEventHandler, true);
        removeEventListener("pagehide", listener, true);
      }.bind(this);

      addEventListener("pagehide", listener, true);
    }
  },

  init: function() {
    addMessageListener("ErrorPage:ReloadPage", this._reload.bind(this));
    addEventListener('DOMContentLoaded',
                     this.domContentLoadedHandler.bind(this),
                     true);
  }
};

ErrorPageHandler.init();
