



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let isParent = Cc["@mozilla.org/xre/runtime;1"]
                 .getService(Ci.nsIXULRuntime)
                 .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

XPCOMUtils.defineLazyGetter(this, "PushService", function() {
  
  const {PushService} = Cu.import("resource://gre/modules/PushService.jsm", {});
  if (isParent) {
    PushService.init();
  }
  return PushService;
});

this.PushNotificationService = function PushNotificationService() {};

PushNotificationService.prototype = {
  classID: Components.ID("{32028e38-903b-4a64-a180-5857eb4cb3dd}"),

  contractID: "@mozilla.org/push/NotificationService;1",

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(PushNotificationService),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIPushNotificationService]),

  _pushEnabled: function _pushEnabled() {
    return Services.prefs.getBoolPref("dom.push.enabled");
  },

  register: function register(scope, pageURL) {
    if (!this._pushEnabled()) {
      return Promise.reject(Cr.NS_ERROR_NOT_AVAILABLE);
    }
    return PushService._register({scope, pageURL});
  },

  unregister: function unregister(scope) {
    if (!this._pushEnabled()) {
      return Promise.reject(Cr.NS_ERROR_NOT_AVAILABLE);
    }
    return PushService._unregister({scope});
  },

  registration: function registration(scope) {
    if (!this._pushEnabled()) {
      return Promise.reject(Cr.NS_ERROR_NOT_AVAILABLE);
    }
    return PushService._registration({scope});
  },

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        Services.obs.addObserver(this, "final-ui-startup", true);
        break;
      case "final-ui-startup":
        Services.obs.removeObserver(this, "final-ui-startup");
        if (!this._pushEnabled()) {
          return;
        }
        if (isParent) {
          PushService.init();
        }
        break;
    }
  }
};

this.PushObserverNotification = function PushObserverNotification() {};

PushObserverNotification.prototype = {
  classID: Components.ID("{66a87970-6dc9-46e0-ac61-adb4a13791de}"),

  contractID: "@mozilla.org/push/ObserverNotification;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPushObserverNotification])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([
  PushNotificationService,
  PushObserverNotification
]);
