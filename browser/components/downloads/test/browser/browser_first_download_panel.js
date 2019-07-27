









add_task(function* test_first_download_panel() {
  
  
  let oldPrefValue = Services.prefs.getBoolPref("browser.download.panel.shown");
  Services.prefs.setBoolPref("browser.download.panel.shown", false);

  registerCleanupFunction(function*() {
    
    yield task_resetState();

    
    
    
    Services.prefs.setBoolPref("browser.download.panel.shown", oldPrefValue);
  });

  
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

  DownloadsCommon.getData(window)._notifyDownloadEvent("start");

  
  yield new Promise(resolve => setTimeout(resolve, 2000));
  DownloadsPanel.onPopupShown = originalOnPopupShown;
});
