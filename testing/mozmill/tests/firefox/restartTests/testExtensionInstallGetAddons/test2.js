





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI'];

const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}

var testCheckExtensionInstalled = function()
{
  addonsManager.waitForOpened(controller);

  
  addonsManager.controller.waitForEval("subject.manager.paneId == 'extensions'", 10000, 100,
                                       {manager: addonsManager});

  
  var notificationBar = addonsManager.getElement({type: "notificationBar"});
  addonsManager.controller.waitForElement(notificationBar, gTimeout);

  
  
  var extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.waitForElement(extension, gTimeout);
  addonsManager.controller.assertJS("subject.isExtensionInstalled == true",
                                    {isExtensionInstalled: extension.getNode().getAttribute('newAddon') == 'true'});
}





