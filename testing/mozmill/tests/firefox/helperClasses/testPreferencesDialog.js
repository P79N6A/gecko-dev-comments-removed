




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var testPrefHelperClass = function () {
  PrefsAPI.openPreferencesDialog(handlePrefDialog);
}







function handlePrefDialog(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  var pane = prefDialog.pane;
  prefDialog.paneId = 'paneContent';

  prefDialog.close(true);
}
