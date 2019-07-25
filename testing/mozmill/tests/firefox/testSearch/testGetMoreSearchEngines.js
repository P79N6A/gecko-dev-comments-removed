




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'SearchAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

const searchEngine = {name: "SearchGeek",
                      url : "https://preview.addons.mozilla.org/en-US/firefox/addon/10772"};


var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}

var teardownModule = function(module)
{
  search.removeEngine(searchEngine.name);
  search.restoreDefaultEngines();
}




var testGetMoreEngines = function()
{
  
  controller.assertJS("subject.isEngineInstalled == false",
                      {isEngineInstalled: search.isEngineInstalled(searchEngine.name)});

  
  var tabCount = controller.tabs.length;
  search.openEngineManager(enginesHandler);

  controller.waitForEval("subject.tabs.length == (subject.preCount + 1)", gTimeout, 100,
                         {tabs: controller.tabs, preCount: tabCount});
  controller.waitForPageLoad();
  
  
  controller.open(searchEngine.url);
  controller.waitForPageLoad();

  
  var md = new ModalDialogAPI.modalDialog(handleSearchInstall);
  md.start();

  
  var triggerLink = new elementslib.XPath(controller.tabs.activeTab,
                                          "//div[@id='addon-summary']/div/div/div/p/a/span");
  controller.waitThenClick(triggerLink, gTimeout);

  controller.waitForEval("subject.engine.isEngineInstalled(subject.name) == true", gTimeout, 100,
                         {engine: search, name: searchEngine.name});

  search.selectedEngine = searchEngine.name;
  search.search({text: "Firefox", action: "returnKey"});
}







var enginesHandler = function(controller)
{
  
  var browseLink = new elementslib.ID(controller.window.document, "addEngines");
  controller.waitThenClick(browseLink);
}







var handleSearchInstall = function(controller)
{
  
  var confirmTitle = UtilsAPI.getProperty("chrome://global/locale/search/search.properties",
                                          "addEngineConfirmTitle");

  if (mozmill.isMac)
    var title = controller.window.document.getElementById("info.title").textContent;
  else
    var title = controller.window.document.title;

  controller.assertJS("subject.windowTitle == subject.addEngineTitle",
                      {windowTitle: title, addEngineTitle: confirmTitle});

  
  var infoBody = new elementslib.ID(controller.window.document, "info.body");
  controller.waitForEval("subject.textContent.indexOf('addons.mozilla.org') != -1",
                         gTimeout, 100, infoBody.getNode());

  var addButton = new elementslib.Lookup(controller.window.document,
                                         '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}')
  controller.waitThenClick(addButton);
}





