



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Ci = Components.interfaces;





function PrivateBrowsingService() {
}

PrivateBrowsingService.prototype = {
  classID: Components.ID("{ba0e4d3d-7be2-41a0-b723-a7c16b22ebe9}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrivateBrowsingService])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PrivateBrowsingService]);
