





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
  set: null,
  prevFiles : [],

  init: function () {
    Services.obs.addObserver(this, "dl-start", true);
    Services.obs.addObserver(this, "dl-done", true);
  },

  observe: function (aSubject, aTopic, aData) {
    
    if (aTopic == "dl-start") {
      
      if (this.set.doPause) {
        let dl = aSubject.QueryInterface(Ci.nsIDownload);
        
        
        do_execute_soon(function() {
          downloadUtils.downloadManager.pauseDownload(dl.id);
          do_timeout(1000, function() {
            downloadUtils.downloadManager.resumeDownload(dl.id);
          });
        });
      }
    } else if (aTopic == "dl-done") {
      
      let file = aSubject.QueryInterface(Ci.nsIDownload).targetFile;
      for each (let prevFile in this.prevFiles) {
        do_check_neq(file.leafName, prevFile.leafName);
      }
      this.prevFiles.push(file);

      
      let fis = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
      fis.init(file, -1, -1, 0);
      var cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].createInstance(Ci.nsIConverterInputStream);
      cstream.init(fis, "UTF-8", 0, 0);

      let val = "";
      let str = {};
      let read = 0;
      do {
        read = cstream.readString(0xffffffff, str);
        val += str.value;
      } while (read != 0);
      cstream.close();

      
      if (this.set.doPause) {
        do_check_eq(val, this.set.data + this.set.data); 
      } else {
        do_check_eq(val, this.set.data);
      }
      runNextTest(); 
    }
  },

  QueryInterface:  XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsISupportsWeakReference])
}




function runNextTest()
{
  if (currentTest == tests.length) {
    for each (var file in DownloadListener.prevFiles) {
      try {
        file.remove(false);
      } catch (ex) {
        try {
          do_report_unexpected_exception(ex, "while removing " + file.path);
        } catch (ex if ex == Components.results.NS_ERROR_ABORT) {
          
        }
      }
    }
    httpserver.stop(do_test_finished);
    return;
  }
  let set = DownloadListener.set = tests[currentTest];
  currentTest++;
  let uri = "http://localhost:" +
            httpserver.identity.primaryPort +
            set.serverURL;
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
  { serverURL: "/test1.html", data: "Test data 1", doPause: false },
  { serverURL: "/test2.html", data: "Test data 2", doPause: false },
  { serverURL: "/test3.html", data: "Test data 3", doPause: true }
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
    httpserver.registerPathHandler(set.serverURL, getResponse(set));
  }

  runNextTest();  
}
