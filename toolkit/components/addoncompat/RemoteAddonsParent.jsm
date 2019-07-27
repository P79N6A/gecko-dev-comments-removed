



this.EXPORTED_SYMBOLS = ["RemoteAddonsParent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');



function setDefault(dict, key, default_)
{
  if (key in dict) {
    return dict[key];
  }
  dict[key] = default_;
  return default_;
}







let NotificationTracker = {
  
  
  
  
  
  
  _paths: {},

  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:GetNotifications", this);
  },

  add: function(path) {
    let tracked = this._paths;
    for (let component of path) {
      tracked = setDefault(tracked, component, {});
    }
    let count = tracked._count || 0;
    count++;
    tracked._count = count;

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.broadcastAsyncMessage("Addons:AddNotification", path);
  },

  remove: function(path) {
    let tracked = this._paths;
    for (let component of path) {
      tracked = setDefault(tracked, component, {});
    }
    tracked._count--;

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.broadcastAsyncMessage("Addons:RemoveNotification", path);
  },

  receiveMessage: function(msg) {
    if (msg.name == "Addons:GetNotifications") {
      return this._paths;
    }
  }
};
NotificationTracker.init();





function Interposition(base)
{
  if (base) {
    this.methods = Object.create(base.methods);
    this.getters = Object.create(base.getters);
    this.setters = Object.create(base.setters);
  } else {
    this.methods = {};
    this.getters = {};
    this.setters = {};
  }
}




let ContentPolicyParent = {
  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:ContentPolicy:Run", this);

    this._policies = [];
  },

  addContentPolicy: function(cid) {
    this._policies.push(cid);
    NotificationTracker.add(["content-policy"]);
  },

  removeContentPolicy: function(cid) {
    let index = this._policies.lastIndexOf(cid);
    if (index > -1) {
      this._policies.splice(index, 1);
    }

    NotificationTracker.remove(["content-policy"]);
  },

  receiveMessage: function (aMessage) {
    switch (aMessage.name) {
      case "Addons:ContentPolicy:Run":
        return this.shouldLoad(aMessage.data, aMessage.objects);
        break;
    }
  },

  shouldLoad: function(aData, aObjects) {
    for (let policyCID of this._policies) {
      let policy = Cc[policyCID].getService(Ci.nsIContentPolicy);
      try {
        let result = policy.shouldLoad(aObjects.contentType,
                                       aObjects.contentLocation,
                                       aObjects.requestOrigin,
                                       aObjects.node,
                                       aObjects.mimeTypeGuess,
                                       null);
        if (result != Ci.nsIContentPolicy.ACCEPT && result != 0)
          return result;
      } catch (e) {
        Cu.reportError(e);
      }
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },
};
ContentPolicyParent.init();



let CategoryManagerInterposition = new Interposition();

CategoryManagerInterposition.methods.addCategoryEntry =
  function(addon, target, category, entry, value, persist, replace) {
    if (category == "content-policy") {
      ContentPolicyParent.addContentPolicy(entry);
    }

    target.addCategoryEntry(category, entry, value, persist, replace);
  };

CategoryManagerInterposition.methods.deleteCategoryEntry =
  function(addon, target, category, entry, persist) {
    if (category == "content-policy") {
      ContentPolicyParent.remoteContentPolicy(entry);
    }

    target.deleteCategoryEntry(category, entry, persist);
  };

let RemoteAddonsParent = {
  init: function() {
  },

  getInterfaceInterpositions: function() {
    let result = {};

    function register(intf, interp) {
      result[intf.number] = interp;
    }

    register(Ci.nsICategoryManager, CategoryManagerInterposition);

    return result;
  },

  getTaggedInterpositions: function() {
    return {};
  },
};
