









































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}

var testSampleTestcase = function()
{
  PrefsAPI.preferencesDialog.open(callbackHandler);
}

var callbackHandler = function(controller)
{
  
  PrefsAPI.preferencesDialog.setPane(controller, 'paneMain');
  controller.sleep(gDelay);

  
  PrefsAPI.preferencesDialog.close(controller, true);
}
