












































const GTB_ID = 'gtbToolbar';





const gtbTimeout = 5000;





var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}




var testGoogleToolbarInstalled = function()
{
  controller.waitForElement(new elementslib.ID(controller.window.document, GTB_ID), gtbTimeout);
}
