





const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

do_get_profile();

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesTestUtils",
                                  "resource://testing-common/PlacesTestUtils.jsm");

var downloadUtils = { };
XPCOMUtils.defineLazyServiceGetter(downloadUtils,
                                   "downloadManager",
                                   "@mozilla.org/download-manager;1",
                                   Ci.nsIDownloadManager);

function createURI(aObj)
{
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                        ios.newURI(aObj, null, null);
}

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

var provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == "DLoads") {
      var file = dirSvc.get("ProfD", Ci.nsILocalFile);
      file.append("downloads.rdf");
      return file;
     }
    print("*** Throwing trying to get " + prop);
    throw Cr.NS_ERROR_FAILURE;
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);

var gDownloadCount = 0;












function addDownload(server, aParams)
{
  if (!server)
    do_throw("Must provide a valid server.");
  const PORT = server.identity.primaryPort;
  if (!aParams)
    aParams = {};
  if (!("resultFileName" in aParams))
    aParams.resultFileName = "download.result";
  if (!("targetFile" in aParams)) {
    aParams.targetFile = dirSvc.get("ProfD", Ci.nsIFile);
    aParams.targetFile.append(aParams.resultFileName);
  }
  if (!("sourceURI" in aParams))
    aParams.sourceURI = "http://localhost:" + PORT + "/head_download_manager.js";
  if (!("downloadName" in aParams))
    aParams.downloadName = null;
  if (!("runBeforeStart" in aParams))
    aParams.runBeforeStart = function () {};

  const nsIWBP = Ci.nsIWebBrowserPersist;
  var persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                .createInstance(Ci.nsIWebBrowserPersist);
  persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                         nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                         nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

  
  gDownloadCount++;

  let dm = downloadUtils.downloadManager;
  var dl = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI(aParams.sourceURI),
                          createURI(aParams.targetFile), aParams.downloadName, null,
                          Math.round(Date.now() * 1000), null, persist, aParams.isPrivate);

  
  
  if (!aParams.isPrivate)
    var test = dm.getDownload(dl.id);

  aParams.runBeforeStart.call(undefined, dl);

  persist.progressListener = dl.QueryInterface(Ci.nsIWebProgressListener);
  persist.savePrivacyAwareURI(dl.source, null, null, 0, null, null, dl.targetFile,
                              aParams.isPrivate);

  return dl;
}

function getDownloadListener()
{
  return {
    onDownloadStateChange: function(aState, aDownload)
    {
      if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_QUEUED)
        do_test_pending();

      if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED ||
          aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_CANCELED ||
          aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FAILED) {
          gDownloadCount--;
        do_test_finished();
      }

      if (gDownloadCount == 0 && typeof httpserv != "undefined" && httpserv)
      {
        do_test_pending();
        httpserv.stop(do_test_finished);
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };
}

XPCOMUtils.defineLazyGetter(this, "Services", function() {
  Cu.import("resource://gre/modules/Services.jsm");
  return Services;
});


Services.prefs.setBoolPref("browser.download.manager.showAlertOnComplete", false);

do_register_cleanup(function() {
  Services.obs.notifyObservers(null, "quit-application", null);
});

function oldDownloadManagerDisabled() {
  try {
    
    Services.downloads.activeDownloadCount;
  } catch (ex) {
    return true;
  }
  return false;
}
