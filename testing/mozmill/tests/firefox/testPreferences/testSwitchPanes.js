









































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gDelay = 100;
const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}




var testPreferencesPanes = function() {
  PrefsAPI.openPreferencesDialog(prefDialogCallback);
}







var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  
  var panes = [
               "paneMain", "paneTabs", "paneContent", "paneApplications",
               "panePrivacy", "paneSecurity", "paneAdvanced"
              ];

  
  for each (pane in panes) {
    prefDialog.paneId = pane;
    controller.sleep(gDelay);
  }

  
  prefDialog.close();
}





