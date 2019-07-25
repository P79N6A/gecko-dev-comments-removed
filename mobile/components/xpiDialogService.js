



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function XPInstallDialogService() { }

XPInstallDialogService.prototype = {
  classDescription: "XPInstall Dialog Service",
  contractID: "@mozilla.org/embedui/xpinstall-dialog-service;1",
  classID: Components.ID("{c1242012-27d8-477e-a0f1-0b098ffc329b}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXPIDialogService, Ci.nsIXPIProgressDialog]),

  confirmInstall: function(aParent, aPackages, aCount) {
    
    return true;
  },

  openProgressDialog: function(aPackages, aCount, aManager) {
    
    let dpb = Cc["@mozilla.org/embedcomp/dialogparam;1"].createInstance(Ci.nsIDialogParamBlock);
    dpb.SetInt(0, 2);                       
    dpb.SetInt(1, aPackages.length);        
    dpb.SetNumberStrings(aPackages.length); 
    for (let i = 0; i < aPackages.length; ++i)
      dpb.SetString(i, aPackages[i]);

    let dpbWrap = Cc["@mozilla.org/supports-interface-pointer;1"].createInstance(Ci.nsISupportsInterfacePointer);
    dpbWrap.data = dpb;
    dpbWrap.dataIID = Ci.nsIDialogParamBlock;

    let obsWrap = Cc["@mozilla.org/supports-interface-pointer;1"].createInstance(Ci.nsISupportsInterfacePointer);
    obsWrap.data = aManager;
    obsWrap.dataIID = Ci.nsIObserver;

    let params = Cc["@mozilla.org/supports-array;1"].createInstance(Ci.nsISupportsArray);
    params.AppendElement(dpbWrap);
    params.AppendElement(obsWrap);

    let os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.notifyObservers(params, "xpinstall-download-started", null);

    
    aManager.observe(this, "xpinstall-progress", "open");
  },

  onStateChange: function(aIndex, aState, aError) { },
  onProgress: function(aIndex, aValue, aMax) { }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([XPInstallDialogService]);
}
