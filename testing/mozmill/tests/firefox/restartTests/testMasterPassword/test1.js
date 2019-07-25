





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI','PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
}

var teardownModule = function(module)
{
  
  var pm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  pm.removeAllLogins();
}




var testSetMasterPassword = function()
{
  var testSite = "http://www-archive.mozilla.org/quality/browser/front-end/testcases/wallet/login.html";

  
  controller.open(testSite);
  controller.waitForPageLoad();

  var userField = new elementslib.ID(controller.tabs.activeTab, "uname");
  var passField = new elementslib.ID(controller.tabs.activeTab, "Password");

  controller.waitForElement(userField, gTimeout);
  controller.type(userField, "bar");
  controller.type(passField, "foo");

  controller.waitThenClick(new elementslib.ID(controller.tabs.activeTab, "LogIn"), gTimeout);
  controller.sleep(500);

  
  var label = UtilsAPI.getProperty("chrome://passwordmgr/locale/passwordmgr.properties",
                                   "notifyBarRememberButtonText");
  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"password-save"}/{"label":"' + label + '"}');

  UtilsAPI.assertElementVisible(controller, button, true);
  controller.waitThenClick(button, gTimeout);
  controller.sleep(500);
  controller.assertNodeNotExist(button);

  
  PrefsAPI.openPreferencesDialog(prefDialogSetMasterPasswordCallback);
}




var testInvokeMasterPassword = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogInvokeMasterPasswordCallback);
}




var testRemoveMasterPassword = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogDeleteMasterPasswordCallback);
}






var prefDialogSetMasterPasswordCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  prefDialog.paneId = 'paneSecurity';

  var masterPasswordCheck = new elementslib.ID(controller.window.document, "useMasterPassword");
  controller.waitForElement(masterPasswordCheck, gTimeout);
  controller.sleep(gDelay);

  
  var md = new ModalDialogAPI.modalDialog(masterPasswordHandler);
  md.start(200);

  controller.click(masterPasswordCheck);

  
  prefDialog.close(true);
}






var masterPasswordHandler = function(controller)
{
  var pw1 = new elementslib.ID(controller.window.document, "pw1");
  var pw2 = new elementslib.ID(controller.window.document, "pw2");

  
  controller.waitForElement(pw1, gTimeout);
  controller.type(pw1, "test1");
  controller.type(pw2, "test1");

  
  var md = new ModalDialogAPI.modalDialog(confirmHandler);
  md.start(200);

  var button = new elementslib.Lookup(controller.window.document,
                           '/id("changemp")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitThenClick(button, gTimeout);
}






var confirmHandler = function(controller)
{
  var button = new elementslib.Lookup(controller.window.document,
                               '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.waitThenClick(button, gTimeout);
}






var prefDialogInvokeMasterPasswordCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  prefDialog.paneId = 'paneSecurity';

  var showPasswordButton = new elementslib.ID(controller.window.document, "showPasswords");
  controller.waitForElement(showPasswordButton, gTimeout);

  
  var md = new ModalDialogAPI.modalDialog(checkMasterHandler);
  md.start(200);

  controller.click(showPasswordButton);

  
  controller.sleep(500);

  var window = mozmill.wm.getMostRecentWindow('Toolkit:PasswordManager');
  var pwdController = new mozmill.controller.MozMillController(window);

  var togglePasswords = new elementslib.ID(pwdController.window.document, "togglePasswords");
  var passwordCol = new elementslib.ID(pwdController.window.document, "passwordCol");

  pwdController.waitForElement(togglePasswords, gTimeout);
  UtilsAPI.assertElementVisible(pwdController, passwordCol, false);

  
  var md = new ModalDialogAPI.modalDialog(checkMasterHandler);
  md.start(200);

  pwdController.click(togglePasswords);

  UtilsAPI.assertElementVisible(pwdController, passwordCol, true);

  
  pwdController.keypress(null, "w", {accelKey: true});
  pwdController.sleep(200);

  prefDialog.close(true);
}






var checkMasterHandler = function(controller)
{
  var passwordBox = new elementslib.ID(controller.window.document, "password1Textbox");

  controller.waitForElement(passwordBox, gTimeout);
  controller.type(passwordBox, "test1");

  var button = new elementslib.Lookup(controller.window.document,
                               '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.click(button);
}






var prefDialogDeleteMasterPasswordCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  prefDialog.paneId = 'paneSecurity';

  var masterPasswordCheck = new elementslib.ID(controller.window.document, "useMasterPassword");
  controller.waitForElement(masterPasswordCheck, gTimeout);

  
  var md = new ModalDialogAPI.modalDialog(removeMasterHandler);
  md.start(200);

  controller.click(masterPasswordCheck);

  
  prefDialog.close(true);
}






var removeMasterHandler = function(controller)
{
  var removePwdField = new elementslib.ID(controller.window.document, "password");

  controller.waitForElement(removePwdField, gTimeout);
  controller.type(removePwdField, "test1");

  
  var md = new ModalDialogAPI.modalDialog(confirmHandler);
  md.start(200);

  controller.click(new elementslib.Lookup(controller.window.document,
                   '/id("removemp")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}'));
}







