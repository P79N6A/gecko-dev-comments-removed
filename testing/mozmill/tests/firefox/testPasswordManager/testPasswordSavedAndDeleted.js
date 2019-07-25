





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);

  module.pm = Cc["@mozilla.org/login-manager;1"]
                 .getService(Ci.nsILoginManager);
  pm.removeAllLogins();
}

var teardownModule = function(module)
{
  
  pm.removeAllLogins();
}




var testSavePassword = function()
{
  var testSite = "http://www-archive.mozilla.org/quality/browser/front-end/testcases/wallet/login.html";

  
  controller.open(testSite);
  controller.waitForPageLoad();

  var userField = new elementslib.ID(controller.tabs.activeTab, "uname");
  var passField = new elementslib.ID(controller.tabs.activeTab, "Password");

  controller.waitForElement(userField, gTimeout);
  controller.type(userField, "bar");
  controller.type(passField, "foo");

  controller.click(new elementslib.ID(controller.tabs.activeTab, "LogIn"));
  controller.sleep(500);

  
  var label = UtilsAPI.getProperty("chrome://passwordmgr/locale/passwordmgr.properties", "notifyBarRememberButtonText");
  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"password-save"}/{"label":"' + label + '"}');

  controller.waitThenClick(button, gTimeout);
  controller.sleep(500);
  controller.assertNodeNotExist(button);

  
  controller.open(testSite);
  controller.waitForPageLoad();

  controller.waitForElement(userField, gTimeout);
  controller.assertValue(userField, "bar");
  controller.assertValue(passField, "foo");
}




var testDeletePassword = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);
}






var prefDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneSecurity';

  controller.waitThenClick(new elementslib.ID(controller.window.document, "showPasswords"), gTimeout);
  controller.sleep(500);

  
  var window = mozmill.wm.getMostRecentWindow('Toolkit:PasswordManager');
  var pwdController = new mozmill.controller.MozMillController(window);
  var signOnsTree = pwdController.window.document.getElementById("signonsTree");

  
  pwdController.assertJS("subject.view.rowCount == 1", signOnsTree);

  
  var md = new ModalDialogAPI.modalDialog(confirmHandler);
  md.start(200);

  pwdController.click(new elementslib.ID(pwdController.window.document, "removeAllSignons"));

  pwdController.assertJS("subject.view.rowCount == 0", signOnsTree);

  
  pwdController.keypress(null, "w", {accelKey:true});
  controller.sleep(200);

  
  prefDialog.close(true);
}






var confirmHandler = function(controller)
{
  controller.waitThenClick(new elementslib.Lookup(controller.window.document,
                           '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}'), gTimeout);
}






