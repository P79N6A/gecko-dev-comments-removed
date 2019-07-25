




































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['SoftwareUpdateAPI', 'UtilsAPI'];

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.update = new SoftwareUpdateAPI.softwareUpdate();
}




var testFallbackUpdate_ErrorPatching = function()
{
  
  update.waitForDialogOpen(controller);
  update.assertUpdateStep('errorpatching');
  controller.window.focus();

  
  update.download(persisted.type, persisted.channel);

  
  update.assertUpdateStep('finished');
}
