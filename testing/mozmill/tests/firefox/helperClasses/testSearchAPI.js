




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'SearchAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
  search.clear();
}

var teardownModule = function(module)
{
  search.clear();
  search.restoreDefaultEngines();
}




var testSearchAPI = function()
{
  
  controller.assertJS("subject.isGoogleInstalled == true",
                      {isGoogleInstalled: search.isEngineInstalled("Google")});
  controller.assertJS("subject.isGooglInstalled == false",
                      {isGooglInstalled: search.isEngineInstalled("Googl")});

  
  search.openEngineManager(handlerManager);

  
  search.selectedEngine = "Yahoo";
  search.search({text: "Firefox", action: "returnKey"});
}

var handlerManager = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);
  var engines = manager.engines;

  
  manager.removeEngine(engines[3].name);
  manager.controller.sleep(500);

  
  manager.moveDownEngine(engines[0].name);
  manager.moveUpEngine(engines[2].name);
  manager.controller.sleep(500);

  
  manager.editKeyword(engines[0].name, handlerKeyword);
  manager.controller.sleep(500);

  
  manager.restoreDefaults();
  manager.controller.sleep(500);

  
  manager.suggestionsEnabled = false;
  manager.controller.sleep(500);

  manager.getMoreSearchEngines();

  
  
}

var handlerKeyword = function(controller)
{
  var textbox = new elementslib.ID(controller.window.document, "loginTextbox");
  controller.type(textbox, "g");

  var okButton = new elementslib.Lookup(controller.window.document,
                                        '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.click(okButton);
}
