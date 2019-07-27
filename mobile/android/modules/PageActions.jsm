



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

this.EXPORTED_SYMBOLS = ["PageActions"];



function resolveGeckoURI(aURI) {
  if (!aURI)
    throw "Can't resolve an empty uri";

  if (aURI.startsWith("chrome://")) {
    let registry = Cc['@mozilla.org/chrome/chrome-registry;1'].getService(Ci["nsIChromeRegistry"]);
    return registry.convertChromeURL(Services.io.newURI(aURI, null, null)).spec;
  } else if (aURI.startsWith("resource://")) {
    let handler = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);
    return handler.resolveURI(Services.io.newURI(aURI, null, null));
  }
  return aURI;
}


var PageActions = {
  _items: { },

  _inited: false,

  _maybeInit: function() {
    if (!this._inited && Object.keys(this._items).length > 0) {
      this._inited = true;
      Services.obs.addObserver(this, "PageActions:Clicked", false);
      Services.obs.addObserver(this, "PageActions:LongClicked", false);
    }
  },

  _maybeUninit: function() {
    if (this._inited && Object.keys(this._items).length == 0) {
      this._inited = false;
      Services.obs.removeObserver(this, "PageActions:Clicked");
      Services.obs.removeObserver(this, "PageActions:LongClicked");
    }
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "PageActions:Clicked") {
      if (this._items[aData].clickCallback) {
        this._items[aData].clickCallback();
      }
    } else if (aTopic == "PageActions:LongClicked") {
      if (this._items[aData].longClickCallback) {
        this._items[aData].longClickCallback();
      }
    }
  },

  add: function(aOptions) {
    let id = uuidgen.generateUUID().toString();
    sendMessageToJava({
      type: "PageActions:Add",
      id: id,
      title: aOptions.title,
      icon: resolveGeckoURI(aOptions.icon),
      important: "important" in aOptions ? aOptions.important : false
    });

    this._items[id] = {
      clickCallback: aOptions.clickCallback,
      longClickCallback: aOptions.longClickCallback
    };

    this._maybeInit();
    return id;
  },

  remove: function(id) {
    sendMessageToJava({
      type: "PageActions:Remove",
      id: id
    });

    delete this._items[id];
    this._maybeUninit();
  }
}
