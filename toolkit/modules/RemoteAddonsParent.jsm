




this.EXPORTED_SYMBOLS = ["RemoteAddonsParent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');






let ContentPolicyParent = {
  





  _policiesToIgnore: [],

  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:ContentPolicy:IgnorePolicies", this);
    ppmm.addMessageListener("Addons:ContentPolicy:Run", this);

    Services.obs.addObserver(this, "xpcom-category-entry-added", true);
    Services.obs.addObserver(this, "xpcom-category-entry-removed", true);
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-category-entry-added":
      case "xpcom-category-entry-removed":
        if (aData == "content-policy")
          this.updatePolicies();
        break;
    }
  },

  







  updatePolicies: function() {
    let needHook = false;

    let services = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager)
                                                       .enumerateCategory("content-policy");
    while (services.hasMoreElements()) {
      let item = services.getNext();
      let name = item.QueryInterface(Components.interfaces.nsISupportsCString).toString();

      if (this._policiesToIgnore.indexOf(name) == -1) {
        needHook = true;
        break;
      }
    }

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.broadcastAsyncMessage("Addons:ContentPolicy:NeedHook", { needed: needHook });
  },

  receiveMessage: function (aMessage) {
    switch (aMessage.name) {
      case "Addons:ContentPolicy:IgnorePolicies":
        this._policiesToIgnore = aMessage.data.policies;
        this.updatePolicies();
        break;

      case "Addons:ContentPolicy:Run":
        return this.shouldLoad(aMessage.data, aMessage.objects);
        break;
    }
  },

  shouldLoad: function(aData, aObjects) {
    let services = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager)
                                                       .enumerateCategory("content-policy");
    while (services.hasMoreElements()) {
      let item = services.getNext();
      let name = item.QueryInterface(Components.interfaces.nsISupportsCString).toString();
      if (this._policiesToIgnore.indexOf(name) != -1)
        continue;

      let policy = Cc[name].getService(Ci.nsIContentPolicy);
      try {
        let result = policy.shouldLoad(aData.contentType,
                                       aObjects.contentLocation,
                                       aObjects.requestOrigin,
                                       aObjects.node,
                                       aData.mimeTypeGuess,
                                       null);
        if (result != Ci.nsIContentPolicy.ACCEPT && result != 0)
          return result;
      } catch (e) {}
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),
};

let RemoteAddonsParent = {
  initialized: false,

  init: function() {
    if (this.initialized)
      return;

    this.initialized = true;

    ContentPolicyParent.init();
  },
};
