








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

this.__defineGetter__("dm", function() {
  delete this.dm;
  return this.dm = Cc["@mozilla.org/download-manager;1"].
                   getService(Ci.nsIDownloadManager);
});

function trigger_pb_cleanup(expected)
{
  var obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  var cancel = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
  cancel.data = false;
  obs.notifyObservers(cancel, "last-pb-context-exiting", null);
  do_check_eq(expected, cancel.data);
  if (!expected)
    obs.notifyObservers(cancel, "last-pb-context-exited", null);
}

function run_test() {
  function finishTest() {
    
    dlG.cancel();
    dlG.remove();
    dm.cleanUp();
    dm.cleanUpPrivate();
    do_check_eq(dm.activeDownloadCount, 0);
    do_check_eq(dm.activePrivateDownloadCount, 0);

    dm.removeListener(listener);
    httpserv.stop(do_test_finished);

    
    Cm.QueryInterface(Ci.nsIComponentRegistrar)
      .unregisterFactory(Components.ID(kPromptServiceUUID),
                         fakePromptServiceFactory);

    
    Cm.QueryInterface(Ci.nsIComponentRegistrar)
      .registerFactory(Components.ID(kPromptServiceUUID), "Prompt Service",
                       kPromptServiceContractID, kPromptServiceFactory);
  }

  do_test_pending();
  let httpserv = new HttpServer();
  httpserv.registerDirectory("/file/", do_get_cwd());
  httpserv.registerPathHandler("/noresume", function (meta, response) {
    response.setHeader("Content-Type", "text/html", false);
    response.setHeader("Accept-Ranges", "none", false);
    response.write("foo");
  });
  httpserv.start(-1);

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
            trigger_pb_cleanup(true);
            do_check_true(promptService.wasCalled());
            do_check_eq(dm.activePrivateDownloadCount, 1);

            promptService.sayProceed();
            trigger_pb_cleanup(false);
            do_check_true(promptService.wasCalled());
            do_check_eq(dm.activePrivateDownloadCount, 0);
            do_check_eq(dlD.state, dm.DOWNLOAD_CANCELED);

            
            dlE = addDownload(httpserv, {
              isPrivate: true,
              targetFile: fileE,
              sourceURI: downloadESource,
              downloadName: downloadEName
            });

            
          } else if (aDownload.targetFile.equals(dlE.targetFile)) {
            
            do_check_true(dlE.resumable);

            promptService.sayCancel();
            trigger_pb_cleanup(true);
            do_check_true(promptService.wasCalled());
            do_check_eq(dm.activePrivateDownloadCount, 1);

            promptService.sayProceed();
            trigger_pb_cleanup(false);
            do_check_true(promptService.wasCalled());
            do_check_eq(dm.activePrivateDownloadCount, 0);
            do_check_eq(dlE.state, dm.DOWNLOAD_CANCELED);

            
            dlF = addDownload(httpserv, {
              isPrivate: true,
              targetFile: fileF,
              sourceURI: downloadFSource,
              downloadName: downloadFName
            });

            
          } else if (aDownload.targetFile.equals(dlF.targetFile)) {
            
            do_check_true(dlF.resumable);
            dlF.pause();

          } else if (aDownload.targetFile.equals(dlG.targetFile)) {
            
            do_check_false(dlG.resumable);

            promptService.sayCancel();
            trigger_pb_cleanup(false);
            do_check_false(promptService.wasCalled());
            do_check_eq(dm.activeDownloadCount, 1);
            do_check_eq(dlG.state, dm.DOWNLOAD_DOWNLOADING);
            finishTest();
          }
          break;

        case dm.DOWNLOAD_PAUSED:
          if (aDownload.targetFile.equals(dlF.targetFile)) {
            promptService.sayProceed();
            trigger_pb_cleanup(false);
            do_check_true(promptService.wasCalled());
            do_check_eq(dm.activePrivateDownloadCount, 0);
            do_check_eq(dlF.state, dm.DOWNLOAD_CANCELED);

            
            dlG = addDownload(httpserv, {
              isPrivate: false,
              targetFile: fileG,
              sourceURI: downloadGSource,
              downloadName: downloadGName
            });

            
          }
          break;
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };

  dm.addPrivacyAwareListener(listener);

  const PORT = httpserv.identity.primaryPort;

  
  const downloadDSource = "http://localhost:" + PORT + "/noresume";
  const downloadDDest = "download-file-D";
  const downloadDName = "download-D";

  
  const downloadESource = "http://localhost:" + PORT + "/file/head_download_manager.js";
  const downloadEDest = "download-file-E";
  const downloadEName = "download-E";

  
  const downloadFSource = "http://localhost:" + PORT + "/file/head_download_manager.js";
  const downloadFDest = "download-file-F";
  const downloadFName = "download-F";

  
  const downloadGSource = "http://localhost:" + PORT + "/noresume";
  const downloadGDest = "download-file-G";
  const downloadGName = "download-G";

  
  let fileD = tmpDir.clone();
  fileD.append(downloadDDest);
  fileD.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let fileE = tmpDir.clone();
  fileE.append(downloadEDest);
  fileE.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let fileF = tmpDir.clone();
  fileF.append(downloadFDest);
  fileF.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  let fileG = tmpDir.clone();
  fileG.append(downloadGDest);
  fileG.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let dlD = addDownload(httpserv, {
    isPrivate: true,
    targetFile: fileD,
    sourceURI: downloadDSource,
    downloadName: downloadDName
  });

  let dlE, dlF, dlG;

  
}
