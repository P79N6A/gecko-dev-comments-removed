



"use strict";

this.EXPORTED_SYMBOLS = ["E10SUtils"];

const {interfaces: Ci, utils: Cu, classes: Cc} = Components;

Cu.import("resource://gre/modules/Services.jsm");

this.E10SUtils = {
  shouldBrowserBeRemote: function(aURL) {
    
    if (!aURL)
      aURL = "about:blank";

    if (aURL.startsWith("about:") &&
        aURL.toLowerCase() != "about:home" &&
        aURL.toLowerCase() != "about:blank" &&
        !aURL.toLowerCase().startsWith("about:neterror") &&
        !aURL.toLowerCase().startsWith("about:certerror")) {
      return false;
    }

    return true;
  },

  shouldLoadURI: function(aDocShell, aURI, aReferrer) {
    
    if (aURI.spec == "about:blank")
      return true;

    
    if (aDocShell.QueryInterface(Ci.nsIDocShellTreeItem).sameTypeParent)
      return true;

    
    let isRemote = Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;
    if (this.shouldBrowserBeRemote(aURI.spec) == isRemote)
      return true;

    return false;
  },

  redirectLoad: function(aDocShell, aURI, aReferrer) {
    
    let messageManager = aDocShell.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIContentFrameMessageManager);
    let sessionHistory = aDocShell.getInterface(Ci.nsIWebNavigation).sessionHistory;

    messageManager.sendAsyncMessage("Browser:LoadURI", {
      loadOptions: {
        uri: aURI.spec,
        flags: Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
        referrer: aReferrer ? aReferrer.spec : null,
      },
      historyIndex: sessionHistory.requestedIndex,
    });
    return false;
  },
};
