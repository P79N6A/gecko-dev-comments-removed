









function gen_test()
{
  try {
    
    for (let yy in gen_resetState()) yield;

    
    
    DownloadsCommon.data.panelHasShownBefore = false;

    prepareForPanelOpen();
    DownloadsCommon.data._notifyNewDownload();
    yield;

    
    DownloadsPanel.hidePanel();

    ok(DownloadsCommon.data.panelHasShownBefore,
       "Should have recorded that the panel was opened on a download.")

    
    
    panelShouldNotOpen();
    DownloadsCommon.data._notifyNewDownload();
    yield waitFor(2);
  } catch(e) {
    ok(false, e);
  } finally {
    
    for (let yy in gen_resetState()) yield;
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
