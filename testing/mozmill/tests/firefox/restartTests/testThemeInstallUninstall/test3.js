





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'PrefsAPI','UtilsAPI'];

const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}




var testCheckThemeChange = function()
{
  addonsManager.open(controller);
  addonsManager.paneId = "themes";

  
  var theme = addonsManager.getListboxItem("addonID", persisted.defaultThemeId);
  addonsManager.controller.waitForElement(theme, gTimeout);
  addonsManager.controller.assertJS("subject.isCurrentTheme == true",
                                    {isCurrentTheme: theme.getNode().getAttribute('current') == 'true'});
}





