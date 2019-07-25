





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'ModalDialogAPI', 'UtilsAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}

var testCheckInstalledExtension = function() 
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




var testUninstallExtension = function()
{
  
  addonsManager.waitForOpened(controller);
  addonsManager.paneId = "extensions";

  
  var extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.waitThenClick(extension, gTimeout);

  
  var md = new ModalDialogAPI.modalDialog(handleTriggerDialog);
  md.start();

  var uninstallButton = addonsManager.getElement({type: "listbox_button", subtype: "uninstall", value: extension});
  addonsManager.controller.waitThenClick(uninstallButton, gTimeout);
 
  
  var restartButton = addonsManager.getElement({type: "notificationBar_buttonRestart"});
  addonsManager.controller.waitForElement(restartButton, gTimeout);
}




var handleTriggerDialog = function(controller)
{
  var cancelButton = new elementslib.Lookup(controller.window.document,
                                            '/id("addonList")/anon({"anonid":"buttons"})/{"dlgtype":"cancel"}');
  controller.waitForElement(cancelButton, gTimeout);

  var uninstallButton = new elementslib.Lookup(controller.window.document,
                                               '/id("addonList")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitForEval("subject.disabled != true", 7000, 100, uninstallButton.getNode());
  controller.waitThenClick(uninstallButton, gTimeout);
}






