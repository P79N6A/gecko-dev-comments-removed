





































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
  cm.removeAll();
}




var testEnableCookies = function()
{
  
  PrefsAPI.openPreferencesDialog(prefEnableCookieDialogCallback);

  
  controller.open("http://www.mozilla.org/");
  controller.waitForPageLoad();

  
  PrefsAPI.openPreferencesDialog(prefCheckEnableDialogCallback);
}






var prefEnableCookieDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  
  var acceptCookiesPref = new elementslib.ID(controller.window.document, "acceptCookies");
  controller.check(acceptCookiesPref, true);

  
  prefDialog.close(true);
}






var prefCheckEnableDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  controller.waitThenClick(new elementslib.ID(controller.window.document, "showCookiesButton"), gTimeout);
  controller.sleep(500);

  try {
    
    var window = mozmill.wm.getMostRecentWindow('Browser:Cookies');
    var cmController = new mozmill.controller.MozMillController(window);
  
    
    var removeCookieButton = new elementslib.ID(cmController.window.document, "removeCookie");
    cmController.waitForElement(removeCookieButton, gTimeout);
    cmController.assertProperty(removeCookieButton, "disabled", false);
  
    cmController.assertJS("subject.cookieExists == true",
                          {cookieExists: cm.cookieExists({host: ".mozilla.org", name: "__utmz", path: "/"})});
    cmController.assertJS("subject.cookieCount > 0",
                          {cookieCount : cm.countCookiesFromHost(".mozilla.org")});
  } catch (ex) {
    throw ex;
  } finally {
    
    cmController.keypress(null, "w", {accelKey: true});
    controller.sleep(200);
  }

  prefDialog.close(true);
}





