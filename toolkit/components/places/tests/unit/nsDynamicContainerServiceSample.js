






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function RemoteContainerSampleService() {
}

RemoteContainerSampleService.prototype = {

  get bms() {
    if (!this._bms)
      this._bms = Cc[BMS_CONTRACTID].getService(Ci.nsINavBookmarksService);
    return this._bms;
  },

  get history() {
    if (!this._history)
      this._history = Cc[NH_CONTRACTID].getService(Ci.nsINavHistoryService);
    return this._history;
  },

  
  get ios() {
    if (!this._ios)
      this._ios = Cc["@mozilla.org/network/io-service;1"].
                   getService(Ci.nsIIOService);
    return this._ios;
  },

  
  onContainerRemoving: function(container) { },

  onContainerMoved: function() { },

  onContainerNodeOpening: function(container, options) {
    container.appendURINode("http://foo.tld/", "uri 1", 0, 0, null);

    var folder = Cc["@mozilla.org/browser/annotation-service;1"].
                 getService(Ci.nsIAnnotationService).
                 getItemAnnotation(container.itemId, "exposedFolder");

    container.appendFolderNode(folder);
  },

  onContainerNodeClosed: function() { },

  createInstance: function LS_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(iid);
  },

  classDescription: "Remote Container Sample Service",
  contractID: "@mozilla.org/browser/remote-container-sample;1",
  classID: Components.ID("{0d42adc5-f07a-4da2-b8da-3e2ef114cb67}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDynamicContainer]),
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([RemoteContainerSampleService]);
}
