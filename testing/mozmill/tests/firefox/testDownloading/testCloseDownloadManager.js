




































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
  
  if (dm.controller.window)
    dm.controller.window.close();
}




var testCloseDownloadManager = function()
{
  
  var windowCount = mozmill.utils.getWindows().length;

  
  dm.open(controller, false);
  dm._controller.keypress(null, "VK_ESCAPE", {});
  controller.waitForEval("subject.getWindows().length == " + windowCount,
                         gTimeout, 100, mozmill.utils);

  
  
  
  
  
  dm.open(controller, false);
  dm.close();
  
  
  
  
  
  
  if (mozmill.isLinux) {
    dm.open(controller, false);
    dm._controller.keypress(null, 'y', {shiftKey:true, accelKey:true});
    controller.waitForEval("subject.getWindows().length == " + windowCount,
                           gTimeout, 100, mozmill.utils); 
  }

  
  
  
  
  
  if (!mozmill.isLinux) {
    dm.open(controller, false);
    dm._controller.keypress(null, 'j', {accelKey:true});
    controller.waitForEval("subject.getWindows().length == " + windowCount,
                           gTimeout, 100, mozmill.utils); 
  }
}





