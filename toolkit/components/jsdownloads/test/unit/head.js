








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

const ServerSocket = Components.Constructor(
                                "@mozilla.org/network/server-socket;1",
                                "nsIServerSocket",
                                "init");
const BinaryOutputStream = Components.Constructor(
                                      "@mozilla.org/binaryoutputstream;1",
                                      "nsIBinaryOutputStream",
                                      "setOutputStream")

const HTTP_SERVER_PORT = 4444;
const HTTP_BASE = "http://localhost:" + HTTP_SERVER_PORT;

const FAKE_SERVER_PORT = 4445;
const FAKE_BASE = "http://localhost:" + FAKE_SERVER_PORT;

const TEST_REFERRER_URI = NetUtil.newURI(HTTP_BASE + "/referrer.html");
const TEST_SOURCE_URI = NetUtil.newURI(HTTP_BASE + "/source.txt");
const TEST_EMPTY_URI = NetUtil.newURI(HTTP_BASE + "/empty.txt");
const TEST_FAKE_SOURCE_URI = NetUtil.newURI(FAKE_BASE + "/source.txt");

const TEST_EMPTY_NOPROGRESS_PATH = "/empty-noprogress.txt";
const TEST_EMPTY_NOPROGRESS_URI = NetUtil.newURI(HTTP_BASE +
                                                 TEST_EMPTY_NOPROGRESS_PATH);

const TEST_INTERRUPTIBLE_PATH = "/interruptible.txt";
const TEST_INTERRUPTIBLE_URI = NetUtil.newURI(HTTP_BASE +
                                              TEST_INTERRUPTIBLE_PATH);

const TEST_INTERRUPTIBLE_GZIP_PATH = "/interruptible_gzip.txt";
const TEST_INTERRUPTIBLE_GZIP_URI = NetUtil.newURI(HTTP_BASE +
                                                   TEST_INTERRUPTIBLE_GZIP_PATH);

const TEST_TARGET_FILE_NAME = "test-download.txt";
const TEST_STORE_FILE_NAME = "test-downloads.json";

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











function promiseSimpleDownload(aSourceURI) {
  return Downloads.createDownload({
    source: { uri: aSourceURI || TEST_SOURCE_URI },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  });
}








function promiseNewDownloadList() {
  
  Downloads._promisePublicDownloadList = null;
  return Downloads.getPublicDownloadList();
}








function promiseNewPrivateDownloadList() {
  
  Downloads._privateDownloadList = null;
  return Downloads.getPrivateDownloadList();
}













function promiseVerifyContents(aFile, aExpectedContents)
{
  let deferred = Promise.defer();
  NetUtil.asyncFetch(aFile, function(aInputStream, aStatus) {
    do_check_true(Components.isSuccessCode(aStatus));
    let contents = NetUtil.readInputStreamToString(aInputStream,
                                                   aInputStream.available());
    if (contents.length <= TEST_DATA_SHORT.length * 2) {
      do_check_eq(contents, aExpectedContents);
    } else {
      
      do_check_eq(contents.length, aExpectedContents.length);
      do_check_true(contents == aExpectedContents);
    }
    deferred.resolve();
  });
  return deferred.promise;
}










function promiseAddDownloadToHistory(aSourceURI) {
  let deferred = Promise.defer();
  PlacesUtils.asyncHistory.updatePlaces(
    {
      uri: aSourceURI || TEST_SOURCE_URI,
      visits: [{
        transitionType: Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
        visitDate:  Date.now()
      }]
    },
    {
      handleError: function handleError(aResultCode, aPlaceInfo) {
        let ex = new Components.Exception("Unexpected error in adding visits.",
                                          aResultCode);
        deferred.reject(ex);
      },
      handleResult: function () {},
      handleCompletion: function handleCompletion() {
        deferred.resolve();
      }
    });
  return deferred.promise;
}







function startFakeServer()
{
  let serverSocket = new ServerSocket(FAKE_SERVER_PORT, true, -1);
  serverSocket.asyncListen({
    onSocketAccepted: function (aServ, aTransport) {
      aTransport.close(Cr.NS_BINDING_ABORTED);
    },
    onStopListening: function () { },
  });
  return serverSocket;
}
























function deferNextResponse()
{
  do_print("Interruptible request will be controlled.");

  
  if (!deferNextResponse._deferred) {
    deferNextResponse._deferred = Promise.defer();
  }
  return deferNextResponse._deferred;
}










function promiseNextRequestReceived()
{
  do_print("Requested notification when interruptible request is received.");

  
  promiseNextRequestReceived._deferred = Promise.defer();
  return promiseNextRequestReceived._deferred.promise;
}














function registerInterruptibleHandler(aPath, aFirstPartFn, aSecondPartFn)
{
  gHttpServer.registerPathHandler(aPath, function (aRequest, aResponse) {
    
    
    let deferResponse = deferNextResponse._deferred;
    deferNextResponse._deferred = null;
    if (deferResponse) {
      do_print("Interruptible request started under control.");
    } else {
      do_print("Interruptible request started without being controlled.");
      deferResponse = Promise.defer();
      deferResponse.resolve();
    }

    
    aResponse.processAsync();
    aFirstPartFn(aRequest, aResponse);

    if (promiseNextRequestReceived._deferred) {
      do_print("Notifying that interruptible request has been received.");
      promiseNextRequestReceived._deferred.resolve();
      promiseNextRequestReceived._deferred = null;
    }

    
    deferResponse.promise.then(function RIH_onSuccess() {
      aSecondPartFn(aRequest, aResponse);
      aResponse.finish();
      do_print("Interruptible request finished.");
    }, function RIH_onFailure() {
      aResponse.abort();
      do_print("Interruptible request aborted.");
    });
  });
}







function isValidDate(aDate) {
  return aDate && aDate.getTime && !isNaN(aDate.getTime());
}




let gHttpServer;

add_task(function test_common_initialize()
{
  
  gHttpServer = new HttpServer();
  gHttpServer.registerDirectory("/", do_get_file("../data"));
  gHttpServer.start(HTTP_SERVER_PORT);

  registerInterruptibleHandler(TEST_INTERRUPTIBLE_PATH,
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);
      aResponse.setHeader("Content-Length", "" + (TEST_DATA_SHORT.length * 2),
                          false);
      aResponse.write(TEST_DATA_SHORT);
    }, function secondPart(aRequest, aResponse) {
      aResponse.write(TEST_DATA_SHORT);
    });

  registerInterruptibleHandler(TEST_EMPTY_NOPROGRESS_PATH,
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);
    }, function secondPart(aRequest, aResponse) { });


  registerInterruptibleHandler(TEST_INTERRUPTIBLE_GZIP_PATH,
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

  
  DownloadIntegration.dontLoad = true;
  
  DownloadIntegration.dontCheckParentalControls = true;
});
