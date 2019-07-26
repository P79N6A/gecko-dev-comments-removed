










Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ForgetAboutSite.jsm");











function uri(aFile)
{
  return Cc["@mozilla.org/network/io-service;1"].
         getService(Ci.nsIIOService).
         newFileURI(aFile);
}









function check_active_download(aURIString, aIsActive)
{
  let dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  let enumerator = dm.activeDownloads;
  let found = false;
  while (enumerator.hasMoreElements()) {
    let dl = enumerator.getNext().QueryInterface(Ci.nsIDownload);
    if (dl.source.spec == aURIString)
      found = true;
  }
  let checker = aIsActive ? do_check_true : do_check_false;
  checker(found);
}




let destFile = dirSvc.get("TmpD", Ci.nsIFile);
destFile.append("dm-test-file");
destFile = uri(destFile);
let data = [
  { source: "http://mozilla.org/direct_match",
    target: destFile.spec,
    removed: true
  },
  { source: "http://www.mozilla.org/subdomain",
    target: destFile.spec,
    removed: true
  },
  { source: "http://ilovemozilla.org/contains_domain",
    target: destFile.spec,
    removed: false
  },
];

function makeGUID() {
  let guid = "";
  for (var i = 0; i < 12; i++)
    guid += Math.floor(Math.random() * 10);
  return guid;
}

function run_test()
{
  if (oldDownloadManagerDisabled()) {
    return;
  }

  
  
  

  
  let downloads = do_get_file("downloads.empty.sqlite");
  downloads.copyTo(dirSvc.get("ProfD", Ci.nsIFile), "downloads.sqlite");

  
  let ss = Cc["@mozilla.org/storage/service;1"].
           getService(Ci.mozIStorageService);
  let file = dirSvc.get("ProfD", Ci.nsIFile);
  file.append("downloads.sqlite");
  let db = ss.openDatabase(file);

  
  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, target, state, autoResume, entityID, guid) " +
    "VALUES (:source, :target, :state, :autoResume, :entityID, :guid)"
  );
  for (let i = 0; i < data.length; i++) {
    stmt.params.source = data[i].source;
    stmt.params.target = data[i].target;
    stmt.params.state = Ci.nsIDownloadManager.DOWNLOAD_PAUSED;
    stmt.params.autoResume = 0; 
    stmt.params.entityID = "foo" 
    stmt.params.guid = makeGUID();
    stmt.execute();
    stmt.reset();
  }
  stmt.finalize();
  stmt = null;
  db.close();
  db = null;

  
  for (let i = 0; i < data.length; i++)
    check_active_download(data[i].source, true);

  
  ForgetAboutSite.removeDataFromDomain("mozilla.org");

  
  for (let i = 0; i < data.length; i++)
    check_active_download(data[i].source, !data[i].removed);

  
  Services.obs.notifyObservers(null, "quit-application", null);
}
