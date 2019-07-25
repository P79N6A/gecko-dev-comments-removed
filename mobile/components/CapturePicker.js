





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function CapturePicker() {

}

CapturePicker.prototype = {
  _file: null,
  _mode: -1,
  _result: -1,
  _shown: false,
  _title: "",
  _type: "",
  _window: null,
  _done: null,

  
  
  
  init: function(aWindow, aTitle, aMode) {
    this._window = aWindow;
    this._title = aTitle;
    this._mode = aMode;
  },

  show: function() {
    if (this._shown)
      throw Cr.NS_ERROR_UNEXPECTED;

    this._shown = true;
    this._file = null;
    this._done = false;

    Services.obs.addObserver(this, "cameraCaptureDone", false);

    let msg = { gecko: { type: "onCameraCapture" } };
    Cc["@mozilla.org/android/bridge;1"].getService(Ci.nsIAndroidBridge).handleGeckoMessage(JSON.stringify(msg));

    
    while (!this._done)
      Services.tm.currentThread.processNextEvent(true);

    if (this._res.ok) {
      this._file = this._res.path;
      
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._res.path);
      Cc["@mozilla.org/uriloader/external-helper-app-service;1"].getService(Ci.nsPIExternalAppLauncher).deleteTemporaryFileOnExit(file);
    }

    return (this._res.ok ? Ci.nsICapturePicker.RETURN_OK : Ci.nsICapturePicker.RETURN_CANCEL);
  },

  observe: function(aObject, aTopic, aData) {
    Services.obs.removeObserver(this, "cameraCaptureDone");
    this._done = true;
    this._res = JSON.parse(aData);
  },

  modeMayBeAvailable: function(aMode) {
    if (aMode != Ci.nsICapturePicker.MODE_STILL)
      return false;
    return true;
  },

  get file() {
    if (this._file) { 
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._file);
      let utils = this._window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      return utils.wrapDOMFile(file);
    } else {
      throw Cr.NS_ERROR_FAILURE;
    }
  },

  get type() {
    return this._type;
  },

  set type(aNewType) {
    if (this._shown)
      throw Cr.NS_ERROR_UNEXPECTED;
    else 
      this._type = aNewType;
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICapturePicker, Ci.nsIObserver]),

  
  classID: Components.ID("{cb5a47f0-b58c-4fc3-b61a-358ee95f8238}"),
};

var components = [ CapturePicker ];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
