




































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'ModalDialogAPI', 'TabbedBrowsingAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();

  persisted.url = "https://preview.addons.mozilla.org/de/firefox/addon/6543/";
  persisted.extensionName = "Nightly Tester Tools";
  persisted.extensionId = "{8620c15f-30dc-4dba-a131-7c5d20cf4a29}";

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var testInstallExtension = function() {
  addonsManager.open(controller);
  addonsManager.paneId = "search";

  
  var browseAllAddons = addonsManager.getElement({type: "link_browseAddons"});
  addonsManager.controller.waitThenClick(browseAllAddons, gTimeout);

  
  controller.waitForEval("subject.tabs.length == 2", gTimeout, 100, controller);
  controller.waitForPageLoad();

  
  controller.open(persisted.url);
  controller.waitForPageLoad();

  
  var md = new ModalDialogAPI.modalDialog(handleTriggerDialog);
  md.start();

  
  var triggerLink = new elementslib.XPath(controller.tabs.activeTab,
                                          "//div[@id='addon-summary']/div/div/div/p/a/span");
  controller.waitForElement(triggerLink, gTimeout);
  controller.click(triggerLink, triggerLink.getNode().width / 2, triggerLink.getNode().height / 2);

  
  addonsManager.controller.waitForEval("subject.manager.paneId == 'installs'", 10000, 100,
                                       {manager: addonsManager});

  
  var extension = addonsManager.getListboxItem("addonID", persisted.extensionId);
  addonsManager.controller.waitForElement(extension, gTimeout);

  var extensionName = extension.getNode().getAttribute('name');
  addonsManager.controller.assertJS("subject.isValidExtensionName == true",
                                    {isValidExtensionName: extensionName == persisted.extensionName});

  
  var restartButton = addonsManager.getElement({type: "notificationBar_buttonRestart"});
  addonsManager.controller.waitForElement(restartButton, gTimeout);
}




var handleTriggerDialog = function(controller) {
  
  var itemElem = controller.window.document.getElementById("itemList");
  var itemList = new elementslib.Elem(controller.window.document, itemElem);
  controller.waitForElement(itemList, gTimeout);

  
  controller.assertJS("subject.extensions.length == 1",
                      {extensions: itemElem.childNodes});

  
  controller.assertJS("subject.extensions[0].name == subject.extensionName",
                      {extensions: itemElem.childNodes, extensionName: persisted.extensionName});

  
  controller.assertJS("subject.isExtensionFromAMO == true",
                      {isExtensionFromAMO: itemElem.childNodes[0].url.indexOf('addons.mozilla.org') != -1});

  
  var cancelButton = new elementslib.Lookup(controller.window.document,
                                            '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"cancel"}');
  controller.assertNode(cancelButton);

  
  var installButton = new elementslib.Lookup(controller.window.document,
                                             '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitForEval("subject.disabled != true", 7000, 100, installButton.getNode());
  controller.click(installButton);
}





