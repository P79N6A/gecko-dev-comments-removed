




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI', 'WidgetsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}

var teardownModule = function(module)
{
  search.restoreDefaultEngines();
}




var testRemoveEngine = function()
{
  var engine = search.visibleEngines[1];

  
  search.openEngineManager(handleEngines);

  controller.waitForEval("subject.oldEngine != subject.search.visibleEngines[1].name", gTimeout, 100,
                         {oldEngine: engine.name, search: search});
}







var handleEngines = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);

  
  var engines = manager.engines;
  controller.assertJS("subject.enginesCount > 1",
                      {enginesCount: engines.length});
  manager.removeEngine(engines[1].name);

  manager.close(true);
}





