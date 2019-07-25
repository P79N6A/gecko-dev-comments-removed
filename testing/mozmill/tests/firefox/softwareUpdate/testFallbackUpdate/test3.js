




































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['SoftwareUpdateAPI', 'UtilsAPI'];

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.update = new SoftwareUpdateAPI.softwareUpdate();

  
  module.persisted.postBuildId = UtilsAPI.appInfo.buildID;
  module.persisted.postLocale = UtilsAPI.appInfo.locale;
  module.persisted.postUserAgent = UtilsAPI.appInfo.userAgent;
  module.persisted.postVersion = UtilsAPI.appInfo.version;
}





var testFallbackUpdate_AppliedAndNoUpdatesFound = function()
{
  
  update.openDialog(controller);
  update.waitForCheckFinished();

  
  try {
    update.assertUpdateStep('noupdatesfound');
  } catch (ex) {
    
    controller.assertJS("subject.newUpdateType != subject.lastUpdateType",
                        {newUpdateType: update.updateType, lastUpdateType: persisted.type});
  }

  
  
  var vc = Cc["@mozilla.org/xpcom/version-comparator;1"]
              .getService(Ci.nsIVersionComparator);
  var check = vc.compare(persisted.postVersion, persisted.preVersion);

  controller.assertJS("subject.newVersionGreater == true",
                      {newVersionGreater: check >= 0});

  controller.assertJS("subject.postBuildId > subject.preBuildId",
                      {postBuildId: persisted.postBuildId, preBuildId: persisted.preBuildId});
  
  
  controller.assertJS("subject.postLocale == subject.preLocale",
                      {postLocale: persisted.postLocale, preLocale: persisted.preLocale});

  
  persisted.success = true;
}





