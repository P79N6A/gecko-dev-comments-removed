




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI'];

const gDelay = 0;
const gTimeout = 5000;

const searchEngine = {name: "YouTube Video Search",
                      url : "http://www.youtube.com/"};

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




var testOpenSearchAutodiscovery = function()
{
  
  controller.open(searchEngine.url);
  controller.waitForPageLoad();

  
  var engineButton = search.getElement({type: "searchBar_dropDown"});
  controller.assertJS("subject.dropDownGlows == 'true'",
                      {dropDownGlows: engineButton.getNode().getAttribute('addengines')});

  
  search.enginesDropDownOpen = true;
  var addEngines = search.installableEngines;
  controller.assertJS("subject.installableEngines.length == 1",
                      {installableEngines: addEngines});

  
  var engine = search.getElement({type: "engine", subtype: "title", value: addEngines[0].name});
  controller.waitThenClick(engine);

  controller.waitForEval("subject.search.selectedEngine == subject.newEngine", gTimeout, 100,
                         {search: search, newEngine: searchEngine.name});

  
  search.search({text: "Firefox", action: "goButton"});

  
  var inputField = search.getElement({type: "searchBar_input"});
  search.clear();
  controller.assertValue(inputField, searchEngine.name);
}





