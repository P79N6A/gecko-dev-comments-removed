



"use strict";

this.EXPORTED_SYMBOLS = [ "Feeds" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");

const Ci = Components.interfaces;

this.Feeds = {

  










  isValidFeed: function(aLink, aPrincipal, aIsFeed) {
    if (!aLink || !aPrincipal)
      return false;

    var type = aLink.type.toLowerCase().replace(/^\s+|\s*(?:;.*)?$/g, "");
    if (!aIsFeed) {
      aIsFeed = (type == "application/rss+xml" ||
                 type == "application/atom+xml");
    }

    if (aIsFeed) {
      
      let principalURI = BrowserUtils.makeURIFromCPOW(aPrincipal.URI);
      let principalToCheck = Services.scriptSecurityManager.getNoAppCodebasePrincipal(principalURI);
      try {
        BrowserUtils.urlSecurityCheck(aLink.href, principalToCheck,
                                      Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
        return type || "application/rss+xml";
      }
      catch(ex) {
      }
    }

    return null;
  },

};
