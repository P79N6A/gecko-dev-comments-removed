






































do_load_httpd_js();

function createURI(aObj)
{
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                        ios.newURI(aObj, null, null);
}

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get("ProfD", Ci.nsIFile);
} catch (e) { }
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfD") {
        return dirSvc.get("CurProcD", Ci.nsILocalFile);
      } else if (prop == "DLoads") {
        var file = dirSvc.get("CurProcD", Ci.nsILocalFile);
        file.append("downloads.rdf");
        return file;
      }
      print("*** Throwing trying to get " + prop);
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}









function importDownloadsFile(aFName)
{
  var file = do_get_file(aFName);
  var newFile = dirSvc.get("ProfD", Ci.nsIFile);
  if (/\.rdf$/i.test(aFName))
    file.copyTo(newFile, "downloads.rdf");
  else if (/\.sqlite$/i.test(aFName))
    file.copyTo(newFile, "downloads.sqlite");
  else
    do_throw("Unexpected filename!");
}

function cleanup()
{
  
  var rdfFile = dirSvc.get("DLoads", Ci.nsIFile);
  if (rdfFile.exists()) rdfFile.remove(true);

  
  var dbFile = dirSvc.get("ProfD", Ci.nsIFile);
  dbFile.append("downloads.sqlite");
  if (dbFile.exists())
    try { dbFile.remove(true); } catch(e) {  }

  
  
  dbFile = dirSvc.get("ProfD", Ci.nsIFile);
  dbFile.append("places.sqlite");
  if (dbFile.exists())
    try { dbFile.remove(true); } catch(e) {  }

  
  var destFile = dirSvc.get("ProfD", Ci.nsIFile);
  destFile.append("download.result");
  if (destFile.exists()) destFile.remove(true);
}

var gDownloadCount = 0;










function addDownload(aParams)
{
  if (!aParams)
    aParams = {};
  if (!("resultFileName" in aParams))
    aParams.resultFileName = "download.result";
  if (!("targetFile" in aParams)) {
    aParams.targetFile = dirSvc.get("ProfD", Ci.nsIFile);
    aParams.targetFile.append(aParams.resultFileName);
  }
  if (!("sourceURI" in aParams))
    aParams.sourceURI = "http://localhost:4444/res/language.properties";
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

  var dl = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI(aParams.sourceURI),
                          createURI(aParams.targetFile), aParams.downloadName, null,
                          Math.round(Date.now() * 1000), null, persist);

  
  
  var test = dm.getDownload(dl.id);

  aParams.runBeforeStart.call(undefined, dl);

  persist.progressListener = dl.QueryInterface(Ci.nsIWebProgressListener);
  persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

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

      if (gDownloadCount == 0)
        httpserv.stop();
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };
}


let ps = Cc['@mozilla.org/preferences-service;1'].getService(Ci.nsIPrefBranch);
ps.setBoolPref("browser.download.manager.showAlertOnComplete", false);

cleanup();

