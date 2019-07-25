





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}

var testSampleTestcase = function()
{
  var md = new ModalDialogAPI.modalDialog(callbackHandler);
  md.start();

  
}

var callbackHandler = function(controller)
{
  
}
