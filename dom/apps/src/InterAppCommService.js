



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/InterAppCommService.jsm");

function InterAppCommServiceProxy() {
}

InterAppCommServiceProxy.prototype = {
  registerConnection: function(aKeyword, aHandlerPageURI, aManifestURI,
                               aDescription, aRules) {
    InterAppCommService.
      registerConnection(aKeyword, aHandlerPageURI, aManifestURI,
                         aDescription, aRules);
  },

  classID: Components.ID("{3dd15ce6-e7be-11e2-82bc-77967e7a63e6}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterAppCommService])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([InterAppCommServiceProxy]);
