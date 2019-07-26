









function test_task()
{
  try {
    
    yield task_resetState();

    
    
    DownloadsCommon.getData(window).panelHasShownBefore = false;

    let promise = promisePanelOpened();
    DownloadsCommon.getData(window)._notifyDownloadEvent("start");
    yield promise;

    
    DownloadsPanel.hidePanel();

    ok(DownloadsCommon.getData(window).panelHasShownBefore,
       "Should have recorded that the panel was opened on a download.")

    
    
    let originalOnPopupShown = DownloadsPanel.onPopupShown;
    DownloadsPanel.onPopupShown = function () {
      originalOnPopupShown.apply(this, arguments);
      ok(false, "Should not have opened the downloads panel.");
    };

    try {
      DownloadsCommon.getData(window)._notifyDownloadEvent("start");

      
      let deferTimeout = Promise.defer();
      setTimeout(deferTimeout.resolve, 2000);
      yield deferTimeout.promise;
    } finally {
      DownloadsPanel.onPopupShown = originalOnPopupShown;
    }
  } finally {
    
    yield task_resetState();
  }
}
