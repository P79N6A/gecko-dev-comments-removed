



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI', 'ModalDialogAPI'];

const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
}




var testCrashRemoveCookieAfterPrivateBrowsingMode = function()
{
  var pb = new PrivateBrowsingAPI.privateBrowsing(controller);

  pb.enabled = false;
  pb.showPrompt = false;

  pb.start();
  pb.stop();

  pb.showPrompt = true;
  pb.enabled = false;

  
  var md = new ModalDialogAPI.modalDialog(clearHistoryHandler);
  md.start();

  controller.click(new elementslib.Elem(controller.menus["tools-menu"].sanitizeItem));
}







var clearHistoryHandler = function(controller)
{
  var clearButton = new elementslib.Lookup(controller.window.document, '/id("SanitizeDialog")/anon({"anonid":"dlg-buttons"})/{"dlgtype":"accept"}');
  controller.waitThenClick(clearButton);
}
