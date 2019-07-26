








"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadPaths",
                                  "resource://gre/modules/DownloadPaths.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadIntegration",
                                  "resource://gre/modules/DownloadIntegration.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "HttpServer",
                                  "resource://testing-common/httpd.js");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gExternalHelperAppService",
           "@mozilla.org/uriloader/external-helper-app-service;1",
           Ci.nsIExternalHelperAppService);

const ServerSocket = Components.Constructor(
                                "@mozilla.org/network/server-socket;1",
                                "nsIServerSocket",
                                "init");
const BinaryOutputStream = Components.Constructor(
                                      "@mozilla.org/binaryoutputstream;1",
                                      "nsIBinaryOutputStream",
                                      "setOutputStream")

XPCOMUtils.defineLazyServiceGetter(this, "gMIMEService",
                                   "@mozilla.org/mime;1",
                                   "nsIMIMEService");

const TEST_TARGET_FILE_NAME = "test-download.txt";
const TEST_STORE_FILE_NAME = "test-downloads.json";

const TEST_REFERRER_URL = "http://www.example.com/referrer.html";

const TEST_DATA_SHORT = "This test string is downloaded.";

const TEST_DATA_SHORT_GZIP_ENCODED_FIRST = [
 31,139,8,0,0,0,0,0,0,3,11,201,200,44,86,40,73,45,46,81,40,46,41,202,204
];
const TEST_DATA_SHORT_GZIP_ENCODED_SECOND = [
  75,87,0,114,83,242,203,243,114,242,19,83,82,83,244,0,151,222,109,43,31,0,0,0
];
const TEST_DATA_SHORT_GZIP_ENCODED =
  TEST_DATA_SHORT_GZIP_ENCODED_FIRST.concat(TEST_DATA_SHORT_GZIP_ENCODED_SECOND);




function run_test()
{
  do_get_profile();
  run_next_test();
}







let gHttpServer;





function httpUrl(aFileName) {
  return "http://localhost:" + gHttpServer.identity.primaryPort + "/" +
         aFileName;
}





let gFileCounter = Math.floor(Math.random() * 1000000);















function getTempFile(aLeafName)
{
  
  let [base, ext] = DownloadPaths.splitBaseNameAndExtension(aLeafName);
  let leafName = base + "-" + gFileCounter + ext;
  gFileCounter++;

  
  let file = FileUtils.getFile("TmpD", [leafName]);
  do_check_false(file.exists());

  do_register_cleanup(function () {
    if (file.exists()) {
      file.remove(false);
    }
  });

  return file;
}








function promiseExecuteSoon()
{
  let deferred = Promise.defer();
  do_execute_soon(deferred.resolve);
  return deferred.promise;
}








function promiseTimeout(aTime)
{
  let deferred = Promise.defer();
  do_timeout(aTime, deferred.resolve);
  return deferred.promise;
}











function promiseTopicObserved(aTopic)
{
  let deferred = Promise.defer();

  Services.obs.addObserver(
    function PTO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(PTO_observe, aTopic);
      deferred.resolve([aSubject, aData]);
    }, aTopic, false);

  return deferred.promise;
}








function promiseClearHistory()
{
  let promise = promiseTopicObserved(PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  do_execute_soon(function() PlacesUtils.bhistory.removeAllPages());
  return promise;
}











function promiseWaitForVisit(aUrl)
{
  let deferred = Promise.defer();

  let uri = NetUtil.newURI(aUrl);

  PlacesUtils.history.addObserver({
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver]),
    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onVisit: function (aURI, aVisitID, aTime, aSessionID, aReferringID,
                       aTransitionType, aGUID, aHidden) {
      if (aURI.equals(uri)) {
        PlacesUtils.history.removeObserver(this);
        deferred.resolve([aTime, aTransitionType]);
      }
    },
    onTitleChanged: function () {},
    onDeleteURI: function () {},
    onClearHistory: function () {},
    onPageChanged: function () {},
    onDeleteVisits: function () {},
  }, false);

  return deferred.promise;
}











function promiseIsURIVisited(aUrl) {
  let deferred = Promise.defer();

  PlacesUtils.asyncHistory.isURIVisited(NetUtil.newURI(aUrl),
    function (aURI, aIsVisited) {
      deferred.resolve(aIsVisited);
    });

  return deferred.promise;
}












function promiseNewDownload(aSourceUrl) {
  return Downloads.createDownload({
    source: aSourceUrl || httpUrl("source.txt"),
    target: getTempFile(TEST_TARGET_FILE_NAME),
  });
}




























function promiseStartLegacyDownload(aSourceUrl, aOptions) {
  let sourceURI = NetUtil.newURI(aSourceUrl || httpUrl("source.txt"));
  let targetFile = (aOptions && aOptions.targetFile)
                   || getTempFile(TEST_TARGET_FILE_NAME);

  let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);
  if (aOptions) {
    aOptions.outPersist = persist;
  }

  let fileExtension = null, mimeInfo = null;
  let match = sourceURI.path.match(/\.([^.\/]+)$/);
  if (match) {
    fileExtension = match[1];
  }

  if (fileExtension) {
    try {
      mimeInfo = gMIMEService.getFromTypeAndExtension(null, fileExtension);
      mimeInfo.preferredAction = Ci.nsIMIMEInfo.saveToDisk;
    } catch (ex) { }
  }

  if (aOptions && aOptions.launcherPath) {
    do_check_true(mimeInfo != null);

    let localHandlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"]
                            .createInstance(Ci.nsILocalHandlerApp);
    localHandlerApp.executable = new FileUtils.File(aOptions.launcherPath);

    mimeInfo.preferredApplicationHandler = localHandlerApp;
  }

  if (aOptions && aOptions.launchWhenSucceeded) {
    do_check_true(mimeInfo != null);

    mimeInfo.preferredAction = Ci.nsIMIMEInfo.useHelperApp;
  }

  
  persist.persistFlags &= ~Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NO_CONVERSION;
  persist.persistFlags |=
    Ci.nsIWebBrowserPersist.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

  
  
  
  
  let transfer =
      Components.classesByID["{1b4c85df-cbdd-4bb6-b04e-613caece083c}"]
                .createInstance(Ci.nsITransfer);

  let deferred = Promise.defer();

  let isPrivate = aOptions && aOptions.isPrivate;
  let promise = isPrivate ? Downloads.getPrivateDownloadList()
                          : Downloads.getPublicDownloadList();
  promise.then(function (aList) {
    
    
    aList.addView({
      onDownloadAdded: function (aDownload) {
        aList.removeView(this);

        
        
        aList.remove(aDownload);

        
        deferred.resolve(aDownload);
      },
    });

    
    
    transfer.init(sourceURI, NetUtil.newURI(targetFile), null, mimeInfo, null,
                  null, persist, isPrivate);
    persist.progressListener = transfer;

    
    persist.savePrivacyAwareURI(sourceURI, null, null, null, null, targetFile,
                                isPrivate);
  }.bind(this)).then(null, do_report_unexpected_exception);

  return deferred.promise;
}















function promiseStartExternalHelperAppServiceDownload(aSourceUrl) {
  let sourceURI = NetUtil.newURI(aSourceUrl ||
                                 httpUrl("interruptible_resumable.txt"));

  let deferred = Promise.defer();

  Downloads.getPublicDownloadList().then(function (aList) {
    
    
    aList.addView({
      onDownloadAdded: function (aDownload) {
        aList.removeView(this);

        
        
        aList.remove(aDownload);

        
        deferred.resolve(aDownload);
      },
    });

    let channel = NetUtil.newChannel(sourceURI);

    
    channel.asyncOpen({
      contentListener: null,

      onStartRequest: function (aRequest, aContext)
      {
        let channel = aRequest.QueryInterface(Ci.nsIChannel);
        this.contentListener = gExternalHelperAppService.doContent(
                                     channel.contentType, aRequest, null, true);
        this.contentListener.onStartRequest(aRequest, aContext);
      },

      onStopRequest: function (aRequest, aContext, aStatusCode)
      {
        this.contentListener.onStopRequest(aRequest, aContext, aStatusCode);
      },

      onDataAvailable: function (aRequest, aContext, aInputStream, aOffset,
                                 aCount)
      {
        this.contentListener.onDataAvailable(aRequest, aContext, aInputStream,
                                             aOffset, aCount);
      },
    }, null);
  }.bind(this)).then(null, do_report_unexpected_exception);

  return deferred.promise;
}








function promiseNewDownloadList() {
  
  Downloads._promisePublicDownloadList = null;
  return Downloads.getPublicDownloadList();
}








function promiseNewPrivateDownloadList() {
  
  Downloads._promisePrivateDownloadList = null;
  return Downloads.getPrivateDownloadList();
}














function promiseVerifyContents(aPath, aExpectedContents)
{
  return Task.spawn(function() {
    let file = new FileUtils.File(aPath);

    if (!(yield OS.File.exists(aPath))) {
      do_throw("File does not exist: " + aPath);
    }

    if ((yield OS.File.stat(aPath)).size == 0) {
      do_throw("File is empty: " + aPath);
    }

    let deferred = Promise.defer();
    NetUtil.asyncFetch(file, function(aInputStream, aStatus) {
      do_check_true(Components.isSuccessCode(aStatus));
      let contents = NetUtil.readInputStreamToString(aInputStream,
                                                     aInputStream.available());
      if (contents.length > TEST_DATA_SHORT.length * 2 ||
          /[^\x20-\x7E]/.test(contents)) {
        
        do_check_eq(contents.length, aExpectedContents.length);
        do_check_true(contents == aExpectedContents);
      } else {
        
        do_check_eq(contents, aExpectedContents);
      }
      deferred.resolve();
    });
    yield deferred.promise;
  });
}







function startFakeServer()
{
  let serverSocket = new ServerSocket(-1, true, -1);
  serverSocket.asyncListen({
    onSocketAccepted: function (aServ, aTransport) {
      aTransport.close(Cr.NS_BINDING_ABORTED);
    },
    onStopListening: function () { },
  });
  return serverSocket;
}




let _gDeferResponses = Promise.defer();



















function mustInterruptResponses()
{
  
  
  
  
  _gDeferResponses.resolve();

  do_print("Interruptible responses will be blocked midway.");
  _gDeferResponses = Promise.defer();
}




function continueResponses()
{
  do_print("Interruptible responses are now allowed to continue.");
  _gDeferResponses.resolve();
}













function registerInterruptibleHandler(aPath, aFirstPartFn, aSecondPartFn)
{
  gHttpServer.registerPathHandler(aPath, function (aRequest, aResponse) {
    do_print("Interruptible request started.");

    
    aResponse.processAsync();
    aFirstPartFn(aRequest, aResponse);

    
    _gDeferResponses.promise.then(function RIH_onSuccess() {
      aSecondPartFn(aRequest, aResponse);
      aResponse.finish();
      do_print("Interruptible request finished.");
    }).then(null, Cu.reportError);
  });
}







function isValidDate(aDate) {
  return aDate && aDate.getTime && !isNaN(aDate.getTime());
}





let gMostRecentFirstBytePos;




add_task(function test_common_initialize()
{
  
  gHttpServer = new HttpServer();
  gHttpServer.registerDirectory("/", do_get_file("../data"));
  gHttpServer.start(-1);

  registerInterruptibleHandler("/interruptible.txt",
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);
      aResponse.setHeader("Content-Length", "" + (TEST_DATA_SHORT.length * 2),
                          false);
      aResponse.write(TEST_DATA_SHORT);
    }, function secondPart(aRequest, aResponse) {
      aResponse.write(TEST_DATA_SHORT);
    });

  registerInterruptibleHandler("/interruptible_resumable.txt",
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);

      
      let data = TEST_DATA_SHORT + TEST_DATA_SHORT;
      if (aRequest.hasHeader("Range")) {
        var matches = aRequest.getHeader("Range")
                              .match(/^\s*bytes=(\d+)?-(\d+)?\s*$/);
        var firstBytePos = (matches[1] === undefined) ? 0 : matches[1];
        var lastBytePos = (matches[2] === undefined) ? data.length - 1
                                            : matches[2];
        if (firstBytePos >= data.length) {
          aResponse.setStatusLine(aRequest.httpVersion, 416,
                             "Requested Range Not Satisfiable");
          aResponse.setHeader("Content-Range", "*/" + data.length, false);
          aResponse.finish();
          return;
        }

        aResponse.setStatusLine(aRequest.httpVersion, 206, "Partial Content");
        aResponse.setHeader("Content-Range", firstBytePos + "-" +
                                             lastBytePos + "/" +
                                             data.length, false);

        data = data.substring(firstBytePos, lastBytePos + 1);

        gMostRecentFirstBytePos = firstBytePos;
      } else {
        gMostRecentFirstBytePos = 0;
      }

      aResponse.setHeader("Content-Length", "" + data.length, false);

      aResponse.write(data.substring(0, data.length / 2));

      
      
      aResponse.secondPartData = data.substring(data.length / 2);
    }, function secondPart(aRequest, aResponse) {
      aResponse.write(aResponse.secondPartData);
    });

  registerInterruptibleHandler("/interruptible_gzip.txt",
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);
      aResponse.setHeader("Content-Encoding", "gzip", false);
      aResponse.setHeader("Content-Length", "" + TEST_DATA_SHORT_GZIP_ENCODED.length);

      let bos =  new BinaryOutputStream(aResponse.bodyOutputStream);
      bos.writeByteArray(TEST_DATA_SHORT_GZIP_ENCODED_FIRST,
                         TEST_DATA_SHORT_GZIP_ENCODED_FIRST.length);
    }, function secondPart(aRequest, aResponse) {
      let bos =  new BinaryOutputStream(aResponse.bodyOutputStream);
      bos.writeByteArray(TEST_DATA_SHORT_GZIP_ENCODED_SECOND,
                         TEST_DATA_SHORT_GZIP_ENCODED_SECOND.length);
    });

  
  DownloadIntegration.dontLoadList = true;
  DownloadIntegration.dontLoadObservers = true;
  
  DownloadIntegration.dontCheckParentalControls = true;
  
  DownloadIntegration.dontOpenFileAndFolder = true;
  DownloadIntegration._deferTestOpenFile = Promise.defer();
  DownloadIntegration._deferTestShowDir = Promise.defer();

  
  
  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  do_register_cleanup(() => registrar = null);

  
  
  let mockFactory = {
    createInstance: function (aOuter, aIid) {
      return {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),
        promptForSaveToFile: function (aLauncher, aWindowContext,
                                       aDefaultFileName,
                                       aSuggestedFileExtension,
                                       aForcePrompt)
        {
          throw new Components.Exception(
                             "Synchronous promptForSaveToFile not implemented.",
                             Cr.NS_ERROR_NOT_AVAILABLE);
        },
        promptForSaveToFileAsync: function (aLauncher, aWindowContext,
                                            aDefaultFileName,
                                            aSuggestedFileExtension,
                                            aForcePrompt)
        {
          let file = getTempFile(TEST_TARGET_FILE_NAME);
          aLauncher.saveDestinationAvailable(file);
        },
      }.QueryInterface(aIid);
    }
  };

  let contractID = "@mozilla.org/helperapplauncherdialog;1";
  let cid = registrar.contractIDToCID(contractID);
  let oldFactory = Components.manager.getClassObject(Cc[contractID],
                                                     Ci.nsIFactory);

  registrar.unregisterFactory(cid, oldFactory);
  registrar.registerFactory(cid, "", contractID, mockFactory);
  do_register_cleanup(function () {
    registrar.unregisterFactory(cid, mockFactory);
    registrar.registerFactory(cid, "", contractID, oldFactory);
  });

  
  
  
  
  
  let transferContractID = "@mozilla.org/transfer;1";
  let transferNewCid = Components.ID("{1b4c85df-cbdd-4bb6-b04e-613caece083c}");
  let transferCid = registrar.contractIDToCID(transferContractID);

  registrar.registerFactory(transferNewCid, "", transferContractID, null);
  do_register_cleanup(function () {
    registrar.registerFactory(transferCid, "", transferContractID, null);
  });
});
