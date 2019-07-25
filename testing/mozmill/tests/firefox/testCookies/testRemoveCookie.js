




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  module.cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  cm.removeAll();
}

var teardownModule = function(module)
{
  cm.removeAll();
}




var testRemoveCookie = function()
{
  
  controller.open("http://www.mozilla.org/");
  controller.waitForPageLoad();

  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);
}






var prefDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode);
  controller.select(historyMode, null, null, "custom");
  controller.sleep(gDelay);

  controller.waitThenClick(new elementslib.ID(controller.window.document, "showCookiesButton"), gTimeout);
  controller.sleep(500);

  try {
    
    var window = mozmill.wm.getMostRecentWindow('Browser:Cookies');
    var cmController = new mozmill.controller.MozMillController(window);
  
    
    var filterField = new elementslib.ID(cmController.window.document, "filter");
    cmController.waitForElement(filterField, gTimeout);
    cmController.type(filterField, "__utmz");
    cmController.sleep(500);
  
    
    var cookiesList = cmController.window.document.getElementById("cookiesList");
    var origNumCookies = cookiesList.view.rowCount;
  
    cmController.click(new elementslib.ID(cmController.window.document, "removeCookie"));
  
    cmController.assertJS("subject.isCookieRemoved == true",
                          {isCookieRemoved: !cm.cookieExists({host: ".mozilla.org", name: "__utmz", path: "/"})});
    cmController.assertJS("subject.list.view.rowCount == subject.numberCookies",
                          {list: cookiesList, numberCookies: origNumCookies - 1});
  } catch (ex) {
    throw ex;
  } finally {
    
    cmController.keypress(null, "w", {accelKey: true});
    controller.sleep(200);
  }

  prefDialog.close(true);
}





