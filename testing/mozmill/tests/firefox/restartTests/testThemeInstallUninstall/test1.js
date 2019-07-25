




































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'ModalDialogAPI', 'TabbedBrowsingAPI'];

const gTimeout = 5000;
const gDownloadTimeout = 60000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();

  persisted.themeName = "Walnut for Firefox";
  persisted.themeId = "{5A170DD3-63CA-4c58-93B7-DE9FF536C2FF}";
  persisted.defaultThemeId = "{972ce4c6-7e08-4474-a285-3208198ce6fd}";

  TabbedBrowsingAPI.closeAllTabs(controller);
}




var testInstallTheme = function() 
{
  addonsManager.open(controller);
  addonsManager.paneId = "search";

  
  var browseAllAddons = addonsManager.getElement({type: "link_browseAddons"});
  addonsManager.controller.waitThenClick(browseAllAddons, gTimeout);

  
  controller.waitForEval("subject.tabs.length == 2", gTimeout, 100, controller);
  controller.waitForPageLoad();

  
  controller.open("https://preview.addons.mozilla.org/en-US/firefox/addon/122");
  controller.waitForPageLoad();

  
  var md = new ModalDialogAPI.modalDialog(handleTriggerDialog);
  md.start();

  
  var triggerLink = new elementslib.XPath(controller.tabs.activeTab,
                                          "//div[@id='addon-summary']/div/div/div/p/a/span");
  controller.waitThenClick(triggerLink, gTimeout);

  
  addonsManager.controller.waitForEval("subject.manager.paneId == 'installs'", 10000, 100,
                                       {manager: addonsManager});

  
  var theme = addonsManager.getListboxItem("addonID", persisted.themeId);
  addonsManager.controller.waitForElement(theme, gDownloadTimeout);

  var themeName = theme.getNode().getAttribute('name');
  addonsManager.controller.assertJS("subject.isValidThemeName == true",
                                    {isValidThemeName: themeName == persisted.themeName});

  addonsManager.controller.assertJS("subject.isThemeInstalled == true",
                                    {isThemeInstalled: theme.getNode().getAttribute('state') == 'success'});

  
  var restartButton = addonsManager.getElement({type: "notificationBar_buttonRestart"});
  addonsManager.controller.waitForElement(restartButton, gTimeout);
}




var handleTriggerDialog = function(controller) 
{
  
  var itemElem = controller.window.document.getElementById("itemList");
  var itemList = new elementslib.Elem(controller.window.document, itemElem);
  controller.waitForElement(itemList, gTimeout);

  
  controller.assertJS("subject.themes.length == 1",
                      {themes: itemElem.childNodes});

  
  controller.assertJS("subject.name == '" + persisted.themeName + "'",
                      itemElem.childNodes[0]);

  
  controller.assertJS("subject.url.indexOf('addons.mozilla.org/') != -1",
                      itemElem.childNodes[0]);

  
  var cancelButton = new elementslib.Lookup(controller.window.document,
                                            '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"cancel"}');
  controller.assertNode(cancelButton);

  
  var installButton = new elementslib.Lookup(controller.window.document,
                                             '/id("xpinstallConfirm")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitForEval("subject.disabled != true", undefined, 100, installButton.getNode());
  controller.click(installButton);
}





