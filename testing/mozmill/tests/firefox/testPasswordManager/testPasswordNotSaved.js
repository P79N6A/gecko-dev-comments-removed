





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var testSite = "http://www-archive.mozilla.org/quality/browser/front-end/testcases/wallet/login.html";

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);

  module.pm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  pm.removeAllLogins();
}

var teardownModule = function(module) {
  
  pm.removeAllLogins();
}




var testPasswordNotificationBar = function() {
  
  controller.open(testSite);
  controller.waitForPageLoad();

  var userField = new elementslib.ID(controller.tabs.activeTab, "uname");
  var passField = new elementslib.ID(controller.tabs.activeTab, "Password");

  controller.waitForElement(userField, gTimeout);
  controller.type(userField, "bar");
  controller.type(passField, "foo");

  
  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"password-save"}/anon({"type":"info"})' +
                                             '/{"class":"messageCloseButton tabbable"}');

  
  controller.assertNodeNotExist(button);

  controller.click(new elementslib.ID(controller.tabs.activeTab, "LogIn"));
  controller.waitForPageLoad();

  controller.waitThenClick(button, gTimeout);
  controller.sleep(500);
  controller.assertNodeNotExist(button);
}




var testPasswordNotSaved = function()
{
  
  controller.open(testSite);
  controller.waitForPageLoad();

  var userField = new elementslib.ID(controller.tabs.activeTab, "uname");
  var passField = new elementslib.ID(controller.tabs.activeTab, "Password");

  controller.waitForElement(userField, gTimeout);
  controller.assertValue(userField, "");
  controller.assertValue(passField, "");

  
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

  var filterField = new elementslib.ID(pwdController.window.document, "filter");
  pwdController.waitForElement(filterField, gTimeout);

  var removeLogin = new elementslib.ID(pwdController.window.document, "removeSignon");
  pwdController.assertProperty(removeLogin, 'disabled', 'true');

  
  pwdController.keypress(null, "W", {accelKey: true});

  prefDialog.close(true);
}






