




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'SearchAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

const searchEngine = {name: "MDC",
                      url : "https://litmus.mozilla.org/testcase_files/firefox/search/mozsearch.html"};

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




var testAddMozSearchPlugin = function()
{
  
  controller.open(searchEngine.url);
  controller.waitForPageLoad();

  
  var md = new ModalDialogAPI.modalDialog(handleSearchInstall);
  md.start();

  
  var addButton = new elementslib.Name(controller.tabs.activeTab, "add");
  controller.click(addButton);

  controller.waitForEval("subject.search.isEngineInstalled(subject.engine) == true", gTimeout, 100,
                         {search: search, engine: searchEngine.name});

  
  controller.assertJS("subject.newEngineNotSelected == true",
                      {newEngineNotSelected: search.selectedEngine != searchEngine.name});

  
  search.selectedEngine = searchEngine.name;
  search.search({text: "Firefox", action: "goButton"});
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

  
  var infoBody = controller.window.document.getElementById("info.body");
  controller.waitForEval("subject.textContent.indexOf('litmus.mozilla.org') != -1",
                         gTimeout, 100, infoBody);

  var addButton = new elementslib.Lookup(controller.window.document,
                                         '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.click(addButton);
}





