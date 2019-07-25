




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'PrefsAPI',
                       'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;


var modalWarningShown = false;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
}

var teardownModule = function(module)
{
  var prefs = new Array("security.warn_entering_secure",
                        "security.warn_entering_weak",
                        "security.warn_leaving_secure",
                        "security.warn_submit_insecure",
                        "security.warn_viewing_mixed");

  
  for each (p in prefs)
    PrefsAPI.preferences.clearUserPref(p);
}




var testEncryptedPageWarning = function()
{
  
  
  
  TabbedBrowsingAPI.closeAllTabs(controller);

  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  
  var md = new ModalDialogAPI.modalDialog(handleSecurityWarningDialog);
  md.start();

  
  controller.open("https://www.verisign.com");

  
  controller.waitForPageLoad();

  
  controller.assertJS("subject.isModalWarningShown == true",
                      {isModalWarningShown: modalWarningShown});
}







var prefDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneSecurity';

  
  var warningSettingsButton = new elementslib.ID(controller.window.document,
                                                 "warningSettings");
  controller.waitForElement(warningSettingsButton, gTimeout);

  
  var md = new ModalDialogAPI.modalDialog(handleSecurityWarningSettingsDialog);
  md.start(500);

  controller.click(warningSettingsButton);

  
  prefDialog.close(true);
}








var handleSecurityWarningSettingsDialog = function(controller)
{
  var prefs = new Array("warn_entering_secure",
                        "warn_entering_weak",
                        "warn_leaving_secure",
                        "warn_submit_insecure",
                        "warn_viewing_mixed");

  
  for each (p in prefs) {
    var element = new elementslib.ID(controller.window.document, p);
    controller.waitForElement(element, gTimeout);
    controller.check(element, (p == "warn_entering_secure"));
  }

  
  var okButton = new elementslib.Lookup(controller.window.document,
                                        '/id("SecurityWarnings")' +
                                        '/anon({"anonid":"dlg-buttons"})' +
                                        '/{"dlgtype":"accept"}');
  controller.click(okButton);
}




var handleSecurityWarningDialog = function(controller)
{
  modalWarningShown = true;

  var enterSecureMessage = UtilsAPI.getProperty("chrome://pipnss/locale/security.properties",
                                                "EnterSecureMessage");

  
  var infoBody = new elementslib.ID(controller.window.document, "info.body");
  controller.waitForElement(infoBody);

  
  controller.assertProperty(infoBody, "textContent", enterSecureMessage);

  
  var checkbox = new elementslib.ID(controller.window.document, "checkbox");
  controller.assertChecked(checkbox);

  
  var okButton = new elementslib.Lookup(controller.window.document,
                                        '/id("commonDialog")' +
                                        '/anon({"anonid":"buttons"})' +
                                        '/{"dlgtype":"accept"}');
  controller.waitThenClick(okButton);
}





