



"use strict";

this.EXPORTED_SYMBOLS = ["E10SUtils"];

const {interfaces: Ci, utils: Cu, classes: Cc} = Components;

Cu.import("resource://gre/modules/Services.jsm");

function getAboutModule(aURL) {
  
  let moduleName = aURL.path.replace(/[#?].*/, "").toLowerCase();
  let contract = "@mozilla.org/network/protocol/about;1?what=" + moduleName;
  try {
    return Cc[contract].getService(Ci.nsIAboutModule);
  }
  catch (e) {
    
    
    return null;
  }
}

this.E10SUtils = {
  canLoadURIInProcess: function(aURL, aProcess) {
    
    if (!aURL)
      aURL = "about:blank";

    let processIsRemote = aProcess == Ci.nsIXULRuntime.PROCESS_TYPE_CONTENT;

    let canLoadRemote = true;
    let mustLoadRemote = true;

    if (aURL.startsWith("about:")) {
      let url = Services.io.newURI(aURL, null, null);
      let module = getAboutModule(url);
      
      
      if (module) {
        let flags = module.getURIFlags(url);
        canLoadRemote = !!(flags & Ci.nsIAboutModule.URI_CAN_LOAD_IN_CHILD);
        mustLoadRemote = !!(flags & Ci.nsIAboutModule.URI_MUST_LOAD_IN_CHILD);
      }
    }

    if (aURL.startsWith("chrome:")) {
      let url = Services.io.newURI(aURL, null, null);
      let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                      getService(Ci.nsIXULChromeRegistry);
      canLoadRemote = chromeReg.canLoadURLRemotely(url);
      mustLoadRemote = chromeReg.mustLoadURLRemotely(url);
    }

    if (mustLoadRemote)
      return processIsRemote;

    if (!canLoadRemote && processIsRemote)
      return false;

    return true;
  },

  shouldLoadURI: function(aDocShell, aURI, aReferrer) {
    
    if (aDocShell.QueryInterface(Ci.nsIDocShellTreeItem).sameTypeParent)
      return true;

    
    return this.canLoadURIInProcess(aURI.spec, Services.appinfo.processType);
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
