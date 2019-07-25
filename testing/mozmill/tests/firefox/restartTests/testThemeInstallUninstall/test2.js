





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'PrefsAPI', 'UtilsAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}




var testCheckInstalledTheme = function()
{
  addonsManager.open(controller);
  addonsManager.paneId = "themes";

  
  var theme = addonsManager.getListboxItem("addonID", persisted.themeId);
  addonsManager.controller.waitThenClick(theme, gTimeout);
  addonsManager.controller.assertJS("subject.isCurrentTheme == true",
                                    {isCurrentTheme: theme.getNode().getAttribute('current') == 'true'});

  addonsManager.close();
}




var testThemeChange = function() 
{
  addonsManager.open(controller);
  addonsManager.paneId = "themes";

  
  var theme = addonsManager.getListboxItem("addonID", persisted.defaultThemeId);
  addonsManager.controller.waitThenClick(theme, gTimeout);

  var useThemeButton = addonsManager.getElement({type: "listbox_button", subtype: "useTheme", value: theme});
  addonsManager.controller.waitThenClick(useThemeButton, gTimeout);

  
  var restartButton = addonsManager.getElement({type: "notificationBar_buttonRestart"});
  addonsManager.controller.waitForElement(restartButton, gTimeout);

  
  addonsManager.controller.assertProperty(useThemeButton, "disabled", "true");

  
  var description = theme.getNode().getAttribute('description');
  addonsManager.controller.assertJS("subject.hasDescriptionChanged == true",
                                    {hasDescriptionChanged: description.indexOf('Restart') != -1});
}






