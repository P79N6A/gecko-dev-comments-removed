



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['DownloadsAPI'];

var URL = "http://releases.mozilla.org/pub/mozilla.org/firefox/releases/3.6/source/firefox-3.6.source.tar.bz2";

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.dm = new DownloadsAPI.downloadManager();
  
  
  dm.cleanAll();
}

var teardownModule = function(module)
{
  dm.cleanAll();
  dm.close();
}





var testDownloadStates = function()
{
  
  DownloadsAPI.downloadFileOfUnknownType(controller, URL);
  
  
  dm.waitForOpened(controller);
  
  
  var download = dm.getElement({type: "download", subtype: "id", value: "dl1"}); 
  controller.waitForElement(download);
  
  
  var pauseButton = dm.getElement({type: "download_button", subtype: "pause", value: download});
  controller.click(pauseButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.paused);

  
  var resumeButton = dm.getElement({type: "download_button", subtype: "resume", value: download});
  controller.click(resumeButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.downloading);

  
  var cancelButton = dm.getElement({type: "download_button", subtype: "cancel", value: download});
  controller.click(cancelButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.canceled);

  
  var retryButton = dm.getElement({type: "download_button", subtype: "retry", value: download});
  controller.click(retryButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.downloading);
}

