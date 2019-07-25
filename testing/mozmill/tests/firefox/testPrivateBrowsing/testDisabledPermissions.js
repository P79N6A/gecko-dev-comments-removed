



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'PrefsAPI', 'PrivateBrowsingAPI',
                       'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var websites = [
                "https://litmus.mozilla.org/testcase_files/firefox/5918/index.html",
                "https://litmus.mozilla.org/testcase_files/firefox/cookies/cookie_single.html"
               ];

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
}

var teardownModule = function(module)
{
  pb.reset();

  
  PrefsAPI.preferences.clearUserPref("network.cookie.lifetimePolicy");
}




var testPermissionsDisabled = function()
{
  
  pb.enabled = false;
  pb.showPrompt = false;

  TabbedBrowsingAPI.closeAllTabs(controller);

  pb.start();

  
  controller.open(websites[0]);
  controller.waitForPageLoad();

  
  var property = mozmill.isWindows ? "popupWarningButton.accesskey" : "popupWarningButtonUnix.accesskey";
  var accessKey = UtilsAPI.getProperty("chrome://browser/locale/browser.properties", property);

  controller.keypress(null, accessKey, {ctrlKey: mozmill.isMac, altKey: !mozmill.isMac});

  var allow = new elementslib.XPath(controller.window.document, "/*[name()='window']/*[name()='popupset'][1]/*[name()='popup'][2]/*[name()='menuitem'][1]");

  controller.waitForElement(allow);
  controller.assertProperty(allow, "disabled", true);

  controller.keypress(null, "VK_ESCAPE", {});

  
  PrefsAPI.openPreferencesDialog(prefCookieHandler);

  
  controller.open(websites[1]);
  controller.waitForPageLoad();
}







var prefCookieHandler = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode, gTimeout);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  var acceptCookies = new elementslib.ID(controller.window.document, "acceptCookies");
  controller.assertChecked(acceptCookies);

  
  var keepCookies = new elementslib.ID(controller.window.document, "keepCookiesUntil");
  controller.waitForElement(keepCookies, gTimeout);
  controller.select(keepCookies, null, null, 1);
  controller.sleep(gDelay);

  prefDialog.close(true);
}




var cookieHandler = function(controller)
{
  var button = new elementslib.ID(controller.window.document, "ok");
  controller.assertNodeNotExist(button);
}





