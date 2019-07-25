





































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




var testRemoveAllCookies = function()
{
  
  controller.open("http://www.mozilla.org/");
  controller.waitForPageLoad();

  controller.open("http://www.amazon.com/");
  controller.waitForPageLoad();

  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);
}






var prefDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode, gTimeout);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  controller.waitThenClick(new elementslib.ID(controller.window.document, "showCookiesButton"), gTimeout);
  controller.sleep(500);

  try {
    
    var window = mozmill.wm.getMostRecentWindow('Browser:Cookies');
    var cmController = new mozmill.controller.MozMillController(window);

    
    var cookiesList = cmController.window.document.getElementById("cookiesList");
    cmController.assertJS("subject.cookieCount > 0",
                          {cookieCount : cookiesList.view.rowCount});

    
    cmController.waitThenClick(new elementslib.ID(cmController.window.document, "removeAllCookies"), gTimeout);
    cmController.assertJS("subject.cookieCount == 0",
                          {cookieCount : cookiesList.view.rowCount});
  } catch (ex) {
    throw ex;
  } finally {
    
    cmController.keypress(null, "w", {accelKey: true});
    controller.sleep(200);
  }

  prefDialog.close(true);
}





