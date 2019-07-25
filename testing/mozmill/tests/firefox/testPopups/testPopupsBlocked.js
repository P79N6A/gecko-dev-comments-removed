





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
}

var teardownModule = function(module)
{
  
  PrefsAPI.preferences.clearUserPref("dom.disable_open_during_load");
  tabBrowser.closeAllTabs();

  for each (window in mozmill.utils.getWindows("navigator:browser")) {
    if (!window.toolbar.visible)
      window.close();
  }
}





var testPopUpBlocked = function()
{
  var url = "https://litmus.mozilla.org/testcase_files/firefox/5918/index.html";

  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  
  var windowCount = mozmill.utils.getWindows().length;

  
  controller.open(url);
  controller.waitForPageLoad();

  

  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"popup-blocked"}/anon({"type":"warning"})' +
                                             '/{"class":"messageCloseButton tabbable"}');
  controller.waitForElement(button, gTimeout);

  
  controller.assertJS("subject.preWindowCount == subject.postWindowCount",
                      {preWindowCount: windowCount, postWindowCount: mozmill.utils.getWindows().length});
}







var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneContent';

  
  var pref = new elementslib.ID(controller.window.document, "popupPolicy");
  controller.waitForElement(pref, gTimeout);
  controller.check(pref, true);

  prefDialog.close(true);
}





