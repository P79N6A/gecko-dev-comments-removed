



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "ContentLinkHandler" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Feeds",
  "resource:///modules/Feeds.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
  "resource://gre/modules/BrowserUtils.jsm");

this.ContentLinkHandler = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener("DOMLinkAdded", (event) => {
      this.onLinkAdded(event, chromeGlobal);
    }, false);
  },

  onLinkAdded: function(event, chromeGlobal) {
    var link = event.originalTarget;
    var rel = link.rel && link.rel.toLowerCase();
    if (!link || !link.ownerDocument || !rel || !link.href)
      return;

    var feedAdded = false;
    var iconAdded = false;
    var searchAdded = false;
    var rels = {};
    for (let relString of rel.split(/\s+/))
      rels[relString] = true;

    for (let relVal in rels) {
      switch (relVal) {
        case "feed":
        case "alternate":
          if (!feedAdded) {
            if (!rels.feed && rels.alternate && rels.stylesheet)
              break;

            if (Feeds.isValidFeed(link, link.ownerDocument.nodePrincipal, "feed" in rels)) {
              chromeGlobal.sendAsyncMessage("Link:AddFeed",
                                            {type: link.type,
                                             href: link.href,
                                             title: link.title});
              feedAdded = true;
            }
          }
          break;
        case "icon":
          if (!iconAdded) {
            if (!Services.prefs.getBoolPref("browser.chrome.site_icons"))
              break;

            var uri = this.getLinkIconURI(link);
            if (!uri)
              break;

            [iconAdded] = chromeGlobal.sendSyncMessage("Link:AddIcon", {url: uri.spec});
          }
          break;
        case "search":
          if (!searchAdded) {
            var type = link.type && link.type.toLowerCase();
            type = type.replace(/^\s+|\s*(?:;.*)?$/g, "");

            let re = /^(?:https?|ftp):/i;
            if (type == "application/opensearchdescription+xml" && link.title &&
                re.test(link.href))
            {
              let engine = { title: link.title, href: link.href };
              chromeGlobal.sendAsyncMessage("Link:AddSearch",
                                            {engine: engine,
                                             url: link.ownerDocument.documentURI});
              searchAdded = true;
            }
          }
          break;
      }
    }
  },

  getLinkIconURI: function(aLink) {
    let targetDoc = aLink.ownerDocument;
    var uri = BrowserUtils.makeURI(aLink.href, targetDoc.characterSet);

    
    
    
    var isAllowedPage = [
      /^about:neterror\?/,
      /^about:blocked\?/,
      /^about:certerror\?/,
      /^about:home$/,
    ].some(function (re) re.test(targetDoc.documentURI));

    if (!isAllowedPage || !uri.schemeIs("chrome")) {
      var ssm = Services.scriptSecurityManager;
      try {
        ssm.checkLoadURIWithPrincipal(targetDoc.nodePrincipal, uri,
                                      Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
      } catch(e) {
        return null;
      }
    }

    try {
      var contentPolicy = Cc["@mozilla.org/layout/content-policy;1"].
                          getService(Ci.nsIContentPolicy);
    } catch(e) {
      return null; 
    }

    
    if (contentPolicy.shouldLoad(Ci.nsIContentPolicy.TYPE_IMAGE,
                                 uri, targetDoc.documentURIObject,
                                 aLink, aLink.type, null)
                                 != Ci.nsIContentPolicy.ACCEPT)
      return null;

    try {
      uri.userPass = "";
    } catch(e) {
      
    }
    return uri;
  },
};
