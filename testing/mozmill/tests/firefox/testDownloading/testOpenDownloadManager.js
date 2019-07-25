



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['DownloadsAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();

  
  module.dm = new DownloadsAPI.downloadManager();
}

var teardownModule = function(module)
{
  
  dm.close(true);
}




var testOpenDownloadManager = function()
{
  
  dm.open(controller, false);
  dm.close();

  
  dm.open(controller, true);
  dm.close();
}





