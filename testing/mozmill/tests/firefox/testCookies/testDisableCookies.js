





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  module.cm = Cc["@mozilla.org/cookiemanager;1"]
                 .getService(Ci.nsICookieManager2);
  cm.removeAll();
}

var teardownModule = function(module)
{
  PrefsAPI.preferences.clearUserPref("network.cookie.cookieBehavior");
  cm.removeAll();
}




var testDisableCookies = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDisableCookieDialogCallback);

  
  controller.open("http://www.mozilla.org/");
  controller.waitForPageLoad();

  
  PrefsAPI.openPreferencesDialog(prefCheckDisableDialogCallback);
}






var prefDisableCookieDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode, gTimeout);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  
  var acceptCookiesPref = new elementslib.ID(controller.window.document, "acceptCookies");
  controller.check(acceptCookiesPref, false);

  
  prefDialog.close(true);
}






var prefCheckDisableDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode, gTimeout);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  var showCookies = new elementslib.ID(controller.window.document, "showCookiesButton");
  controller.waitThenClick(showCookies, gTimeout);
  controller.sleep(500);

  try {
    
    var window = mozmill.wm.getMostRecentWindow('Browser:Cookies');
    var cmController = new mozmill.controller.MozMillController(window);
  
    
    var removeCookieButton = new elementslib.ID(cmController.window.document, "removeCookie");
    cmController.waitThenClick(removeCookieButton, gTimeout);
  
    
    
    cmController.assertJS("subject.cookieCount == 0",
                          {cookieCount : cm.countCookiesFromHost(".mozilla.org")});
  } catch (ex) {
    throw ex;
  } finally {
    
    cmController.keypress(null, "w", {accelKey: true});
    controller.sleep(200);
  }

  prefDialog.close(true);
}





