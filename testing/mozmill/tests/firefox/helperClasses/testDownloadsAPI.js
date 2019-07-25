



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['DownloadsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.dm = new DownloadsAPI.downloadManager();
}

var teardownModule = function(module)
{
  
  dm.cancelActiveDownloads();
  dm.close();
}

var testOpenDownloadManager = function()
{
  var url = "ftp://ftp.mozilla.org/pub/firefox/releases/3.6/mac/en-US/Firefox%203.6.dmg";

  DownloadsAPI.downloadFileOfUnknownType(controller, url);

  
  dm.open(controller, true);

  
  var download = dm.getElement({type: "download", subtype: "state", value : "0"});
  dm.controller.waitForElement(download, gTimeout);

  var pauseButton = dm.getElement({type: "download_button", subtype: "pause", value: download});
  var resumeButton = dm.getElement({type: "download_button", subtype: "resume", value: download});
  
  
  dm.controller.click(pauseButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.paused);

  
  dm.controller.click(resumeButton);
  dm.waitForDownloadState(download, DownloadsAPI.downloadState.downloading);
}
