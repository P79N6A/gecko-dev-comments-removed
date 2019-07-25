





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testOpenCloseOptionsDialog = function()
{
  
  PrefsAPI.openPreferencesDialog(prefPaneResetCallback);
}




var testOptionsDialogRetention = function()
{
  
  PrefsAPI.openPreferencesDialog(prefPaneSetCallback);

  
  PrefsAPI.openPreferencesDialog(prefPaneCheckCallback);
}







var prefPaneResetCallback = function(controller)
{
  let prefDialog = new PrefsAPI.preferencesDialog(controller);

  prefDialog.paneId = 'paneMain';
  prefDialog.close();
}







var prefPaneSetCallback = function(controller)
{
  let prefDialog = new PrefsAPI.preferencesDialog(controller);

  prefDialog.paneId = 'paneAdvanced';
  prefDialog.paneId = 'panePrivacy';
  prefDialog.close();
}







var prefPaneCheckCallback = function(controller)
{
  let prefDialog = new PrefsAPI.preferencesDialog(controller);

  controller.assertJS("subject.paneId == 'panePrivacy'",
                      {paneId: prefDialog.paneId});
  prefDialog.close();
}






