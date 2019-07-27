























function clearAllDoneHelper(getDownloads) {
  var clearedPromise = new Promise(function(resolve, reject) {
    function gotDownloads(downloads) {
      
      if (downloads.length === 0) {
        resolve();
        return;
      }

      
      var expectedIds = new Set();
      function changeHandler(evt) {
        var download = evt.download;
        if (download.state === "finalized") {
          expectedIds.delete(download.id);
          if (expectedIds.size === 0) {
            resolve();
          }
        }
      }
      downloads.forEach(function(download) {
        if (download.state === "downloading") {
          ok(false, "A download is still active: " + download.path);
          reject("Active download");
        }
        download.onstatechange = changeHandler;
        expectedIds.add(download.id);
      });
      navigator.mozDownloadManager.clearAllDone();
    }
    function gotBadNews(err) {
      ok(false, "Problem clearing all downloads: " + err);
      reject(err);
    }
    navigator.mozDownloadManager.getDownloads().then(gotDownloads, gotBadNews);
 });
 if (!getDownloads) {
   return clearedPromise;
 }
 return clearedPromise.then(function() {
   return navigator.mozDownloadManager.getDownloads();
 });
}
