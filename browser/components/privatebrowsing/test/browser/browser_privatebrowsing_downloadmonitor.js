







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  if (!gDownloadMgr)
    gDownloadMgr = dm;
  let iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  let panel = document.getElementById("download-monitor");
  waitForExplicitFinish();

  
  addDownload(dm, {
    resultFileName: "pbtest-1",
    downloadName: "PB Test 1"
  });

  
  if (!DownloadMonitorPanel.inited())
    DownloadMonitorPanel.init();
  else
    DownloadMonitorPanel.updateStatus();
  ok(!panel.hidden, "The download panel should be successfully added initially");

  
  pb.privateBrowsingEnabled = true;

  setTimeout(function () {
    ok(panel.hidden, "The download panel should be hidden when entering the private browsing mode");

    
    let file = addDownload(dm, {
      resultFileName: "pbtest-2",
      downloadName: "PB Test 2"
    }).targetFile;

    
    DownloadMonitorPanel.updateStatus();

    
    ok(!panel.hidden, "The download panel should show up when a new download is added");

    
    pb.privateBrowsingEnabled = false;

    setTimeout(function () {
      ok(panel.hidden, "The download panel should be hidden when leaving the private browsing mode");

      
      let dls = dm.activeDownloads;
      while (dls.hasMoreElements()) {
        let dl = dls.getNext().QueryInterface(Ci.nsIDownload);
        dm.removeDownload(dl.id);
        let file = dl.targetFile;
        if (file.exists())
          file.remove(false);
      }
      if (file.exists())
        file.remove(false);

      finish();
    }, 0);
  }, 0);
}












function addDownload(dm, aParams)
{
  if (!aParams)
    aParams = {};
  if (!("resultFileName" in aParams))
    aParams.resultFileName = "download.result";
  if (!("targetFile" in aParams)) {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    aParams.targetFile = dirSvc.get("ProfD", Ci.nsIFile);
    aParams.targetFile.append(aParams.resultFileName);
  }
  if (!("sourceURI" in aParams))
    aParams.sourceURI = "http://localhost:8888/browser/browser/components/privatebrowsing/test/browser/staller.sjs";
  if (!("downloadName" in aParams))
    aParams.downloadName = null;
  if (!("runBeforeStart" in aParams))
    aParams.runBeforeStart = function () {};

  const nsIWBP = Ci.nsIWebBrowserPersist;
  let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                .createInstance(Ci.nsIWebBrowserPersist);
  persist.persistFlags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                         nsIWBP.PERSIST_FLAGS_BYPASS_CACHE |
                         nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

  let dl = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                          createURI(aParams.sourceURI),
                          createURI(aParams.targetFile), aParams.downloadName, null,
                          Math.round(Date.now() * 1000), null, persist);

  
  
  let test = dm.getDownload(dl.id);

  aParams.runBeforeStart.call(undefined, dl);

  persist.progressListener = dl.QueryInterface(Ci.nsIWebProgressListener);
  persist.saveURI(dl.source, null, null, null, null, dl.targetFile);

  return dl;
}

function createURI(aObj)
{
  let ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return (aObj instanceof Ci.nsIFile) ? ios.newFileURI(aObj) :
                                        ios.newURI(aObj, null, null);
}
