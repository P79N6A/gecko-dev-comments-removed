









function gen_test()
{
  try {
    
    for (let yy in gen_resetState(DownloadsCommon.getData(window))) yield undefined;

    
    
    DownloadsCommon.getData(window).panelHasShownBefore = false;

    prepareForPanelOpen();
    DownloadsCommon.getData(window)._notifyDownloadEvent("start");
    yield undefined;

    
    DownloadsPanel.hidePanel();

    ok(DownloadsCommon.getData(window).panelHasShownBefore,
       "Should have recorded that the panel was opened on a download.")

    
    
    panelShouldNotOpen();
    DownloadsCommon.getData(window)._notifyDownloadEvent("start");
    yield waitFor(2);
  } catch(e) {
    ok(false, e);
  } finally {
    
    for (let yy in gen_resetState(DownloadsCommon.getData(window))) yield undefined;
  }
}





function panelShouldNotOpen()
{
  
  let originalOnViewLoadCompleted = DownloadsPanel.onViewLoadCompleted;
  DownloadsPanel.onViewLoadCompleted = function () {
    DownloadsPanel.onViewLoadCompleted = originalOnViewLoadCompleted;
    ok(false, "Should not have opened the downloads panel.");
  };
}
