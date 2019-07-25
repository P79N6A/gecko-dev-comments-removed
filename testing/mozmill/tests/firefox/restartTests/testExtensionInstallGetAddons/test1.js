





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'ModalDialogAPI', 'UtilsAPI'];

const gTimeout = 5000;
const gSearchTimeout = 30000;
const gInstallTimeout = 30000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();

  persisted.extensionName = "Nightly Tester Tools";
  persisted.extensionId = "{8620c15f-30dc-4dba-a131-7c5d20cf4a29}";

  AddonsAPI.useAmoPreviewUrls();
}

var teardownModule = function(module)
{
  AddonsAPI.resetAmoPreviewUrls();
}




var testInstallExtension = function()
{
  
  addonsManager.open(controller);

  
  addonsManager.search(persisted.extensionName);

  
  var footer = addonsManager.getElement({type: "search_status", subtype: "footer"});
  addonsManager.controller.waitForElement(footer, gSearchTimeout);

  
  var extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.waitThenClick(extension, gSearchTimeout);

  
  var md = new ModalDialogAPI.modalDialog(handleTriggerDialog);
  md.start();

  
  var installButton = addonsManager.getElement({type: "listbox_button", subtype: "installSearchResult", value: extension});
  addonsManager.controller.waitThenClick(installButton);

  
  
  
  addonsManager.controller.waitForEval("subject.manager.paneId == 'installs' || " +
                                       "subject.extension.getAttribute('action') == 'installing'", gInstallTimeout, 100,
                                       {manager: addonsManager, extension: extension.getNode()});
  addonsManager.paneId = "installs";

  
  extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.waitForElement(extension, gInstallTimeout);
  addonsManager.controller.waitForEval("subject.extension.getAttribute('state') == 'success'", gInstallTimeout, 100,
                                       {extension: extension.getNode()});

  
  var restartButton = addonsManager.getElement({type: "notificationBar_buttonRestart"});
  addonsManager.controller.waitForElement(restartButton, gTimeout);
}




var handleTriggerDialog = function(controller) 
{
  
  var itemElem = controller.window.document.getElementById("itemList");
  var itemList = new elementslib.Elem(controller.window.document, itemElem);
  controller.waitForElement(itemList, gTimeout);

  
  controller.assertJS("subject.extensionsCount == 1",
                      {extensionsCount: itemElem.childNodes.length});

  
  controller.assertJS("subject.extensions[0].name == subject.targetName",
                      {extensions: itemElem.childNodes, targetName: persisted.extensionName});

  
  controller.assertJS("subject.isExtensionFromAMO == true",
                      {isExtensionFromAMO: itemElem.childNodes[0].url.indexOf('addons.mozilla.org/') != -1});

  
  var cancelButton = new elementslib.Lookup(controller.window.document,
                                            '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"cancel"}');
  controller.assertNode(cancelButton);

  
  var installButton = new elementslib.Lookup(controller.window.document,
                                             '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitForEval("subject.disabled != true", 7000, 100, installButton.getNode());
  controller.click(installButton);
}





