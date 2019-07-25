






































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI'];

const gDelay = 0;
const gTimeout = 5000;

const localTestFolder = collector.addHttpResource('./files');

var plugins = {"darwin": "DefaultPlugin.plugin",
               "winnt": "npnul32.dll",
               "linux": "libnullplugin.so"};

var setupModule = function(module) 
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}

var teardownModule = function(module)
{
  addonsManager.close();
}




var testDisableEnablePlugin = function()
{
  var pluginId = plugins[mozmill.platform];

  
  addonsManager.open(controller);

  
  addonsManager.setPluginState("addonID", pluginId, false);

  
  var status = new elementslib.ID(controller.tabs.activeTab, "status");

  controller.open(localTestFolder + "plugin.html");
  controller.waitForPageLoad();
  controller.assertText(status, "disabled");

  
  addonsManager.setPluginState("addonID", pluginId, true);

  
  controller.open(localTestFolder + "plugin.html");
  controller.waitForPageLoad();
  controller.assertText(status, "enabled");
}





