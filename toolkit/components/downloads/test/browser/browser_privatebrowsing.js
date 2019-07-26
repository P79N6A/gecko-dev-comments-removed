




















function test() {
  waitForExplicitFinish();

  let windowsToClose = [];
  let tmpDir = Services.dirsvc.get("TmpD", Ci.nsIFile);

  
  let downloadA;
  const downloadASource = "data:image/png;base64,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";
  const downloadADest = "download-file-A";
  const downloadAName = "download-A";

  
  let downloadB;
  const downloadBSource = "data:application/octet-stream;base64,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";
  const downloadBDest = "download-file-B";
  const downloadBName = "download-B";

    
  let fileA = createFile(downloadADest);
  let fileB = createFile(downloadBDest);

  let listener = {
    onDownloadStateChange: function(aState, aDownload) {
      switch (aDownload.state) {
        case Services.downloads.DOWNLOAD_FAILED:
        case Services.downloads.DOWNLOAD_CANCELED:
        case Services.downloads.DOWNLOAD_DIRTY:
        case Services.downloads.DOWNLOAD_BLOCKED_POLICY:
          
          if (aDownload.targetFile.exists())
            aDownload.targetFile.remove(false);
          Services.downloads.removeListener(this);
          throw ("Download failed (name: " + aDownload.displayName + ", state: " + aDownload.state + ")");
          finish();
          break;
        case Services.downloads.DOWNLOAD_FINISHED:
          ok(aDownload.targetFile.exists(), "Check that targetFile exists");
          aDownload.targetFile.remove(false);
          this.onDownloadFinished();
          break;
      }
    },
    onStateChange: function(a, b, c, d, e) { },
    onProgressChange: function(a, b, c, d, e, f, g) { },
    onSecurityChange: function(a, b, c, d) { },
    onDownloadFinished: function() {
      
      
      
      is(Services.downloads.activeDownloadCount, 0,
        "Check that activeDownloadCount is zero");

      
      testOnWindow(true, function(win) {
        
        downloadB = addDownload({
          isPrivate: true,
          targetFile: fileB,
          sourceURI: downloadBSource,
          downloadName: downloadBName,
          runBeforeStart: function (aDownload) {
            checkDownloads(aDownload);
          }
        });
        
        waitForDownloadState(downloadB,
          Services.downloads.DOWNLOAD_FINISHED, function() {
          testOnWindow(false, function() {
            checkDownloads(downloadB);
            cleanUp();
          });
        });
      });
    }
  };
  Services.downloads.addListener(listener);

  registerCleanupFunction(function() {
    Services.downloads.removeListener(listener);
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  function cleanUp() {
    Services.downloads.removeDownload(downloadA.id);
    if (downloadA.targetFile.exists()) {
      downloadA.targetFile.remove(false);
    }
    if (downloadB.targetFile.exists()) {
      downloadB.targetFile.remove(false);
    }
    Services.downloads.cleanUp();
    is(Services.downloads.activeDownloadCount, 0,
      "Make sure download DB is empty");
    finish();
  }

  function testOnWindow(aIsPrivate, aCallback) {
    whenNewWindowLoaded(aIsPrivate, function(win) {
      windowsToClose.push(win);
      aCallback(win);
    });
  }

  function createFile(aFileName) {
    let file = tmpDir.clone();
    file.append(aFileName);
    file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
    return file;
  }

  function checkDownloads(aDownloadB) {
    
    ok(is_download_available(downloadA.id, downloadASource,
      fileA, downloadAName),
      "Check that Download-A is accessible");

    
    is(Services.downloads.activeDownloadCount, 0,
      "Check that Download-B is not retrievable");

    
    try {
      let downloadID = aDownloadB.id;
      ok(false,
        "Check that Download-B is not available");
    } catch (e) {
      ok(true,
        "Check that Download-B is not available");
    }
  }

  
  is(Services.downloads.activeDownloadCount, 0,
    "Make sure we're starting with an empty DB");

  testOnWindow(false, function(win) {
    
    downloadA = addDownload({
      isPrivate: false,
      targetFile: fileA,
      sourceURI: downloadASource,
      downloadName: downloadAName,
      runBeforeStart: function (aDownload) {
        
        is(Services.downloads.activeDownloadCount, 1,
           "Check that Download-A is retrievable");
        ok(is_active_download_available(aDownload.id, downloadASource, fileA,
          downloadAName, false), "Check that active download (Download-A) is available");
        ok(is_download_available(aDownload.id, downloadASource, fileA,
          downloadAName), "Check that download (Download-A) is available");
      }
    });
  });
}





function is_active_download_available(aID, aSrc, aDst, aName, aPrivate) {
  let enumerator = Services.downloads.activeDownloads;
  while (enumerator.hasMoreElements()) {
    let download = enumerator.getNext();
    if (download.id == aID &&
        download.source.spec == aSrc &&
        download.targetFile.path == aDst.path &&
        download.displayName == aName &&
        download.isPrivate == aPrivate)
      return true;
  }
  return false;
}





function is_download_available(aID, aSrc, aDst, aName) {
  let download;
  try {
    download = Services.downloads.getDownload(aID);
  } catch (ex) {
    return false;
  }
  return (download.id == aID &&
          download.source.spec == aSrc &&
          download.targetFile.path == aDst.path &&
          download.displayName == aName);
}
