





Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
do_load_manifest("test_downloads.manifest");

let httpserver = null;
let currentTest = 0;

function WindowContext() { }
WindowContext.prototype = {
  QueryInterface:  XPCOMUtils.generateQI([Ci.nsIInterfaceRequestor]),
  getInterface: XPCOMUtils.generateQI([Ci.nsIURIContentListener,
                                       Ci.nsILoadGroup]),

  
  onStartURIOpen: function (uri) { },
  isPreferred: function (type, desiredtype) { return false; },

  
  addRequest: function (request, context) { },
  removeRequest: function (request, context, status) { }
};

let DownloadListener = {
  set : null,
  init: function () {
    Services.obs.addObserver(this, "dl-start", true);
    Services.obs.addObserver(this, "dl-done", true);
    Services.obs.addObserver(this, "dl-cancel", true);
    Services.obs.addObserver(this, "dl-fail", true);
  },

  
  
  onCancel: function(aSubject, aTopic, aData) {
    let dl = aSubject.QueryInterface(Ci.nsIDownload);
    do_check_false(dl.targetFile.exists());
    runNextTest();
  },

  observe: function (aSubject, aTopic, aData) {
    switch(aTopic) {
      case "dl-start" :
        
        
        let dl = aSubject.QueryInterface(Ci.nsIDownload);

        if (this.set.doPause) {
          downloadUtils.downloadManager.pauseDownload(dl.id);
        }

        if (this.set.doResume) {
          downloadUtils.downloadManager.resumeDownload(dl.id);
        }

        downloadUtils.downloadManager.cancelDownload(dl.id);
        break;
      case "dl-cancel" :
        this.onCancel(aSubject, aTopic, aData);
        break;
      case "dl-fail" :
        do_throw("Download failed");
        break;
      case "dl-done" :
        do_throw("Download finished");
        break;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
}



function runNextTest()
{
  if (currentTest == tests.length) {
    httpserver.stop(do_test_finished);
    return;
  }

  let set = DownloadListener.set = tests[currentTest];
  currentTest++;
  let uri = "http://localhost:" +
             httpserver.identity.primaryPort +
             set.serverPath;
  let channel = NetUtil.newChannel2(uri,
                                    null,
                                    null,
                                    null,      
                                    Services.scriptSecurityManager.getSystemPrincipal(),
                                    null,      
                                    Ci.nsILoadInfo.SEC_NORMAL,
                                    Ci.nsIContentPolicy.TYPE_OTHER);
  let uriloader = Cc["@mozilla.org/uriloader;1"].getService(Ci.nsIURILoader);
  uriloader.openURI(channel, Ci.nsIURILoader.IS_CONTENT_PREFERRED,
                    new WindowContext());
}



function getResponse(aSet) {
  return function(aMetadata, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);
    if (aMetadata.hasHeader("Range")) {
      var matches = aMetadata.getHeader("Range").match(/^\s*bytes=(\d+)?-(\d+)?\s*$/);
      aResponse.setStatusLine(aMetadata.httpVersion, 206, "Partial Content");
      aResponse.bodyOutputStream.write(aSet.data, aSet.data.length);
      return;
    }
    aResponse.setHeader("Accept-Ranges", "bytes", false);
    aResponse.setHeader("Content-Disposition", "attachment; filename=test.txt;", false);
    aResponse.bodyOutputStream.write(aSet.data, aSet.data.length);
  }
}





let tests = [
  { serverPath: "/test1.html", data: "Test data 1" },
  { serverPath: "/test2.html", data: "Test data 2", doPause: true },
  { serverPath: "/test3.html", data: "Test data 3", doPause: true, doResume: true},
];

function run_test() {
  if (oldDownloadManagerDisabled()) {
    return;
  }

  
  DownloadListener.init();
  Services.prefs.setBoolPref("browser.download.manager.showWhenStarting", false);

  httpserver = new HttpServer();
  httpserver.start(-1);
  do_test_pending();

  
  
  for(let i = 0; i < tests.length; i++) {
    let set = tests[i];
    httpserver.registerPathHandler(set.serverPath, getResponse(set));
  }

  runNextTest(); 
}
