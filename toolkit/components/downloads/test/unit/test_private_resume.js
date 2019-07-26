







function run_test() {
  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  do_test_pending();
  let httpserv = new HttpServer();

  let times = 0;
  httpserv.registerPathHandler("/head_download_manager.js", function (meta, response) {
    response.setHeader("Content-Type", "text/plain", false);
    response.setStatusLine("1.1", !meta.hasHeader('range') ? 200 : 206);

    
    
    
    if (!meta.hasHeader('Cookie')) {
      do_check_true(times == 0 || times == 1);
      response.setHeader('Set-Cookie', 'times=' + times++);
    } else {
      do_check_eq(times, 2);
      do_check_eq(meta.getHeader('Cookie'), 'times=1');
    }
    let full = "";
    let body = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"; 
    for (var i = 0; i < 1000; i++) {
      full += body;
    }
    response.write(full);
  });
  httpserv.start(-1);

  let state = 0;

  let listener = {
    onDownloadStateChange: function(aState, aDownload) {
      switch (aDownload.state) {
        case downloadUtils.downloadManager.DOWNLOAD_DOWNLOADING:
          
          if (state != 1)
            break;

          state++;
          do_check_true(aDownload.resumable);

          aDownload.pause();
          do_check_eq(aDownload.state, downloadUtils.downloadManager.DOWNLOAD_PAUSED);

          do_execute_soon(function() {
            aDownload.resume();
          });
          break;

        case downloadUtils.downloadManager.DOWNLOAD_FINISHED:
          if (state == 0) {
            do_execute_soon(function() {
              
              
              

              state++;

              addDownload(httpserv, {
                isPrivate: true,
                sourceURI: downloadCSource,
                downloadName: downloadCName + "!!!",
                runBeforeStart: function (aDownload) {
                  
                  do_check_eq(downloadUtils.downloadManager.activePrivateDownloadCount, 1);
                }
              });
            });
          } else if (state == 2) {
            
            do_execute_soon(function() {
              httpserv.stop(do_test_finished);
            });
          }
          break;

        default:
          break;
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { }
  };

  downloadUtils.downloadManager.addPrivacyAwareListener(listener);

  const downloadCSource = "http://localhost:" +
                          httpserv.identity.primaryPort +
                          "/head_download_manager.js";
  const downloadCName = "download-C";

  
  let dl = addDownload(httpserv, {
    isPrivate: false,
    sourceURI: downloadCSource,
    downloadName: downloadCName,
    runBeforeStart: function (aDownload) {
      
      do_check_eq(downloadUtils.downloadManager.activeDownloadCount, 1);
    }
  });
}
