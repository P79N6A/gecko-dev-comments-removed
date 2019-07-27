



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Prefetcher",
                                  "resource://gre/modules/Prefetcher.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RemoteAddonsParent",
                                  "resource://gre/modules/RemoteAddonsParent.jsm");
















































function AddonInterpositionService()
{
  Prefetcher.init();
  RemoteAddonsParent.init();

  
  
  this._interfaceInterpositions = RemoteAddonsParent.getInterfaceInterpositions();
  this._taggedInterpositions = RemoteAddonsParent.getTaggedInterpositions();

  let wl = [];
  for (let v in this._interfaceInterpositions) {
    let interp = this._interfaceInterpositions[v];
    wl.push(...Object.getOwnPropertyNames(interp.methods));
    wl.push(...Object.getOwnPropertyNames(interp.getters));
    wl.push(...Object.getOwnPropertyNames(interp.setters));
  }

  for (let v in this._taggedInterpositions) {
    let interp = this._taggedInterpositions[v];
    wl.push(...Object.getOwnPropertyNames(interp.methods));
    wl.push(...Object.getOwnPropertyNames(interp.getters));
    wl.push(...Object.getOwnPropertyNames(interp.setters));
  }

  let nameSet = new Set();
  wl = wl.filter(function(item) {
    if (nameSet.has(item))
      return true;

    nameSet.add(item);
    return true;
  });

  this._whitelist = wl;
}

AddonInterpositionService.prototype = {
  classID: Components.ID("{1363d5f0-d95e-11e3-9c1a-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonInterposition, Ci.nsISupportsWeakReference]),

  getWhitelist: function() {
    return this._whitelist;
  },

  
  
  getObjectTag: function(target) {
    if (Cu.isCrossProcessWrapper(target)) {
      return Cu.getCrossProcessWrapperTag(target);
    }

    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if (target instanceof Ci.nsIDOMXULElement) {
      if (target.localName == "browser" && target.isRemoteBrowser) {
        return "RemoteBrowserElement";
      }

      if (target.localName == "tabbrowser") {
        return "TabBrowserElement";
      }
    }

    if (target instanceof Ci.nsIDOMChromeWindow && target.gMultiProcessBrowser) {
      return "ChromeWindow";
    }

    if (target instanceof Ci.nsIDOMEventTarget) {
      return "EventTarget";
    }

    return "generic";
  },

  interposeProperty: function(addon, target, iid, prop) {
    let interp;
    if (iid) {
      interp = this._interfaceInterpositions[iid];
    } else {
      try {
        interp = this._taggedInterpositions[this.getObjectTag(target)];
      }
      catch (e) {
        Cu.reportError(new Components.Exception("Failed to interpose object", e.result, Components.stack.caller));
      }
    }

    if (!interp) {
      return Prefetcher.lookupInCache(addon, target, prop);
    }

    let desc = { configurable: false, enumerable: true };

    if ("methods" in interp && prop in interp.methods) {
      desc.writable = false;
      desc.value = function(...args) {
        return interp.methods[prop](addon, target, ...args);
      }

      return desc;
    } else if ("getters" in interp && prop in interp.getters) {
      desc.get = function() { return interp.getters[prop](addon, target); };

      if ("setters" in interp && prop in interp.setters) {
        desc.set = function(v) { return interp.setters[prop](addon, target, v); };
      }

      return desc;
    }

    return Prefetcher.lookupInCache(addon, target, prop);
  },

  interposeCall: function(addonId, originalFunc, originalThis, args) {
    args.splice(0, 0, addonId);
    return originalFunc.apply(originalThis, args);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AddonInterpositionService]);
