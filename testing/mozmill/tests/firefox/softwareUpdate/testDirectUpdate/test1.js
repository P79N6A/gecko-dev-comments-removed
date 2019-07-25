




































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['SoftwareUpdateAPI', 'UtilsAPI'];

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.update = new SoftwareUpdateAPI.softwareUpdate();

  
  module.persisted.preBuildId = UtilsAPI.appInfo.buildID;
  module.persisted.preLocale = UtilsAPI.appInfo.locale;
  module.persisted.preUserAgent = UtilsAPI.appInfo.userAgent;
  module.persisted.preVersion = UtilsAPI.appInfo.version;
}

var teardownModule = function(module)
{
  
  module.persisted.updateBuildId = update.activeUpdate.buildID;
  module.persisted.updateType = update.isCompleteUpdate ? "complete" : "partial";
  module.persisted.updateVersion = update.activeUpdate.version;
}




var testDirectUpdate_Download = function()
{
  
  controller.assertJS("subject.isUpdateAllowed == true",
                      {isUpdateAllowed: update.allowed});

  
  update.openDialog(controller);
  update.waitForCheckFinished();

  
  update.assertUpdateStep('updatesfound');

  
  update.download(persisted.type, persisted.channel);

  
  update.assertUpdateStep('finished');
}





