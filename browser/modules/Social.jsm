



"use strict";

let EXPORTED_SYMBOLS = ["Social"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService",
  "resource://gre/modules/SocialService.jsm");

let Social = {
  lastEventReceived: 0,
  provider: null,
  init: function Social_init(callback) {
    if (this.provider) {
      schedule(callback);
      return;
    }

    
    
    SocialService.getProviderList(function (providers) {
      if (providers.length)
        this.provider = providers[0];
      callback();
    }.bind(this));
  },

  get uiVisible() {
    return this.provider && this.provider.enabled && this.provider.port;
  },

  set enabled(val) {
    SocialService.enabled = val;
  },
  get enabled() {
    return SocialService.enabled;
  },

  get active() {
    return Services.prefs.getBoolPref("social.active");
  },
  set active(val) {
    Services.prefs.setBoolPref("social.active", !!val);
    this.enabled = !!val;
  },

  toggle: function Social_toggle() {
    this.enabled = !this.enabled;
  },

  toggleSidebar: function SocialSidebar_toggle() {
    let prefValue = Services.prefs.getBoolPref("social.sidebar.open");
    Services.prefs.setBoolPref("social.sidebar.open", !prefValue);
  },

  sendWorkerMessage: function Social_sendWorkerMessage(message) {
    
    
    if (this.provider && this.provider.port)
      this.provider.port.postMessage(message);
  },

  
  _getShareablePageUrl: function Social_getShareablePageUrl(aURI) {
    let uri = aURI.clone();
    try {
      
      uri.userPass = "";
    } catch (e) {}
    return uri.spec;
  },

  isPageShared: function Social_isPageShared(aURI) {
    let url = this._getShareablePageUrl(aURI);
    return this._sharedUrls.hasOwnProperty(url);
  },

  sharePage: function Social_sharePage(aURI) {
    let url = this._getShareablePageUrl(aURI);
    this._sharedUrls[url] = true;
    this.sendWorkerMessage({
      topic: "social.user-recommend",
      data: { url: url }
    });
  },

  unsharePage: function Social_unsharePage(aURI) {
    let url = this._getShareablePageUrl(aURI);
    delete this._sharedUrls[url];
    this.sendWorkerMessage({
      topic: "social.user-unrecommend",
      data: { url: url }
    });
  },

  _sharedUrls: {}
};

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}
