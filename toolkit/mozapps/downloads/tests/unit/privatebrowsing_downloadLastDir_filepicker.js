





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function FilePickerService() {
  let fileptr = Cc["@mozilla.org/supports-interface-pointer;1"].
                createInstance(Ci.nsISupportsInterfacePointer);
  this._obs.notifyObservers(fileptr, "TEST_FILEPICKER_GETFILE", "");
  this.file = fileptr.data.QueryInterface(fileptr.dataIID);
}

FilePickerService.prototype = {
  _obs: Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService),
  classDescription: "File Picker Test Service",
  contractID: "@mozilla.org/filepicker;1",
  classID: Components.ID("{a070e4b7-65f3-45b3-96dd-1b682ff18588}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFilePicker]),

  
  modeOpen: 0,
  modeSave: 1,
  modeGetFolder: 2,
  modeOpenMultiple: 3,
  returnOK: 0,
  returnCancel: 1,
  returnReplace: 2,
  filterAll: 1,
  filterHTML: 2,
  filterText: 4,
  filterImages: 8,
  filterXML: 16,
  filterXUL: 32,
  filterApps: 64,

  
  defaultExtension: "",
  defaultString: "",
  get displayDirectory() { return null; },
  set displayDirectory(val) {
    this._obs.notifyObservers(val, "TEST_FILEPICKER_SETDISPLAYDIRECTORY", "");
  },
  file: null,
  get files() { return null; },
  get fileURL() { return null; },
  filterIndex: 0,

  
  appendFilter: function() {},
  appendFilters: function() {},
  init: function() {},
  show: function() {
    return this.returnOK;
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([FilePickerService]);
