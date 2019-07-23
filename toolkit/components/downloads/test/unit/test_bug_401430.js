






































const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);





const resultFileName = "test\u00e3\u041b\u3056" + Date.now() + ".doc";

function checkResult() {
  
  var resultFile = do_get_file(resultFileName);
  resultFile.remove(false);

  do_check_true(checkRecentDocsFor(resultFileName));
  do_test_finished();
}

function checkRecentDocsFor(aFileName) {
  var recentDocsKey = Cc["@mozilla.org/windows-registry-key;1"].
                        createInstance(Ci.nsIWindowsRegKey);
  var recentDocsPath =
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs";
  recentDocsKey.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                     recentDocsPath,
                     Ci.nsIWindowsRegKey.ACCESS_READ);
  var count = recentDocsKey.valueCount;
  for (var i = 0; i < count; ++i) {
    var valueName = recentDocsKey.getValueName(i);
    var binValue = recentDocsKey.readBinaryValue(valueName);

    
    
    var fileNameRaw = binValue.split("\0\0")[0];

    
    var fileName = "";
    for (var c = 0; c < fileNameRaw.length; c += 2)
      fileName += String.fromCharCode(fileNameRaw.charCodeAt(c) |
                                      fileNameRaw.charCodeAt(c+1) * 256);

    if (aFileName == fileName)
      return true;
  }
  return false;
}

var httpserv = null;
function run_test()
{
  
  
  var httpPH = Cc["@mozilla.org/network/protocol;1?name=http"].
               getService(Ci.nsIHttpProtocolHandler);
  if (httpPH.platform != "Windows")
    return;

  
  do_test_pending();

  httpserv = new nsHttpServer();
  httpserv.registerDirectory("/", do_get_cwd());
  httpserv.start(4444);

  var listener = {
    onDownloadStateChange: function test_401430_odsc(aState, aDownload) {
      if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
        
        
        do_timeout(1000, "checkResult();");
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };

  dm.addListener(listener);
  dm.addListener(getDownloadListener());

  var dl = addDownload({resultFileName: resultFileName});
}
