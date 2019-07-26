








"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "HttpServer",
                                  "resource://testing-common/httpd.js");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const ServerSocket = Components.Constructor(
                                "@mozilla.org/network/server-socket;1",
                                "nsIServerSocket",
                                "init");

const HTTP_SERVER_PORT = 4444;
const HTTP_BASE = "http://localhost:" + HTTP_SERVER_PORT;

const FAKE_SERVER_PORT = 4445;
const FAKE_BASE = "http://localhost:" + FAKE_SERVER_PORT;

const TEST_SOURCE_URI = NetUtil.newURI(HTTP_BASE + "/source.txt");
const TEST_FAKE_SOURCE_URI = NetUtil.newURI(FAKE_BASE + "/source.txt");

const TEST_TARGET_FILE_NAME = "test-download.txt";
const TEST_DATA_SHORT = "This test string is downloaded.";




function run_test()
{
  run_next_test();
}









function getTempFile(aLeafName)
{
  let file = FileUtils.getFile("TmpD", [aLeafName]);
  function GTF_removeFile()
  {
    if (file.exists()) {
      file.remove(false);
    }
  }

  
  GTF_removeFile();

  
  do_register_cleanup(GTF_removeFile);

  return file;
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




let gHttpServer;

add_task(function test_common_initialize()
{
  
  gHttpServer = new HttpServer();
  gHttpServer.registerDirectory("/", do_get_file("../data"));
  gHttpServer.start(HTTP_SERVER_PORT);
});
