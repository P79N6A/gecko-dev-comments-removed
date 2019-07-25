








const Cm = Components.manager;

const kPromptServiceUUID = "{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}";
const kPromptServiceContractID = "@mozilla.org/embedcomp/prompt-service;1";


const kPromptServiceFactory = Cm.getClassObject(Cc[kPromptServiceContractID],
                                                Ci.nsIFactory);

let fakePromptServiceFactory = {
  createInstance: function(aOuter, aIid) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return promptService.QueryInterface(aIid);
  }
};

let promptService = {
  _buttonChoice: 0,
  _called: false,
  wasCalled: function() {
    let called = this._called;
    this._called = false;
    return called;
  },
  sayCancel: function() {
    this._buttonChoice = 1;
    this._called = false;
  },
  sayProceed: function() {
    this._buttonChoice = 0;
    this._called = false;
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIPromptService) ||
        aIID.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  confirmEx: function(parent, title, text, buttonFlags,
                      button0Title, button1Title, button2Title,
                      checkMsg, checkState) {
    this._called = true;
    return this._buttonChoice;
  }
};

Cm.QueryInterface(Ci.nsIComponentRegistrar)
  .registerFactory(Components.ID(kPromptServiceUUID), "Prompt Service",
                   kPromptServiceContractID, fakePromptServiceFactory);

this.__defineGetter__("pb", function () {
  delete this.pb;
  try {
    return this.pb = Cc["@mozilla.org/privatebrowsing;1"].
                     getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {}
  return this.pb = null;
});

this.__defineGetter__("dm", function() {
  delete this.dm;
  return this.dm = Cc["@mozilla.org/download-manager;1"].
                   getService(Ci.nsIDownloadManager);
});

function run_test() {
  if (!pb) 
    return;

  function finishTest() {
    
    dm.cancelDownload(dlE.id);
    dm.removeDownload(dlE.id);
    dm.cleanUp();
    do_check_eq(dm.activeDownloadCount, 0);

    prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
    dm.removeListener(listener);
    httpserv.stop(do_test_finished);

    
    Cm.QueryInterface(Ci.nsIComponentRegistrar)
      .unregisterFactory(Components.ID(kPromptServiceUUID),
                         fakePromptServiceFactory);

    
    Cm.QueryInterface(Ci.nsIComponentRegistrar)
      .registerFactory(Components.ID(kPromptServiceUUID), "Prompt Service",
                       kPromptServiceContractID, kPromptServiceFactory);
  }

  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  do_test_pending();
  let httpserv = new HttpServer();
  httpserv.registerDirectory("/file/", do_get_cwd());
  httpserv.registerPathHandler("/noresume", function (meta, response) {
    response.setHeader("Content-Type", "text/html", false);
    response.setHeader("Accept-Ranges", "none", false);
    response.write("foo");
  });
  httpserv.start(4444);

  let tmpDir = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).
               get("TmpD", Ci.nsIFile);

  
  do_check_eq(dm.activeDownloadCount, 0);

  let listener = {
    onDownloadStateChange: function(aState, aDownload)
    {
      switch (aDownload.state) {
        case dm.DOWNLOAD_QUEUED:
        case dm.DOWNLOAD_DOWNLOADING:
          if (aDownload.targetFile.equals(dlD.targetFile)) {
            
            do_check_false(dlD.resumable);

            
            promptService.sayCancel();

            
            pb.privateBrowsingEnabled = true;
            do_check_true(promptService.wasCalled());
            do_check_false(pb.privateBrowsingEnabled); 

            
            do_check_neq(dlD.state, dm.DOWNLOAD_PAUSED);

            
            promptService.sayProceed();

            
            pb.privateBrowsingEnabled = true;
            do_check_true(promptService.wasCalled());
            do_check_true(pb.privateBrowsingEnabled);

            
            do_check_eq(dlD.state, dm.DOWNLOAD_CANCELED);

            
            pb.privateBrowsingEnabled = false;
            do_check_false(pb.privateBrowsingEnabled);

            
            dlE = addDownload({
              targetFile: fileE,
              sourceURI: downloadESource,
              downloadName: downloadEName
            });

            
          } else if (aDownload.targetFile.equals(dlE.targetFile)) {
            
            do_check_true(dlE.resumable);

            
            pb.privateBrowsingEnabled = true;
            do_check_false(promptService.wasCalled());
            do_check_true(pb.privateBrowsingEnabled);

            
            dlF = addDownload({
              targetFile: fileF,
              sourceURI: downloadFSource,
              downloadName: downloadFName
            });

            
          } else if (aDownload.targetFile.equals(dlF.targetFile)) {
            
            do_check_true(dlF.resumable);

            
            promptService.sayCancel();

            
            pb.privateBrowsingEnabled = false;
            do_check_true(promptService.wasCalled());
            do_check_true(pb.privateBrowsingEnabled); 

            
            do_check_neq(dlF.state, dm.DOWNLOAD_PAUSED);

            
            promptService.sayProceed();

            
            pb.privateBrowsingEnabled = false;
            do_check_true(promptService.wasCalled());
            do_check_false(pb.privateBrowsingEnabled);

            
            do_check_eq(dlF.state, dm.DOWNLOAD_PAUSED);

            finishTest();
          }
          break;
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };

  dm.addListener(listener);

  
  const downloadDSource = "http://localhost:4444/noresume";
  const downloadDDest = "download-file-D";
  const downloadDName = "download-D";

  
  const downloadESource = "http://localhost:4444/file/head_download_manager.js";
  const downloadEDest = "download-file-E";
  const downloadEName = "download-E";

  
  const downloadFSource = "http://localhost:4444/file/test_privatebrowsing_cancel.js";
  const downloadFDest = "download-file-F";
  const downloadFName = "download-F";

  
  let fileD = tmpDir.clone();
  fileD.append(downloadDDest);
  fileD.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let fileE = tmpDir.clone();
  fileE.append(downloadEDest);
  fileE.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let fileF = tmpDir.clone();
  fileF.append(downloadFDest);
  fileF.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let dlD = addDownload({
    targetFile: fileD,
    sourceURI: downloadDSource,
    downloadName: downloadDName
  });
  downloadD = dlD.id;

  let dlE, dlF;

  
}
