



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "RemoteAddonsParent",
                                  "resource://gre/modules/RemoteAddonsParent.jsm");
















































function AddonInterpositionService()
{
  RemoteAddonsParent.init();

  
  
  this._interfaceInterpositions = RemoteAddonsParent.getInterfaceInterpositions();
  this._taggedInterpositions = RemoteAddonsParent.getTaggedInterpositions();
}

AddonInterpositionService.prototype = {
  classID: Components.ID("{1363d5f0-d95e-11e3-9c1a-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonInterposition, Ci.nsISupportsWeakReference]),

  
  
  getObjectTag: function(target) {
    if (Cu.isCrossProcessWrapper(target)) {
      if (target instanceof Ci.nsIDocShellTreeItem) {
        return "ContentDocShellTreeItem";
      }

      if (target instanceof Ci.nsIDOMDocument) {
        return "ContentDocument";
      }
    }

    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if ((target instanceof Ci.nsIDOMXULElement) &&
        target.localName == "browser" &&
        target.namespaceURI == XUL_NS &&
        target.getAttribute("remote") == "true") {
      return "RemoteBrowserElement";
    }

    if (target instanceof Ci.nsIDOMChromeWindow) {
      return "ChromeWindow";
    }

    if (target instanceof Ci.nsIDOMEventTarget) {
      return "EventTarget";
    }

    return "generic";
  },

  interpose: function(addon, target, iid, prop) {
    let interp;
    if (iid) {
      interp = this._interfaceInterpositions[iid];
    } else {
      interp = this._taggedInterpositions[this.getObjectTag(target)];
    }

    if (!interp) {
      return null;
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

    return null;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AddonInterpositionService]);
