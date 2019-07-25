





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI'];

const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  module.addonsManager = new AddonsAPI.addonsManager();
}

var testCheckUninstalledExtension = function()
{
  addonsManager.open(controller);
  addonsManager.paneId = "extensions";

  
  var extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.sleep(100);
  addonsManager.controller.assertNodeNotExist(extension);
}





