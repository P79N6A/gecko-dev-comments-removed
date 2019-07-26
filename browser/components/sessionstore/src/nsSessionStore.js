















const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/sessionstore/SessionStore.jsm");

function SessionStoreService() {}



Object.keys(SessionStore).forEach(function (aName) {
  let desc = Object.getOwnPropertyDescriptor(SessionStore, aName);
  Object.defineProperty(SessionStoreService.prototype, aName, desc);
});

SessionStoreService.prototype.classID =
  Components.ID("{5280606b-2510-4fe0-97ef-9b5a22eafe6b}");
SessionStoreService.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.nsISessionStore]);

let NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStoreService]);
